// Runs OoTMM's own entrance pool code at the pinned v32.3 revision for every M1 combination and writes the
// facts tools/ootmm/m1-contract.json records, for compare_m1_pinned_node.py to check against the fixture.
//
//   node --experimental-transform-types --no-warnings tools/ootmm/run_pinned_entrance_pools.mts \
//        --reference <ootmm-stable-32.3> --contract tools/ootmm/m1-contract.json --out <facts.json> [--jobs N]
//
// Nothing in the checkout is written, and nothing is installed. Module hooks registered here (no --import):
// - Every OoTMM source and data file is served from the Git blob at the contract's upstreamCommit, so checkout
//   line endings and local edits cannot change what runs; a file that is not a pinned blob fails the run.
//   Packages (lodash-es, yaml, csv) come from the checkout's node_modules, as for OoTMM itself.
// - Relative imports without an extension resolve to .ts / index.ts, and JSON imports need no attribute, as in
//   the bundler-style OoTMM sources (the rules of tooling/node-runner/ts-resolve.mjs).
// - packages/core/build/index.ts runs first, unchanged, and @ootmm/core then reads that fresh build of the pinned
//   data/ instead of packages/core/dist (which is only compared). Its helper packages/core/build/helpers.ts gets
//   two in-memory changes: (1) its `node:fs` import resolves to a data: URL shim (FS_SHIM_SOURCE) whose
//   readFileSync / globSync serve the pinned blobs (globSync also requires the checkout to list the same files)
//   and whose writeFileSync / mkdirSync keep the build output in memory; (2) `const __dirname = <the checkout's
//   packages/core/build as a JSON string>; ` is prefixed to its line 1, as tsx ran it as CommonJS (__dirname set).
//   A re-pin where helpers.ts stops using __dirname or these four fs functions (fs/promises, say) breaks here.
// - packages/logic/src/solver/entrances.ts gets one appended line, `export { WorldShuffler, LogicPassEntrances };`,
//   so the classes it keeps private can be driven directly.
// Those three are the only changes to OoTMM code. Each is recorded when applied, a worker fails unless all three
// were, and the facts list them (inMemoryChanges). The facts also record the sha256 of the contract bytes read
// (contract.sha256): sampleLayout, rows and the combination list come from the contract, so the comparer reports
// facts made from another contract as a divergence.
//
// Each combination's world is built as OoTMM builds it: options() (makeSettings + validateSettings, as
// generator.ts:191), worldState (pipeline.ts:63-76), then price and song events, the passes solvedWorldState
// runs before logicPassEntrances (pipeline.ts:78-84). A WorldShuffler made as runAttempt makes it
// (entrances.ts:1090-1101) then runs until its first random draw: placePoolsRecursive is replaced on the
// instance and records the pools placePools built after removing empty ones (entrances.ts:567-591). The pool,
// candidate, reset, dead-end, region and settings facts come from OoTMM's own methods on that state, so none
// depends on the RNG. One unmodified WorldShuffler.run() per combination (seeded as worldState seeds it) is also
// recorded; its layout depends on the RNG, so only whether it placed and where it may place are compared.
//
// With --temple <fixture> instead of --contract (tools/ootmm/temple-song-fixtures.json), it runs OoTMM's own
// validateAgeTemple (entrances.ts:265-287) instead, for the Temple probe's GLOBAL node (limits design, Stage 2):
//
//   node --experimental-transform-types --no-warnings tools/ootmm/run_pinned_entrance_pools.mts \
//        --reference <ootmm-stable-32.3> --temple tools/ootmm/temple-song-fixtures.json --out <facts.json> [--jobs N]
//
// Each case is a group of the fixture (a starting age, a world and an instrument) with no starting warp song or
// with one OoT warp song. Its world is built as above with erWallmasters ownGame and the fixture's settings and
// starting items (each world setting is checked to be applied); a crafted world then replaces the fixture's exits on that world before any shuffler sees it
// (recorded in the facts). The shuffler stops at its first random draw as above, and validateAgeTemple runs,
// unchanged, on changedWorld(game links, pools), the first world isValidShuffle hands it. In this mode only,
// Pathfinder.prototype.run is wrapped to keep the state it returns during that call, which gives the areas each age
// reached; the value it returns is unchanged. For the songless case of each group, the same world is also
// searched from each warp-song landing and from the opposite age's spawn area: a clone whose SPAWN has one exit, to
// that area at the opposite age, run as validateAgeTemple runs its own (`walks`). The facts hold no time or path,
// so a rerun with the same checkout writes the same bytes (tools/ootmm/temple-song-facts.json).
import { registerHooks } from 'node:module';
import { execFileSync } from 'node:child_process';
import { existsSync, globSync, readFileSync, realpathSync, statSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { availableParallelism } from 'node:os';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { Worker, isMainThread, parentPort, workerData } from 'node:worker_threads';
import path from 'node:path';

const M1_FLAGS = ['erOneWaysMajor', 'erOneWaysIkana', 'erOneWaysOwls', 'erOneWaysWaterVoids', 'erOneWaysAnywhere'];
const M1_KEYS = ['erWallmasters', 'erOneWays', ...M1_FLAGS];
const SEED = 'combo-ship-m1-c19';
const ENTRANCES_EXPORT = '\nexport { WorldShuffler, LogicPassEntrances };\n';
const BLOB_ROOTS = ['data/', 'packages/core/src/', 'packages/core/build/', 'packages/logic/src/'];
const HELPERS_TS = 'packages/core/build/helpers.ts';
const ENTRANCES_TS = 'packages/logic/src/solver/entrances.ts';
// The module helpers.ts imports in place of node:fs; ctx is set up in the worker.
const FS_SHIM_SOURCE = [
  'const ctx = globalThis.__pinnedCoreBuild;',
  'export function readFileSync(file) { return ctx.read(String(file)); }',
  'export function globSync(pattern, options) { return ctx.glob(pattern, options); }',
  'export function mkdirSync() {}',
  'export function writeFileSync(file, data) { ctx.write(String(file), data); }',
].join('\n');
// Each in-memory change to OoTMM code, as the facts name it (the text is what the hook applied).
const CHANGE_FS = `${HELPERS_TS}: import from 'node:fs' resolved to the in-memory shim`;
const CHANGE_DIRNAME = `${HELPERS_TS}: prefixed to line 1`;
const CHANGE_EXPORT = `${ENTRANCES_TS}: appended`;

const sha256 = (data: string | Buffer) => createHash('sha256').update(data).digest('hex');
const sorted = (values: Iterable<string>) => [...values].sort();
const combinationId = (s: any) =>
  [`erWallmasters=${s.erWallmasters}`, `erOneWays=${s.erOneWays}`, ...M1_FLAGS.filter(f => s[f])].join(';');
const m1Settings = (s: any) => Object.fromEntries(M1_KEYS.map(k => [k, s[k]]));

// --temple: one task per group (a starting age, a world and an instrument), in the fixture's order.
const templeGroups = (fx: any) => fx.groups.map(({ startingAge, world, instrument }: any) => {
  if (!['child', 'adult'].includes(startingAge) || !fx.worlds[world] || !fx.instruments[instrument]) {
    throw new Error(`Unknown temple group ${startingAge} ${world} ${instrument}`);
  }
  return { startingAge, world, instrument };
});
const templeCaseId = (g: any, song: string | null) =>
  `startingAge=${g.startingAge};world=${g.world};instrument=${g.instrument};song=${song ?? 'none'}`;

function argument(name: string, fallback?: string): string {
  const index = process.argv.indexOf(name);
  if (index >= 0 && index + 1 < process.argv.length) return process.argv[index + 1];
  if (fallback === undefined) throw new Error(`Missing ${name}`);
  return fallback;
}

// ======================================== worker: drives OoTMM ========================================

async function startWorker() {
  const REFERENCE: string = workerData.reference;
  const contract = JSON.parse(workerData.contractText);  // the bytes main hashed, not a second read
  const PIN: string = contract.upstreamCommit;
  const fold = (url: string) => (process.platform === 'win32' ? url.toLowerCase() : url);
  const refUrl = (rel: string) => pathToFileURL(path.join(REFERENCE, rel)).href;
  const relative = (file: string) => path.relative(REFERENCE, file).split(path.sep).join('/');

  // Pinned blobs, read once with git cat-file (no checkout file is trusted for OoTMM's own sources and data).
  const listing = execFileSync('git', ['-C', REFERENCE, 'ls-tree', '-r', '--name-only', PIN, '--', ...BLOB_ROOTS],
                               { encoding: 'utf8', maxBuffer: 1 << 26 }).split('\n').filter(Boolean);
  const raw = execFileSync('git', ['-C', REFERENCE, 'cat-file', '--batch'],
                           { input: listing.map(p => `${PIN}:${p}\n`).join(''), maxBuffer: 1 << 28 });
  const blobs = new Map<string, string>();
  for (let pos = 0, i = 0; i < listing.length; i++) {
    const end = raw.indexOf(10, pos);
    const [, kind, size] = raw.subarray(pos, end).toString('utf8').split(' ');
    if (kind !== 'blob') throw new Error(`Missing pinned blob ${listing[i]}`);
    blobs.set(listing[i], raw.subarray(end + 1, end + 1 + Number(size)).toString('utf8'));
    pos = end + 1 + Number(size) + 1;
  }
  const served = new Map<string, string>();
  const serve = (rel: string) => {
    const text = blobs.get(rel);
    if (text === undefined) throw new Error(`${rel} is not a blob of ${PIN}`);
    served.set(rel, sha256(text));
    return text;
  };

  // The core data build's fs: reads and globs from the pinned blobs, writes captured in memory.
  const built = new Map<string, string>();
  const literal = (text: string) => text.replace(/[.+?^${}()|[\]\\]/g, '\\$&');
  const globRegex = (pattern: string) => new RegExp('^' + pattern.split('/').map((part, i, parts) =>
    part === '**' ? '(?:[^/]+/)*' : part.split('*').map(literal).join('[^/]*') + (i < parts.length - 1 ? '/' : '')
  ).join('') + '$');
  (globalThis as any).__pinnedCoreBuild = {
    read(file: string) { return serve(relative(file)); },
    glob(pattern: string, options: { cwd: string }) {
      const base = relative(options.cwd);
      const regex = globRegex(pattern);
      const pinned = listing.filter(p => p.startsWith(base + '/') && regex.test(p.slice(base.length + 1)));
      // Node's globSync on the checkout gives the order the build sees; the file set must be the pinned one.
      const found = globSync(pattern, { cwd: options.cwd }).map(p => p.split(path.sep).join('/'));
      if (JSON.stringify(sorted(found)) !== JSON.stringify(sorted(pinned.map(p => p.slice(base.length + 1))))) {
        throw new Error(`The checkout's ${pattern} files differ from ${PIN}`);
      }
      return found;
    },
    write(file: string, data: string) { built.set(path.basename(file), String(data)); },
  };
  const FS_SHIM = 'data:text/javascript,' + encodeURIComponent(FS_SHIM_SOURCE);
  const changes = new Map<string, string>();  // every change made to OoTMM code, recorded as it is applied

  const REF_PREFIX = fold(pathToFileURL(REFERENCE + path.sep).href);
  const HELPERS_URL = fold(refUrl(HELPERS_TS));
  const ENTRANCES_URL = fold(refUrl(ENTRANCES_TS));
  const DIST_PREFIX = fold(refUrl('packages/core/dist') + '/');
  const SELF_URL = import.meta.url;
  const suffixes = ['.ts', '.tsx', '/index.ts', '/index.tsx'];
  const isPinned = (file: string) => blobs.has(relative(file));
  const isFile = (file: string) => isPinned(file) || (existsSync(file) && statSync(file).isFile());
  const packages = new Set<string>();

  registerHooks({
    resolve(specifier, context, nextResolve) {
      const parent = context.parentURL;
      if (parent && fold(parent) === HELPERS_URL && specifier === 'node:fs') {
        changes.set(CHANGE_FS, FS_SHIM_SOURCE);
        return { url: FS_SHIM, format: 'module', shortCircuit: true };
      }
      if (parent === SELF_URL && specifier.startsWith('@ootmm/')) {
        // Resolve the workspace packages exactly as the OoTMM sources do, through the checkout's node_modules.
        return nextResolve(specifier, { ...context, parentURL: refUrl('packages/logic/src/index.ts') });
      }
      if ((specifier.startsWith('./') || specifier.startsWith('../')) && parent?.startsWith('file:')) {
        const url = new URL(specifier, parent).href;
        if (fold(url).startsWith(DIST_PREFIX) && specifier.endsWith('.json')) {
          const importAttributes = { ...context.importAttributes, type: 'json' };
          return { url, format: 'json', importAttributes, shortCircuit: true };
        }
        const base = fileURLToPath(url);
        if (!isFile(base)) {
          for (const suffix of suffixes) {
            if (isFile(base + suffix)) return nextResolve(pathToFileURL(base + suffix).href, context);
          }
        }
      }
      let result;
      if (specifier.endsWith('.json')) {
        const attributes = { ...context.importAttributes, type: 'json' };
        result = nextResolve(specifier, { ...context, importAttributes: attributes });
        result = { ...result, importAttributes: { ...result.importAttributes, type: 'json' } };
      } else {
        result = nextResolve(specifier, context);
      }
      if (result.url.includes('/node_modules/') && !specifier.startsWith('.')) packages.add(result.url);
      return result;
    },
    load(url, context, nextLoad) {
      const key = fold(url);
      if (key.startsWith(DIST_PREFIX) && key.endsWith('.json')) {
        const fresh = built.get(path.basename(fileURLToPath(url)));
        if (fresh === undefined) throw new Error(`The pinned core build did not write ${url}`);
        return { format: 'json', source: fresh, shortCircuit: true };
      }
      const result = nextLoad(url, context);
      if (!key.startsWith(REF_PREFIX) || key.includes('/node_modules/')) return result;
      let source = serve(relative(fileURLToPath(url)));
      if (key === HELPERS_URL) {
        // helpers.ts is CommonJS-style TypeScript (run by tsx); as an ES module it needs __dirname. Same line 1.
        const prefix = `const __dirname = ${JSON.stringify(path.dirname(fileURLToPath(url)))}; `;
        changes.set(CHANGE_DIRNAME, prefix);
        source = prefix + source;
      } else if (key === ENTRANCES_URL) {
        changes.set(CHANGE_EXPORT, ENTRANCES_EXPORT);
        source += ENTRANCES_EXPORT;
      }
      return { ...result, source };
    },
  });

  // Fresh build of the pinned data with the pinned build script, then the OoTMM packages.
  await import(refUrl('packages/core/build/index.ts'));
  const distComparison = Object.fromEntries(sorted(built.keys()).map(name => {
    const onDisk = path.join(REFERENCE, 'packages/core/dist', name);
    const data = built.get(name)!;
    const checkoutDistMatches = existsSync(onDisk) && readFileSync(onDisk, 'utf8') === data;
    return [name, { sha256: sha256(data), checkoutDistMatches }];
  }));
  const core: any = await import('@ootmm/core');
  const logic: any = await import('@ootmm/logic');
  const { WorldShuffler, LogicPassEntrances } = logic;
  if (typeof WorldShuffler !== 'function' || typeof LogicPassEntrances !== 'function') {
    throw new Error('entrances.ts was not loaded through the export hook');
  }
  for (const file of ['packages/core/src/data/data.ts', ENTRANCES_TS, HELPERS_TS, 'data/defs/entrances.yml']) {
    if (!served.has(file)) throw new Error(`${file} was not served from the pinned blob`);
  }
  const expectedChanges = [CHANGE_DIRNAME, CHANGE_EXPORT, CHANGE_FS];
  if (JSON.stringify(sorted(changes.keys())) !== JSON.stringify(sorted(expectedChanges))) {
    throw new Error(`In-memory changes applied: ${sorted(changes.keys()).join(' | ')}; expected ${expectedChanges}`);
  }
  const ENTRANCES = core.ENTRANCES;
  const monitor = new core.Monitor({ onLog: () => {}, onWarn: () => {}, onProgress: () => {} });
  const timing: Record<string, number> = {};
  const timed = <T,>(name: string, run: () => T): T => {
    const start = performance.now();
    try { return run(); } finally { timing[name] = (timing[name] || 0) + (performance.now() - start) / 1000; }
  };

  async function buildState(settings: any) {
    const start = performance.now();
    const opts = core.options({ seed: SEED, settings });
    let state: any = await logic.worldState(monitor, opts);
    state = { ...state, ...logic.logicPassPrice(state) };
    state = { ...state, ...logic.logicPassSongEvents(state) };
    timing.worldBuild = (timing.worldBuild || 0) + (performance.now() - start) / 1000;
    return state;
  }

  class Stop extends Error {}

  function capturePools(state: any, settings: any) {
    const shuffler = new WorldShuffler(state.random, 0, state.worlds, settings, state.startingItems, state.allItems);
    let captured: any = null;
    shuffler.placePoolsRecursive = (pools: any) => { captured = pools; throw new Stop(); };
    try {
      shuffler.run();
    } catch (error) {
      if (!(error instanceof Stop)) throw error;
    }
    if (captured === null) throw new Error('placePools never reached placePoolsRecursive');
    return { shuffler, pools: captured as Record<string, any> };
  }

  // One real placement attempt, unchanged: WorldShuffler.run() with the RNG worldState seeded, as the first attempt
  // of logicPassEntrances would make it (entrances.ts:905-934, 500-599, validity per step 388-416). The layout
  // depends on the RNG; only its RNG-independent properties are compared (success, pairs within the candidates).
  function place(state: any, settings: any, facts: any[]) {
    const shuffler = new WorldShuffler(state.random, 0, state.worlds, settings, state.startingItems, state.allItems);
    try {
      const { world } = shuffler.run();
      const sources = new Set(facts.flatMap(p => p.src));
      const pairs = [...world.entranceOverrides].filter(([s]: [string, string]) => sources.has(s)).sort();
      return { placed: true, pairs };
    } catch (error: any) {
      if (!(error instanceof logic.LogicEntranceError)) throw error;
      return { placed: false, error: String(error.message) };
    }
  }

  function poolFacts(shuffler: any, pools: Record<string, any>) {
    const descrs = shuffler.makePools();
    return Object.entries(pools).map(([name, pool]: [string, any]) => {
      const candidates: Record<string, string[]> = {};
      for (const src of sorted(pool.src)) {
        // The candidate filter of placePoolsRecursive (entrances.ts:513).
        candidates[src] = sorted([...pool.dst].filter((dst: string) =>
          shuffler.isAssignableConstraints(src, dst, !!pool.opts.ownGame)));
      }
      const count = (names: Set<string>) => ({ oot: [...names].filter(n => ENTRANCES[n].game === 'oot').length,
                                               mm: [...names].filter(n => ENTRANCES[n].game === 'mm').length });
      return { name, ownGame: !!pool.opts.ownGame, alias: !!pool.opts.alias,
               srcTypes: descrs[name].src, dstTypes: descrs[name].dst,
               srcCount: count(pool.src), dstCount: count(pool.dst),
               src: sorted(pool.src), dst: sorted(pool.dst), candidates };
    });
  }

  // placeSingleExpr (entrances.ts:203-217) on a copy of the source area: does it AND can_reset_time?
  function resetGate(shuffler: any, src: string, dst: string) {
    const from = ENTRANCES[src].from;
    const scratch = { areas: { [from]: { exits: { ...shuffler.world.areas[from].exits } } } };
    const songOfTime = Object.getPrototypeOf(shuffler).songOfTime;
    let gated = false;
    shuffler.songOfTime = (expr: any) => { gated = true; return songOfTime.call(shuffler, expr); };
    try {
      shuffler.placeSingleExpr(scratch, src, dst);
    } finally {
      delete shuffler.songOfTime;
    }
    return gated;
  }

  // changedWorld then LogicPassEntrances.processRegions (entrances.ts:349-386, 1044-1059): a digest of every
  // area's region, every location's region and dungeonsEntrances.
  function regionsAfter(state: any, shuffler: any, overrides: any, pools: any, extraHintRegions: boolean) {
    const world = shuffler.changedWorld(overrides, pools);
    const pass = new LogicPassEntrances({ ...state, settings: { ...state.settings, extraHintRegions } });
    pass.worlds = [world];
    pass.processRegions();
    // With pools, changeWorldAssumePools adds the 'ASSUMED' hub (region NONE, no locations); it is not compared.
    const areas = Object.keys(world.areas).filter(a => a !== 'ASSUMED').sort().map(a => [a, world.areas[a].region]);
    const locations = Object.keys(world.regions).sort().map(l => [l, world.regions[l]]);
    const dungeons = [...world.dungeonsEntrances].sort();
    return sha256(JSON.stringify({ areas, locations, dungeons }));
  }

  // The pool facts with the OoT Wallmaster sources left out, which may differ between vanilla and MQ.
  const ootWallmasters = Object.keys(ENTRANCES)
    .filter(n => ENTRANCES[n].type === 'wallmaster' && ENTRANCES[n].game === 'oot');
  const withoutOotWallmasters = (facts: any[]) => sha256(JSON.stringify(facts.map(({ srcCount, ...p }) => ({
    ...p, src: p.src.filter((s: string) => !ootWallmasters.includes(s)),
    candidates: Object.fromEntries(Object.entries(p.candidates).filter(([s]) => !ootWallmasters.includes(s))),
  }))));
  const wallmasterAreas = new Set(ootWallmasters.map(n => ENTRANCES[n].from));
  const wallmasterDungeons = Object.keys(core.WORLD.mq).filter(key => ['oot', 'mq'].some(section =>
    Object.keys(core.WORLD[section][key] || {}).some(area => wallmasterAreas.has(core.gameId('oot', area, ' ')))));

  async function combination(index: number) {
    const entry = contract.combinations[index];
    const requested = m1Settings(entry.settings);
    const state = await buildState(requested);
    const { shuffler, pools } = timed('pools', () => capturePools(state, state.settings));
    const facts = timed('pools', () => poolFacts(shuffler, pools));
    const gates: Record<string, Set<boolean>> = {};
    timed('resetGate', () => {
      for (const pool of facts) {
        for (const [src, dsts] of Object.entries(pool.candidates)) {
          for (const dst of dsts) (gates[dst] ||= new Set()).add(resetGate(shuffler, src, dst));
        }
      }
    });
    const destinations = Object.keys(gates);
    // The fixture's sample layout and the union of every candidate edge, through OoTMM's own world change.
    const base = { ...shuffler.overrides };  // connectGamesDefault (entrances.ts:888-903)
    const layout: [string, string][] = entry.sampleLayout;
    const pairs = new Set(facts.flatMap(p =>
      Object.entries(p.candidates).flatMap(([s, ds]) => ds.map(d => `${s}>${d}`))));
    const sources = facts.flatMap(p => p.src);
    const regions: Record<string, any> = {};
    timed('regions', () => {
      for (const extra of [false, true]) {
        const noEr = regionsAfter(state, shuffler, base, undefined, extra);
        const sample = regionsAfter(state, shuffler, { ...base, ...Object.fromEntries(layout) }, undefined, extra);
        const union = facts.length ? regionsAfter(state, shuffler, base, pools, extra) : noEr;
        regions[`extraHintRegions=${extra}`] = { sampleLayoutUnchanged: sample === noEr,
                                                 allCandidatesUnchanged: union === noEr };
      }
    });
    const erEnabled = state.worlds[0].exprParsers.mm.parse('er_enabled');
    const placement = timed('placement', () => place(state, state.settings, facts));
    return {
      id: entry.id, requested, normalized: m1Settings(state.settings),
      erEnabled: erEnabled === logic.exprTrue() ? true : erEnabled === logic.exprFalse() ? false : 'not constant',
      isEntranceShuffle: logic.isEntranceShuffle(state.settings),
      gameLinks: Object.entries(base).sort(),
      pools: facts,
      resetGated: sorted(destinations.filter(d => gates[d].has(true))),
      resetGateDependsOnSource: sorted(destinations.filter(d => gates[d].size !== 1)),
      deadEnds: sorted(destinations.filter(d =>
        Object.keys(shuffler.world.areas[ENTRANCES[d].to].exits).length === 0)),
      sampleLayout: {
        pairsOutsideCandidates: layout.filter(([s, d]) => !pairs.has(`${s}>${d}`)),
        sourcesWithoutPair: sources.filter(s => !layout.some(([a]) => a === s)).sort(),
        sourcesPairedTwice: sorted(new Set(layout.map(([s]) => s).filter((s, i, all) => all.indexOf(s) !== i))),
        regions,
      },
      placement,
      areaKeys: sha256(JSON.stringify(Object.keys(state.worlds[0].areas).sort())),
      withoutOotWallmasters: withoutOotWallmasters(facts),
    };
  }

  // OoT Wallmaster sources for one vanilla/MQ choice. The world is built once, with erWallmasters=ownGame, and the
  // shuffler runs on it with the settings of each combination that has Wallmasters: the only M1 key a world
  // expression reads is er_enabled (macros_mm.yml:305), already true whenever Wallmasters are on, and the area
  // set does not depend on er* keys (checked over the 66 all-vanilla builds), so it is the world OoTMM builds
  // for each of those combinations.
  const WALLMASTERS_ALONE = 'erWallmasters=ownGame;erOneWays=none';
  async function mqContext(bits: number) {
    const mq = wallmasterDungeons.filter((_: string, i: number) => bits & (1 << i));
    const mqDungeons = { type: 'specific', values: mq };
    const state = await buildState({ erWallmasters: 'ownGame', mqDungeons });
    const flag = state.worlds[0].resolvedFlags.mqDungeons;
    if (wallmasterDungeons.some((k: string) => flag.has(k) !== mq.includes(k))) {
      throw new Error(`mqDungeons ${mq} not applied`);
    }
    let ootSources: string[] | null = null;
    const shortfalls: string[] = [];
    const others: Record<string, string> = {};
    const placements: Record<string, any> = {};
    for (const entry of contract.combinations.filter((c: any) => c.settings.erWallmasters !== 'none')) {
      const settings = core.makeSettings({ ...m1Settings(entry.settings), mqDungeons });
      const facts = timed('pools', () => {
        const { shuffler, pools } = capturePools(state, settings);
        return poolFacts(shuffler, pools);
      });
      const wallmasters = facts.find(p => p.name === 'WALLMASTERS')!;
      const sources = wallmasters.src.filter((s: string) => ENTRANCES[s].game === 'oot');
      if (ootSources !== null && JSON.stringify(ootSources) !== JSON.stringify(sources)) {
        throw new Error('OoT Wallmaster sources depend on er* keys');
      }
      ootSources = sources;
      // Own-game alias pool: the OoT sources need distinct OoT destinations from their candidate sets.
      const reachable = new Set(sources.flatMap((s: string) => wallmasters.candidates[s]));
      const short = sources.length > reachable.size;
      if (short) shortfalls.push(entry.id);
      others[entry.id] = withoutOotWallmasters(facts);
      // A real attempt for every shortfall, and for Wallmasters alone (the tightest own-game capacity).
      if (short || entry.id === WALLMASTERS_ALONE) {
        placements[entry.id] = timed('placement', () => place(state, settings, facts));
      }
    }
    return { mq, ootSources: sorted(ootSources!), shortfalls, others, placements,
             areaKeys: sha256(JSON.stringify(Object.keys(state.worlds[0].areas).sort())) };
  }

  // The registry fields the fixture records for each M1 row, as @ootmm/core's ENTRANCES has them.
  function rows() {
    return Object.fromEntries(Object.keys(contract.rows).sort().map(name => {
      const e = ENTRANCES[name];
      if (!e) return [name, null];
      return [name, { game: e.game, id: e.id, type: e.type, reverse: e.reverse ?? null, from: e.from, to: e.to,
                      fromMap: e.fromMap, toMap: e.toMap, noSot: e.flags.includes('no-sot') }];
    }));
  }

  // validateSettings over the 128 raw M1 inputs: the ones makeSettings keeps unchanged.
  function settingsFixedPoints() {
    const result: string[] = [];
    for (const erWallmasters of ['none', 'ownGame']) {
      for (const erOneWays of ['none', 'ownGame']) {
        for (let bits = 0; bits < 32; bits++) {
          const s: any = { erWallmasters, erOneWays };
          M1_FLAGS.forEach((flag, i) => { s[flag] = !!(bits & (1 << i)); });
          const made = core.makeSettings(s);
          if (M1_KEYS.every(k => made[k] === s[k])) result.push(combinationId(s));
        }
      }
    }
    return sorted(result);
  }

  // ---------------------------------------- --temple: validateAgeTemple ----------------------------------------
  // The pathfinder states validateAgeTemple computes, read through a wrapper that returns the same value. It is
  // installed by the first --temple task, so the M1 runs never see it.
  let pathfinderStates: any[] | null = null;
  let pathfinderWrapped = false;
  const watchPathfinder = () => {
    if (pathfinderWrapped) return;
    pathfinderWrapped = true;
    const pathfinderRun = logic.Pathfinder.prototype.run;
    logic.Pathfinder.prototype.run = function (this: any, ...rest: any[]) {
      const result = pathfinderRun.apply(this, rest);
      if (pathfinderStates) pathfinderStates.push(result);
      return result;
    };
  };
  const AGE_NAME = (age: any) => (age === logic.AGE_CHILD ? 'child' : 'adult');
  // The six one-way-song rows (entrances.yml:174-179), by the song area they leave: SONG_TP_<song> -> landing.
  const warpLanding = (song: string) => {
    const rows = Object.keys(ENTRANCES).filter(n => ENTRANCES[n].type === 'one-way-song' &&
                                                   ENTRANCES[n].from === `OOT SONG_TP_${song}`);
    if (rows.length !== 1) throw new Error(`Expected one warp song row for ${song}, found ${rows}`);
    return { row: rows[0], landing: ENTRANCES[rows[0]].to as string };
  };
  const templeSettings = (fx: any, c: any, song: string | null) => {
    const world = fx.worlds[c.world], instrument = fx.instruments[c.instrument];
    const startingItems = { ...instrument.items, ...(song ? { [`OOT_SONG_TP_${song}`]: 1 } : {}) };
    return { ...world.settings, ...instrument.settings, startingAge: c.startingAge, erWallmasters: 'ownGame',
             startingItems };
  };
  const craft = (fx: any, c: any, state: any) => {
    const crafted: string[] = [];
    for (const [from, exits] of Object.entries(fx.worlds[c.world].exits || {}) as [string, any][]) {
      for (const [to, rule] of Object.entries(exits) as [string, string][]) {
        const area = state.worlds[0].areas[from];
        if (!area || !(to in area.exits)) throw new Error(`Crafted exit ${from} -> ${to} is not in the world`);
        area.exits[to] = rule === 'adult' ? logic.exprAge(logic.AGE_ADULT) : rule === 'child'
          ? logic.exprAge(logic.AGE_CHILD) : rule === 'false' ? logic.exprFalse() : (() => {
            throw new Error(`Unknown crafted rule ${rule}`);
          })();
        crafted.push(`${from} -> ${to}: ${rule}`);
      }
    }
    return crafted;
  };
  // One world: its validateAgeTemple answer and what each age reached in that search.
  async function templeWorld(fx: any, c: any, song: string | null) {
    const state = await buildState(templeSettings(fx, c, song));
    // The world's own settings reach OoTMM as written (a set, such as mqDungeons, through its resolved flags).
    for (const [key, value] of Object.entries(fx.worlds[c.world].settings) as [string, any][]) {
      const applied = value?.type === 'specific'
        ? value.values.every((v: string) => state.worlds[0].resolvedFlags[key].has(v))
        : state.settings[key] === value;
      if (!applied) throw new Error(`World setting ${key} of ${c.world} not applied`);
    }
    const crafted = craft(fx, c, state);
    const { shuffler, pools } = timed('pools', () => capturePools(state, state.settings));
    const world = shuffler.changedWorld({ ...shuffler.overrides }, pools);
    let temple = false, runs: any[] = [];
    watchPathfinder();
    pathfinderStates = runs;
    try {
      temple = timed('temple', () => shuffler.validateAgeTemple(world));
    } finally {
      pathfinderStates = null;
    }
    if (runs.length !== 1) throw new Error(`validateAgeTemple ran ${runs.length} searches`);
    const ws = runs[0].ws[0];
    const ages: Record<string, any> = {};
    for (const age of [logic.AGE_CHILD, logic.AGE_ADULT]) {
      const areas: Map<string, any> = ws.ages[age].areas;
      const names = [...areas.keys()];
      ages[AGE_NAME(age)] = {
        areas: names.length,
        global: areas.has('OOT GLOBAL'),
        songAreas: sorted(names.filter(a => a.startsWith('OOT SONG_TP_'))),
        mm: names.some(a => a.startsWith('MM ')),
        market: areas.has('OOT Market'),
        temple: areas.has('OOT Temple of Time'),
      };
    }
    return { state, shuffler, world, temple, ages, crafted,
             normalized: Object.fromEntries(['startingAge', 'dekuTree', 'ocarinaButtonsShuffleOot',
                                             'sharedOcarinaButtons', 'erWallmasters', 'logic', 'games']
                                             .map(k => [k, state.settings[k]])),
             startingItems: sorted([...state.startingItems].map(([pi, n]: [any, number]) => `${pi.item.id}=${n}`)) };
  }
  // A songless world searched from one area at the opposite age, as validateAgeTemple searches from SPAWN.
  function walk(base: any, opposite: any, area: string) {
    const world = logic.cloneWorld(base.world);
    if (!world.areas[area]) throw new Error(`No area ${area}`);
    world.areas['OOT SPAWN'].exits = { [area]: logic.exprAge(opposite) };
    const worlds = [...base.state.worlds];
    worlds[0] = world;
    const pathfinder = new logic.Pathfinder(worlds, base.state.settings, base.state.startingItems);
    const state = timed('walk', () => pathfinder.run(null, { recursive: true, singleWorld: 0 }));
    return state.ws[0].ages[opposite].areas.has('OOT Temple of Time');
  }
  // One starting age, world and instrument: the songless case with its walks, then each song alone.
  async function templeGroup(index: number) {
    const fx = contract;
    const group = templeGroups(fx)[index];
    const opposite = group.startingAge === 'child' ? logic.AGE_ADULT : logic.AGE_CHILD;
    const songs = fx.songs.map((s: string) => ({ song: s, ...warpLanding(s) }));
    const base = await templeWorld(fx, group, null);
    const spawnArea = opposite === logic.AGE_CHILD ? "OOT Link's House" : 'OOT Temple of Time';
    const walks: Record<string, boolean> = {};
    for (const area of [spawnArea, ...songs.map((s: any) => s.landing)]) walks[area] = walk(base, opposite, area);
    const cases = [];
    for (const { song, row, landing } of [{ song: null, row: null, landing: null }, ...songs]) {
      const result = song === null ? base : await templeWorld(fx, group, song);
      cases.push({ id: templeCaseId(group, song), song, row, landing, temple: result.temple, ages: result.ages,
                   crafted: result.crafted, normalized: result.normalized, startingItems: result.startingItems });
    }
    return { ...group, opposite: AGE_NAME(opposite), spawnArea, walks, cases,
             inMemoryChanges: Object.fromEntries(sorted(changes.keys()).map(k => [k, changes.get(k)!])) };
  }

  parentPort!.on('message', async (task: any) => {
    try {
      const result = task.kind === 'combination' ? await combination(task.index)
        : task.kind === 'mq' ? await mqContext(task.bits)
        : task.kind === 'temple' ? await templeGroup(task.index)
        : { fixedPoints: settingsFixedPoints(), wallmasterDungeons, distComparison, rows: rows(),
            inMemoryChanges: Object.fromEntries(sorted(changes.keys()).map(k => [k, changes.get(k)!])) };
      parentPort!.postMessage({ task, result, served: Object.fromEntries(served), packages: [...packages], timing });
    } catch (error: any) {
      parentPort!.postMessage({ task, error: String(error?.stack || error) });
    }
  });
  parentPort!.postMessage({ ready: true });
}

// ======================================== main: schedules and merges ========================================

function packageVersions(urls: Iterable<string>) {
  const versions: Record<string, string> = {};
  for (const url of urls) {
    for (let dir = path.dirname(fileURLToPath(url)); dir !== path.dirname(dir); dir = path.dirname(dir)) {
      const manifest = path.join(dir, 'package.json');
      if (existsSync(manifest)) {
        const data = JSON.parse(readFileSync(manifest, 'utf8'));
        if (data.name && data.version) { versions[data.name] = data.version; break; }
      }
    }
  }
  return Object.fromEntries(Object.entries(versions).sort());
}

// --temple: every group of the fixture, merged in its order. Timing and paths stay out of the facts' cases.
async function mainTemple() {
  const reference = realpathSync.native(path.resolve(argument('--reference')));
  const fixturePath = path.resolve(argument('--temple'));
  const out = path.resolve(argument('--out'));
  const fixtureBytes = readFileSync(fixturePath);
  const contractText = fixtureBytes.toString('utf8');
  const fixture = JSON.parse(contractText);
  const jobs = Math.max(1, Number(argument('--jobs', String(Math.min(8, Math.max(1, availableParallelism() - 2))))));
  const started = performance.now();
  const tasks = templeGroups(fixture).map((_: any, index: number) => ({ kind: 'temple', index }));
  const results = new Map<number, any>();
  const served: Record<string, string> = {};
  const packageUrls = new Set<string>();
  const timings: Record<string, number>[] = [];
  const workers: Worker[] = [];
  try {
    await new Promise<void>((resolve, reject) => {
      let next = 0;
      let pending = tasks.length;
      const dispatch = (worker: Worker) => { if (next < tasks.length) worker.postMessage(tasks[next++]); };
      for (let i = 0; i < Math.min(jobs, tasks.length); i++) {
        const worker = new Worker(new URL(import.meta.url), { workerData: { reference, contractText } });
        workers.push(worker);
        worker.on('error', reject);
        worker.on('exit', code => { if (pending > 0) reject(new Error(`A worker exited (${code}) with tasks left`)); });
        worker.on('message', (message: any) => {
          if (message.ready) return dispatch(worker);
          if (message.error) return reject(new Error(`${JSON.stringify(message.task)}: ${message.error}`));
          results.set(message.task.index, message.result);
          Object.assign(served, message.served);
          message.packages.forEach((u: string) => packageUrls.add(u));
          timings[i] = message.timing;
          if (--pending === 0) return resolve();
          dispatch(worker);
        });
      }
    });
  } finally {
    await Promise.all(workers.map(w => w.terminate()));
  }
  const workerSeconds: Record<string, number> = {};
  for (const t of timings) for (const [k, v] of Object.entries(t || {})) workerSeconds[k] = (workerSeconds[k] || 0) + v;
  const groups = tasks.map((t: any) => results.get(t.index));
  const changes = groups.map((g: any) => JSON.stringify(g.inMemoryChanges));
  if (new Set(changes).size !== 1) throw new Error('Workers applied different in-memory changes');
  const facts = {
    upstreamCommit: fixture.upstreamCommit,
    fixture: { sha256: sha256(fixtureBytes) },
    node: process.version,
    seed: SEED,
    // Their names only: the __dirname prefix holds this checkout's path.
    inMemoryChanges: Object.keys(groups[0].inMemoryChanges),
    observation: 'Pathfinder.prototype.run wrapped to keep the state validateAgeTemple searched; its value is unchanged',
    servedBlobs: Object.fromEntries(Object.entries(served).sort()),
    packages: packageVersions(packageUrls),
    groups: groups.map(({ inMemoryChanges, ...g }: any) => g),
  };
  writeFileSync(out, JSON.stringify(facts, null, 1) + '\n');
  const seconds = Object.entries(workerSeconds).map(([k, v]) => `${k} ${v.toFixed(1)}`).join(', ');
  console.log(`Pinned OoTMM validateAgeTemple: ${groups.length} groups, ` +
              `${groups.reduce((n: number, g: any) => n + g.cases.length, 0)} cases in ` +
              `${((performance.now() - started) / 1000).toFixed(1)} s with ${jobs} workers (${seconds}) -> ${out}`);
}

async function main() {
  if (process.argv.includes('--temple')) return mainTemple();
  const reference = realpathSync.native(path.resolve(argument('--reference')));
  const contractPath = path.resolve(argument('--contract'));
  const out = path.resolve(argument('--out'));
  // Read once: these bytes are hashed into the facts and are the only contract the workers see.
  const contractBytes = readFileSync(contractPath);
  const contractText = contractBytes.toString('utf8');
  const contract = JSON.parse(contractText);
  const jobs = Math.max(1, Number(argument('--jobs', String(Math.min(8, Math.max(1, availableParallelism() - 2))))));
  const started = performance.now();

  const tasks: any[] = [{ kind: 'meta' }];
  contract.combinations.forEach((_: any, index: number) => tasks.push({ kind: 'combination', index }));
  const mqContexts = 1 << contract.mqSources.dungeons.length;  // checked against OoTMM's own list below
  for (let bits = 0; bits < mqContexts; bits++) tasks.push({ kind: 'mq', bits });
  tasks.sort((a, b) => (a.kind === 'mq' ? 0 : 1) - (b.kind === 'mq' ? 0 : 1));  // MQ worlds first, all similar

  const results = new Map<string, any>();
  const served: Record<string, string> = {};
  const packageUrls = new Set<string>();
  const timings: Record<string, number>[] = [];
  const workers: Worker[] = [];
  try {
    await new Promise<void>((resolve, reject) => {
      let next = 0;
      let pending = tasks.length;
      const dispatch = (worker: Worker) => { if (next < tasks.length) worker.postMessage(tasks[next++]); };
      for (let i = 0; i < Math.min(jobs, tasks.length); i++) {
        const worker = new Worker(new URL(import.meta.url), { workerData: { reference, contractText } });
        workers.push(worker);
        worker.on('error', reject);
        worker.on('exit', code => { if (pending > 0) reject(new Error(`A worker exited (${code}) with tasks left`)); });
        worker.on('message', (message: any) => {
          if (message.ready) return dispatch(worker);
          if (message.error) return reject(new Error(`${JSON.stringify(message.task)}: ${message.error}`));
          results.set(`${message.task.kind}:${message.task.index ?? message.task.bits ?? ''}`, message.result);
          Object.assign(served, message.served);
          message.packages.forEach((u: string) => packageUrls.add(u));
          timings[i] = message.timing;
          if (--pending === 0) return resolve();
          dispatch(worker);
        });
      }
    });
  } finally {
    await Promise.all(workers.map(w => w.terminate()));
  }
  const workerSeconds: Record<string, number> = {};
  for (const t of timings) for (const [k, v] of Object.entries(t || {})) workerSeconds[k] = (workerSeconds[k] || 0) + v;

  const meta = results.get('meta:');
  if (meta.wallmasterDungeons.length !== contract.mqSources.dungeons.length) {
    throw new Error(`OoTMM has ${meta.wallmasterDungeons.length} Wallmaster dungeons; ` +
                    `the contract lists ${contract.mqSources.dungeons.length}`);
  }
  const combinations = contract.combinations.map((_: any, i: number) => results.get(`combination:${i}`));
  const vanilla = Object.fromEntries(combinations.map((c: any) => [c.id, c.withoutOotWallmasters]));
  const table = [];
  const capacityShortfalls = [];
  const otherRowsDependOnMq: Record<string, string[]> = {};
  const mqAreaKeys = new Set<string>();
  for (let bits = 0; bits < mqContexts; bits++) {
    const context = results.get(`mq:${bits}`);
    table.push({ mq: context.mq, ootSources: context.ootSources, placements: context.placements });
    if (context.shortfalls.length) {
      capacityShortfalls.push({ mq: context.mq, ootSources: context.ootSources.length,
                                combinations: context.shortfalls });
    }
    const differing = Object.keys(context.others).filter(id => context.others[id] !== vanilla[id]);
    if (differing.length) otherRowsDependOnMq[context.mq.join('+') || 'vanilla'] = differing;
    mqAreaKeys.add(context.areaKeys);
  }
  const facts = {
    upstreamCommit: contract.upstreamCommit,
    contract: { path: contractPath, sha256: sha256(contractBytes) },
    node: process.version,
    jobs,
    seed: SEED,
    inMemoryChanges: meta.inMemoryChanges,
    servedBlobs: Object.fromEntries(Object.entries(served).sort()),
    packages: packageVersions(packageUrls),
    coreBuild: meta.distComparison,
    settingsFixedPoints: meta.fixedPoints,
    rows: meta.rows,
    areaKeysInvariantAcrossCombinations: new Set(combinations.map((c: any) => c.areaKeys)).size === 1,
    combinations: combinations.map(({ areaKeys, withoutOotWallmasters, ...rest }: any) => rest),
    mqSources: { dungeons: meta.wallmasterDungeons, table, capacityShortfalls, otherRowsDependOnMq,
                 distinctAreaSets: mqAreaKeys.size },
    seconds: { wall: (performance.now() - started) / 1000, summedOverWorkers: workerSeconds },
  };
  writeFileSync(out, JSON.stringify(facts, null, 1) + '\n');
  console.log(`Pinned OoTMM facts: ${combinations.length} combinations and ${table.length} MQ contexts in ` +
              `${facts.seconds.wall.toFixed(1)} s with ${jobs} workers -> ${out}`);
}

if (isMainThread) {
  await main();
} else {
  await startWorker();
}
