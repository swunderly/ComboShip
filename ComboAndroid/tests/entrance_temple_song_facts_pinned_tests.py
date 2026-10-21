"""OoTMM's own validateAgeTemple, run with the pinned Node, still gives the recorded GLOBAL facts.

tools/ootmm/run_pinned_entrance_pools.mts --temple runs the exact OoTMM v32.3 code on every case of
tools/ootmm/temple-song-fixtures.json (a starting age, a world, an instrument, no starting warp song or one) and
writes no time or path, so the same checkout writes the same bytes. This reruns it and requires
tools/ootmm/temple-song-facts.json byte for byte, which entrance_temple_global_lemma_tests.py reads. A fixture case
the world does not have (a crafted exit that is not there) fails the run.

Reports SKIPPED when the pinned Node (COMBO_PINNED_NODE, or tooling/node/node-v24.21.0-* above this checkout) or the
OoTMM checkout with its node_modules (COMBO_OOTMM_REFERENCE, or references/ootmm-stable-32.3) is absent.
"""
from pathlib import Path
import argparse
import json
import os
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
RUNNER = source/'tools/ootmm/run_pinned_entrance_pools.mts'
FIXTURE = source/'tools/ootmm/temple-song-fixtures.json'
FACTS = source/'tools/ootmm/temple-song-facts.json'
NODE_VERSION = 'v24.21.0'


def find_node():
    configured = os.environ.get('COMBO_PINNED_NODE')
    if configured:
        return Path(configured)
    for parent in source.parents:
        for folder in sorted((parent/'tooling/node').glob(f'node-{NODE_VERSION}-*')):
            for name in ('node.exe', 'bin/node'):
                if (folder/name).is_file():
                    return folder/name
    return None


def find_reference():
    configured = os.environ.get('COMBO_OOTMM_REFERENCE')
    if configured:
        return Path(configured)
    return next((parent/'references/ootmm-stable-32.3' for parent in source.parents
                 if (parent/'references/ootmm-stable-32.3/.git').exists()), None)


node, reference = find_node(), find_reference()
if node is None or not node.is_file():
    print(f'SKIPPED: the pinned Node {NODE_VERSION} is not installed (set COMBO_PINNED_NODE, or unpack it under '
          f'tooling/node/ above this checkout)')
    sys.exit(0)
if reference is None or not (reference/'.git').exists():
    print('SKIPPED: set COMBO_OOTMM_REFERENCE to the exact local OoTMM v32.3 checkout to run its Temple check')
    sys.exit(0)
missing = [p for p in ('lodash-es', 'yaml', 'csv') if not (reference/'node_modules'/p).exists()]
if missing:
    print(f'SKIPPED: the OoTMM checkout has no node_modules for {", ".join(missing)}; nothing is installed here')
    sys.exit(0)
sys.path.insert(0, str(source/'tools/ootmm'))
import export_registry as registry
registry.verified_blob(reference, 'data/defs/entrances.yml')  # refuses any other revision
version = subprocess.run([str(node), '--version'], capture_output=True, text=True, timeout=60).stdout.strip()
assert version == NODE_VERSION, version


def run(fixture, out):
    command = [str(node), '--experimental-transform-types', '--no-warnings', str(RUNNER), '--reference',
               str(reference.resolve()), '--temple', str(fixture), '--out', str(out), '--jobs', '4']
    return subprocess.run(command, capture_output=True, text=True, encoding='utf-8', timeout=900)


out = work/'temple-song-facts.json'
out.unlink(missing_ok=True)
result = run(FIXTURE, out)
assert result.returncode == 0, result.stdout + result.stderr
assert out.read_bytes() == FACTS.read_bytes(), \
    f'The pinned run differs from {FACTS.name}; rerun it with --out {FACTS} and review the change'
print(result.stdout.strip())

# A crafted exit the world does not have is an error, not a silent case.
bad = json.loads(FIXTURE.read_text(encoding='utf-8'))
bad['worlds'] = {'missing': {'settings': {}, 'exits': {'OOT Kokiri Forest': {'OOT Nowhere': 'adult'}}}}
bad['instruments'] = {'none': {'settings': {}, 'items': {}}}
bad['groups'] = [{'startingAge': 'adult', 'world': 'missing', 'instrument': 'none'}]
(work/'bad-fixture.json').write_text(json.dumps(bad), encoding='utf-8')
refused = run(work/'bad-fixture.json', work/'bad-facts.json')
assert refused.returncode != 0 and 'Crafted exit OOT Kokiri Forest -> OOT Nowhere is not in the world' in refused.stderr, \
    refused.stderr
print('PASS the pinned validateAgeTemple run reproduces', FACTS.name, 'and refuses a crafted exit it cannot apply')
