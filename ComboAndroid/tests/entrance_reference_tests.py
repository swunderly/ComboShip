"""Offline source and deterministic exporter checks; explicitly skipped without the pinned checkout."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
reference = os.environ.get('COMBO_OOTMM_REFERENCE')
if reference:
    reference = Path(reference)
else:
    reference = next((parent/'references/ootmm-stable-32.3' for parent in source.parents
                      if (parent/'references/ootmm-stable-32.3/.git').exists()), None)
if reference is None:
    print('SKIPPED: set COMBO_OOTMM_REFERENCE to the exact local OoTMM v32.3 checkout')
    sys.exit(0)

sys.path.insert(0, str(source/'tools/ootmm'))
import export_registry as registry
import export_grotto_context as context
import export_oot_wallmaster_sources as wallmasters

original, records = registry.read_registry(reference)
manifest = json.loads((source/'tools/ootmm/registry-source.json').read_text(encoding='utf-8'))
assert hashlib.sha256(original).hexdigest() == manifest['sourceSha256']
assert records == manifest['entrances']
runtime = registry.verified_blob(reference, context.RUNTIME_SOURCE)
expected = json.loads((source/'tools/ootmm/grotto-context-source.json').read_text(encoding='utf-8'))
assert json.loads(registry.json_bytes(context.parse_context(runtime, records))) == expected
wallmaster_blob = registry.verified_blob(reference, wallmasters.SOURCE)
wallmaster_expected = json.loads((source/wallmasters.OUTPUT).read_text(encoding='utf-8'))
assert wallmasters.parse_source(wallmaster_blob, records) == wallmaster_expected
outputs = ['combo/rando/entrances/Registry.h', 'combo/rando/entrances/GrottoMetadata.h',
           'combo/rando/entrances/LICENSE.OoTMM', 'tools/ootmm/registry-source.json',
           'tools/ootmm/grotto-context-source.json', wallmasters.OUTPUT,
           'ComboAndroid/tests/fixtures/ootmm-32.3-solver-contract.json']
attributes = subprocess.check_output(['git', '-C', str(source), '-c', 'core.autocrlf=true',
                                     'check-attr', 'eol', '--', *outputs], text=True, timeout=30)
assert len(attributes.splitlines()) == len(outputs)
assert all(line.endswith(': eol: lf') for line in attributes.splitlines())

def run(script, output, check=False, ref=reference):
    command = [sys.executable, str(source/'tools/ootmm'/script), '--reference', str(ref),
               '--output-root', str(output)] + (['--check'] if check else [])
    return subprocess.run(command, capture_output=True, text=True, timeout=60)

with tempfile.TemporaryDirectory(prefix='entrance-export-', dir=work) as scratch:
    scratch = Path(scratch)
    for script, target in [('export_registry.py', 'combo/rando/entrances/Registry.h'),
                           ('export_grotto_context.py', 'combo/rando/entrances/GrottoMetadata.h'),
                           ('export_solver_contract.py', 'ComboAndroid/tests/fixtures/ootmm-32.3-solver-contract.json'),
                           ('export_oot_wallmaster_sources.py', wallmasters.OUTPUT)]:
        result = run(script, source, check=True)
        assert result.returncode == 0, result.stdout + result.stderr
        result = run(script, scratch)
        assert result.returncode == 0, result.stdout + result.stderr
        assert (scratch/target).read_bytes() == (source/target).read_bytes()
        assert run(script, scratch, check=True).returncode == 0
        damaged = (scratch/target).read_bytes() + b'// deliberate stale output\n'
        (scratch/target).write_bytes(damaged)
        assert run(script, scratch, check=True).returncode != 0
        assert (scratch/target).read_bytes() == damaged  # --check is read-only on failure.
        before = {p.relative_to(scratch): p.read_bytes() for p in scratch.rglob('*') if p.is_file()}
        rejected = run(script, scratch, ref=source)  # this repo is not the required upstream pin
        assert rejected.returncode != 0 and 'Expected exact OoTMM v32.3 revision' in rejected.stderr
        assert before == {p.relative_to(scratch): p.read_bytes() for p in scratch.rglob('*') if p.is_file()}

# The intentionally restricted YAML parser fails closed on new syntax, fields and identities.
sample = b'A: { game: mm, id: 0x10000, type: grotto, reverse: B }\nB: { game: mm, id: 0x10100, type: grotto-exit, reverse: A }\n'
assert registry.parse_registry(sample)[0]['id'] == 0x10000
for invalid in [sample.replace(b'type: grotto,', b'newField: value, type: grotto,'),
                sample.replace(b'0x10100', b'0x10000'), sample.replace(b'reverse: A', b'reverse: B'),
                sample.replace(b'0x10000', b'0x80000000'), sample + sample.splitlines(keepends=True)[0]]:
    try:
        registry.parse_registry(invalid)
    except (ValueError, KeyError):
        pass
    else:
        raise AssertionError('Malformed registry input was accepted')
print('Canonical Git blobs, exact pin, read-only stale checks, deterministic exports and fail-closed parser verified.')
