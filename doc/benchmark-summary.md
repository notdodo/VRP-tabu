# Benchmark Summary

Experiments were stopped on 2026-06-02. This file records the consolidated current-code baseline and points to the raw artifacts.

## Current Baseline

Build command: `make release LTO=ON NATIVE=ON`

Raw results: `doc/benchmarks/2026-06-02-current-baseline/results.tsv`

Per-instance logs: `doc/benchmarks/2026-06-02-current-baseline/logs/`

Full experiment ledger: `doc/benchmark-results.md`

| Instance | Target | Best | Gap | Routes | Solver ms | Wall s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `E-n101-k14` | 1067 | 1078 | +11 | 14 | 29596 | 29 |
| `E-n101-k8` | 815 | 817 | +2 | 8 | 128007 | 128 |
| `E-n51-k5` | 521 | 521 | 0 | 5 | 15218 | 16 |
| `E-n76-k10` | 830 | 830 | 0 | 10 | 20154 | 20 |
| `E-n76-k14` | 1021 | 1022 | +1 | 14 | 26981 | 27 |
| `E-n76-k7` | 682 | 689 | +7 | 7 | 37430 | 38 |
| `E-n76-k8` | 735 | 738 | +3 | 8 | 40222 | 40 |

Total solver time: 297608 ms.

Total measured wall time: 298 s.

## Status

Solved instances: `E-n51-k5`, `E-n76-k10`.

Open gaps: `E-n101-k14` +11, `E-n101-k8` +2, `E-n76-k14` +1, `E-n76-k7` +7, `E-n76-k8` +3.

The current solver is faster than the earliest recorded full-set baselines, but the original objective is not complete because five instances still miss their known target.

## Previous Selected Run

The previous selected full benchmark is retained for comparison at `doc/benchmarks/2026-06-02-final-selected/results.tsv` with logs in `doc/benchmarks/2026-06-02-final-selected/logs/`.
