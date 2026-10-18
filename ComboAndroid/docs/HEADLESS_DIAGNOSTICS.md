# Private combined-fill diagnostics

`COMBO_RANDO_DIAGNOSTICS` defaults to OFF. Enable it only in a separate diagnostic
source/build, preserving the exact input recipe and original artifact identities.
It does not enable entrance capabilities. Its OoT library carries the existing
non-release marker, so the unchanged APK verifier rejects it for release.

The headless generator accepts `--diagnostics-dir NEW_DIRECTORY` only in a
diagnostic build, for one generated seed and no playthrough mode. The directory
must not already exist. Ordinary builds reject the flag. Generation and retry
budgets are unchanged; no extra oracle searches, shuffles or RNG draws are made.

For each built-in attempt, the tool writes generated metadata:

- At the dump boundary: the existing OoT dump, MM fixed placements, forced
  placements and resolved master/starting world. Current native modules prepare
  entrances before confined placements; their file is named `after-confined`.
  Older modules retain the `before-entrances` name and post-dump shuffle.
- After native entrance shuffle: the actual entrance override table.
- At each completed final sphere validation: collected inventory, all placements
  with locked/reachable/first-credited-sphere fields, goal state, query counts,
  RNG state and the last existing native OoT query's input/items/region age bits.

The native snapshot is captured immediately after the existing reachability
search. Its getter only serializes that saved snapshot. Native observation can
only be enabled after headless initialization. No save container, ROM, archive
or private app configuration is exported. Trace files are not importable seeds.

The normal fixed-point sphere map only describes its candidate checks; fixed
placements may be absent. Diagnostic first-credit spheres are therefore kept
separately and never change normal generation's sphere map or results.

Run `fill_diagnostics_tests.py` with the usual `--source`, `--json-include` and
`--build-dir` arguments. It compiles both macro configurations and compares real
combined-fill successes/failures, byte results and query counts with observation
disabled/enabled. Native engine states and a real failed recipe still require a
separately identified ARM64 diagnostic execution.
