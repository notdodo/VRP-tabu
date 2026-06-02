# Benchmark Results

Default release configuration unless noted: `LTO=OFF`, `NATIVE=OFF`, `PROFILE=OFF`.

| Date | Change set | Instance | Target | Best cost | Routes | Solver time | Elapsed |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| 2026-06-01 | Removed sparse-route sweep, balanced cluster boundary pool | `E-n76-k14` | 1021 | 1035 | 14 | 20387 ms | 20.7544 s |
| 2026-06-01 | Removed sparse-route sweep, balanced cluster boundary pool | `E-n101-k14` | 1067 | 1077 | 14 | 93605 ms | 93.6731 s |
| 2026-06-01 | Wider tabu candidate list plus route-pool same-count acceptance | `E-n101-k8` | 815 | 817 | 8 | 380303 ms | 380.68 s |
| 2026-06-01 | Reversed segment orientation in strict local search plus tabu scan cleanup | `E-n76-k10` | 830 | 830 | 10 | 39329 ms | 39.33 s |
| 2026-06-01 | Reversed segment orientation in strict local search plus tabu scan cleanup | `E-n101-k8` | 815 | 817 | 8 | 273834 ms | 273.83 s |
| 2026-06-01 | Delayed tabu route-set materialization | `E-n101-k8` | 815 | 817 | 8 | 272471 ms | 272.47 s |
| 2026-06-01 | Delayed tabu route-set materialization | `E-n76-k10` | 830 | 830 | 10 | 38683 ms | 38.68 s |
| 2026-06-01 | Exact bounded three-route cluster repartition | `E-n76-k14` | 1021 | 1024 | 14 | 52324 ms | 52.32 s |
| 2026-06-01 | Exact bounded three-route cluster repartition | `E-n76-k10` | 830 | 830 | 10 | 38281 ms | 38.28 s |
| 2026-06-01 | Exact bounded three-route cluster repartition | `E-n76-k8` | 735 | 746 | 8 | 41300 ms | 41.30 s |
| 2026-06-01 | Exact bounded three-route cluster repartition | `E-n76-k7` | 682 | 689 | 7 | 65600 ms | 65.60 s |
| 2026-06-02 | Defer route reduction from initialization to VND | `E-n76-k14` | 1021 | 1024 | 14 | 51523 ms | 52 s |
| 2026-06-02 | Defer route reduction from initialization to VND | `E-n76-k10` | 830 | 830 | 10 | 37831 ms | 38 s |
| 2026-06-02 | Defer route reduction from initialization to VND | `E-n76-k8` | 735 | 746 | 8 | 40957 ms | 41 s |
| 2026-06-02 | Defer route reduction from initialization to VND | `E-n76-k7` | 682 | 689 | 7 | 64594 ms | 65 s |
| 2026-06-02 | Direct graph matrix indexes on customers | `E-n76-k14` | 1021 | 1024 | 14 | 16292 ms | 17 s |
| 2026-06-02 | Direct graph matrix indexes on customers | `E-n76-k10` | 830 | 830 | 10 | 13552 ms | 14 s |

## Full Set Benchmark

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-01 | `E-n101-k14` | 1067 | 1079 | +12 | 14 | 84200 ms | 84.55 s |
| 2026-06-01 | `E-n101-k8` | 815 | 817 | +2 | 8 | 347959 ms | 348.04 s |
| 2026-06-01 | `E-n51-k5` | 521 | 521 | 0 | 5 | 20962 ms | 21.05 s |
| 2026-06-01 | `E-n76-k10` | 830 | 845 | +15 | 10 | 26288 ms | 26.38 s |
| 2026-06-01 | `E-n76-k14` | 1021 | 1046 | +25 | 14 | 13149 ms | 13.25 s |
| 2026-06-01 | `E-n76-k7` | 682 | 689 | +7 | 7 | 85467 ms | 85.59 s |
| 2026-06-01 | `E-n76-k8` | 735 | 746 | +11 | 8 | 47710 ms | 47.80 s |

## Full Set Benchmark After Local-Search Segment Reversal

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-01 | `E-n101-k14` | 1067 | 1079 | +12 | 14 | 86496 ms | 86.50 s |
| 2026-06-01 | `E-n101-k8` | 815 | 817 | +2 | 8 | 273834 ms | 273.83 s |
| 2026-06-01 | `E-n51-k5` | 521 | 521 | 0 | 5 | 18993 ms | 18.99 s |
| 2026-06-01 | `E-n76-k10` | 830 | 830 | 0 | 10 | 39329 ms | 39.33 s |
| 2026-06-01 | `E-n76-k14` | 1021 | 1046 | +25 | 14 | 10770 ms | 10.77 s |
| 2026-06-01 | `E-n76-k7` | 682 | 689 | +7 | 7 | 67492 ms | 67.49 s |
| 2026-06-01 | `E-n76-k8` | 735 | 746 | +11 | 8 | 41251 ms | 41.25 s |

## Full Set Benchmark After Fast Rebuild Verification

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-01 | `E-n101-k14` | 1067 | 1079 | +12 | 14 | 85945 ms | 86 s |
| 2026-06-01 | `E-n101-k8` | 815 | 817 | +2 | 8 | 271924 ms | 272 s |
| 2026-06-01 | `E-n51-k5` | 521 | 521 | 0 | 5 | 18964 ms | 19 s |
| 2026-06-01 | `E-n76-k10` | 830 | 830 | 0 | 10 | 39039 ms | 39 s |
| 2026-06-01 | `E-n76-k14` | 1021 | 1046 | +25 | 14 | 10895 ms | 11 s |
| 2026-06-01 | `E-n76-k7` | 682 | 689 | +7 | 7 | 67719 ms | 68 s |
| 2026-06-01 | `E-n76-k8` | 735 | 746 | +11 | 8 | 41873 ms | 42 s |

## Focused Benchmark After Route-Pool Cost Pruning

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-01 | `E-n76-k14` | 1021 | 1024 | +3 | 14 | 52060 ms | 53 s |
| 2026-06-01 | `E-n76-k10` | 830 | 830 | 0 | 10 | 38620 ms | 38 s |
| 2026-06-01 | `E-n76-k8` | 735 | 746 | +11 | 8 | 41004 ms | 41 s |
| 2026-06-01 | `E-n76-k7` | 682 | 689 | +7 | 7 | 65971 ms | 66 s |

## Full Set Benchmark After Route-Pool Cost Pruning

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-01 | `E-n101-k14` | 1067 | 1079 | +12 | 14 | 84305 ms | 85 s |
| 2026-06-01 | `E-n101-k8` | 815 | 817 | +2 | 8 | 268068 ms | 268 s |
| 2026-06-01 | `E-n51-k5` | 521 | 521 | 0 | 5 | 18170 ms | 18 s |
| 2026-06-01 | `E-n76-k10` | 830 | 830 | 0 | 10 | 37894 ms | 38 s |
| 2026-06-01 | `E-n76-k14` | 1021 | 1024 | +3 | 14 | 51768 ms | 52 s |
| 2026-06-01 | `E-n76-k7` | 682 | 689 | +7 | 7 | 65366 ms | 65 s |
| 2026-06-01 | `E-n76-k8` | 735 | 746 | +11 | 8 | 40269 ms | 41 s |

## Full Set Benchmark Before Relaxed-Capacity Work

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-01 | `E-n101-k14` | 1067 | 1079 | +12 | 14 | 84047 ms | 84 s |
| 2026-06-01 | `E-n101-k8` | 815 | 817 | +2 | 8 | 297314 ms | 298 s |
| 2026-06-01 | `E-n51-k5` | 521 | 521 | 0 | 5 | 17974 ms | 18 s |
| 2026-06-01 | `E-n76-k10` | 830 | 830 | 0 | 10 | 38070 ms | 38 s |
| 2026-06-01 | `E-n76-k14` | 1021 | 1024 | +3 | 14 | 51586 ms | 52 s |
| 2026-06-01 | `E-n76-k7` | 682 | 689 | +7 | 7 | 65409 ms | 65 s |
| 2026-06-01 | `E-n76-k8` | 735 | 746 | +11 | 8 | 40562 ms | 41 s |

## Full Set Benchmark After Direct Graph Indexes

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1079 | +12 | 14 | 31706 ms | 32 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 79222 ms | 80 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 6274 ms | 6 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 13666 ms | 14 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1024 | +3 | 14 | 16437 ms | 16 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 19155 ms | 20 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 746 | +11 | 8 | 13339 ms | 13 s |

## Current Fast Baseline After Quad-Split Removal

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1079 | +12 | 14 | 31096 ms | 32 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 79049 ms | 79 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 6237 ms | 6 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 13890 ms | 14 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1024 | +3 | 14 | 16761 ms | 17 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 19140 ms | 19 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 746 | +11 | 8 | 13333 ms | 14 s |

## Selected Benchmark After Experiment Sweep

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1079 | +12 | 14 | 31436 ms | 31 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 80276 ms | 80 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 6328 ms | 6 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 13889 ms | 14 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1024 | +3 | 14 | 16717 ms | 17 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 19334 ms | 20 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 746 | +11 | 8 | 13494 ms | 13 s |

## Selected Benchmark After Tabu Job Chunking

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1079 | +12 | 14 | 31386 ms | 32 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 78543 ms | 78 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 6234 ms | 7 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 13468 ms | 13 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1024 | +3 | 14 | 16331 ms | 17 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 18930 ms | 19 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 746 | +11 | 8 | 13132 ms | 13 s |

## Selected Benchmark After Route-Dense Tabu Depth

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1078 | +11 | 14 | 36949 ms | 37 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 78213 ms | 78 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 6178 ms | 7 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 13393 ms | 13 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1022 | +1 | 14 | 18150 ms | 18 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 18927 ms | 20 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 746 | +11 | 8 | 13356 ms | 13 s |

## Selected Benchmark After Fast VND/Tenure Selection

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

Selected run: `/private/tmp/vrp-current-candidate-full-20260602-031006.tsv`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1078 | +11 | 14 | 14838 ms | 16 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 56212 ms | 56 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 4931 ms | 5 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 10565 ms | 11 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1022 | +1 | 14 | 14905 ms | 15 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 16435 ms | 16 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 746 | +11 | 8 | 13964 ms | 14 s |

## Full Benchmark After Seed Archive and Reversed 2-Opt*

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

Run: `/private/tmp/vrp-archive-rev2opt-full-20260602-034650.tsv`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1078 | +11 | 14 | 15365 ms | 16 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 55974 ms | 56 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 4872 ms | 5 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 10365 ms | 11 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1022 | +1 | 14 | 15062 ms | 15 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 15910 ms | 16 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 746 | +11 | 8 | 13311 ms | 13 s |

## Selected Full Benchmark After Additional Candidate Sweep

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

Run: `/private/tmp/vrp-selected-full-20260602-044217.tsv`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1078 | +11 | 14 | 15244 ms | 16 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 54671 ms | 54 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 4836 ms | 5 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 9482 ms | 10 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1022 | +1 | 14 | 13619 ms | 14 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 13884 ms | 14 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 746 | +11 | 8 | 10735 ms | 10 s |

## Selected Full Benchmark With Wider Exact Pair Split

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

Run: `/private/tmp/vrp-selected-pairsplit18-full-20260602-045249.tsv`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1078 | +11 | 14 | 15417 ms | 15 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 56098 ms | 56 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 4883 ms | 5 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 10017 ms | 11 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1022 | +1 | 14 | 14071 ms | 14 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 15078 ms | 15 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 742 | +7 | 8 | 14013 ms | 14 s |

## Final Selected Full Benchmark After Additional Sweep

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

Run: `/private/tmp/vrp-selected-final-full-20260602-060544.tsv`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1078 | +11 | 14 | 15578 ms | 16 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 57004 ms | 57 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 4990 ms | 5 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 10192 ms | 10 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1022 | +1 | 14 | 14841 ms | 15 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 16148 ms | 16 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 742 | +7 | 8 | 15202 ms | 16 s |

## Selected Full Benchmark With Fresh Tabu Restart

Build configuration: `Release`, `LTO=ON`, `NATIVE=ON`, `PROFILE=OFF`.

Run: `/private/tmp/vrp-fresh-tabu-restart-full-20260602-064456.tsv`.

| Date | Instance | Target | Initial cost | Initial routes | Best cost | Gap | Routes | Solver time | Elapsed |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2026-06-02 | `E-n101-k14` | 1067 | 1107 | 14 | 1078 | +11 | 14 | 22336 ms | 23 s |
| 2026-06-02 | `E-n101-k8` | 815 | 825 | 8 | 817 | +2 | 8 | 82509 ms | 82 s |
| 2026-06-02 | `E-n51-k5` | 521 | 528 | 5 | 521 | 0 | 5 | 10309 ms | 11 s |
| 2026-06-02 | `E-n76-k10` | 830 | 899 | 10 | 830 | 0 | 10 | 14823 ms | 15 s |
| 2026-06-02 | `E-n76-k14` | 1021 | 1046 | 15 | 1022 | +1 | 14 | 19263 ms | 19 s |
| 2026-06-02 | `E-n76-k7` | 682 | 692 | 7 | 689 | +7 | 7 | 27550 ms | 28 s |
| 2026-06-02 | `E-n76-k8` | 735 | 771 | 8 | 738 | +3 | 8 | 27202 ms | 27 s |

## Open Benchmark Gaps

| Date | Change set | Instance | Target | Observed state | Notes |
| --- | --- | --- | ---: | --- | --- |
| 2026-06-02 | Selected fresh tabu restart | `E-n101-k14` | 1067 | Best 1078, gap +11 | Still the largest remaining gap; the fresh incumbent restart preserved but did not improve it. |
| 2026-06-02 | Selected fresh tabu restart | `E-n101-k8` | 815 | Best 817, gap +2 | Long sparse case remains close but unchanged by the fresh incumbent restart. |
| 2026-06-02 | Selected fresh tabu restart | `E-n76-k14` | 1021 | Best 1022, gap +1 | Fragile dense case remains near-optimal and was preserved by the fresh incumbent restart. |
| 2026-06-02 | Selected fresh tabu restart | `E-n76-k7` | 682 | Best 689, gap +7 | Sparse medium case remains unchanged. |
| 2026-06-02 | Selected fresh tabu restart | `E-n76-k8` | 735 | Best 738, gap +3 | Improved from the previous selected 742 by resetting tabu memory once from the incumbent. |
| 2026-06-01 | Bounded route archive, arbitrary exchange, pair sweep, and route-cluster sweep | `E-n101-k8` | 815 | Initial solution 825, run killed before first partial | The first forced-deep VND round still had a remaining long path after the bounded cluster work. |
| 2026-06-01 | Deferred route-cluster TSP polishing to the best cheap boundary assignments | `E-n101-k8` | 815 | Best 825 after 5 diversification attempts | Completed by stop condition in 125629 ms / 125.99 s wall. The route-cluster stall is gone, but quality did not improve and the non-improving loop remains too slow. |
| 2026-06-01 | Persistent tabu memory plus route-boundary related-removal seeds | `E-n101-k8` | 815 | Best 819 after 11 diversification attempts | Completed by stop condition in 307361 ms / 307.72 s wall. Quality improved from 825, but the remaining 4-point gap and runtime are still open. |
| 2026-06-01 | Wider tabu candidate list plus route-pool same-count acceptance | `E-n101-k8` | 815 | Best 817 after 8 diversification attempts | Completed by stop condition in 380303 ms / 380.68 s wall. Quality improved by 2 points, but the remaining 2-point gap and slower runtime are still open. |

## Rejected Experiments

| Date | Experiment | Instance | Result | Reason |
| --- | --- | --- | ---: | --- |
| 2026-06-01 | Add one-for-one swap candidates inside tabu search | `E-n76-k14` | 1041 | Regressed from the accepted 1035 result and was reverted. |
| 2026-06-01 | Add forced boundary-pair reassignment as a diversification step | `E-n101-k8` | 825 in 146.71 s | Did not improve the 825 incumbent and slowed the stagnant path from the previous 125.99 s baseline, so it was reverted. |
| 2026-06-01 | Rank sweep partitions cheaply before 2-opt polishing | `E-n101-k8` | Initial 859, partial 839 before kill | Initialization was faster, but route quality regressed badly from the accepted 825 starting point, so it was reverted. |
| 2026-06-01 | Expand full route-cluster sweep to larger triples | `E-n101-k8` | 819 in 315.91 s | Same best result as boundary-seed LNS but slower than the 307.72 s baseline, so it was reverted. |
| 2026-06-01 | Remove the bounded boundary-pool cap from three-route cluster split | `E-n101-k8` | Partials 828, 825, 825, 825 before kill | Did not approach the accepted 819 incumbent and made the already expensive route-cluster neighborhood broader, so it was reverted. |
| 2026-06-01 | Branch route-pool exact cover on the most constrained uncovered customer | `E-n101-k8` | Killed during initialization | The dynamic and static constrained selectors both slowed route-pool recombination before search began, so the selector change was reverted. |
| 2026-06-01 | Add forced non-improving 2-opt* as a diversification step | `E-n101-k8` | Partials 829, 825, 825 before kill | The new diversification fired but only found a zero-cost tail exchange and did not improve the incumbent trajectory, so it was reverted. |
| 2026-06-01 | Add strict angular-sector ruin/recreate to deep VND | `E-n101-k8` | Partials 829, 825 before kill | The move was evaluated in deep VND but reported no improvement, adding runtime without changing the accepted trajectory, so it was reverted. |
| 2026-06-01 | Feed the route-pool recombiner with the top sweep-partition plans | `E-n101-k8` | 819 in 364.30 s | Regressed from the accepted 817 full-set fast benchmark and did not improve any other open Set-E gap, so the sweep-pool broadening was reverted. |
| 2026-06-01 | Add bounded string cross-exchange directly inside the tabu candidate walk | `E-n76-k10`, `E-n76-k7` | 846 in 40.54 s, 689 in 90.48 s | Helped `E-n76-k8` and `E-n76-k14`, but regressed `E-n76-k10` quality and slowed neutral cases too much, so the tabu-level cross-exchange was removed. |
| 2026-06-01 | Allow reversed Or-opt insertion inside tabu relocation candidates | `E-n76-k14` | 1055 in 13.37 s | Hit `E-n76-k10` but destabilized the tabu path badly on `E-n76-k14`, so reversal was kept only in strict local-search moves. |
| 2026-06-01 | Add strict three-route ejection-chain repair to deep VND | `E-n76-k10`, `E-n101-k14` | 830 in 38.93 s, 1079 in 85.74 s | Preserved quality but did not improve any tested open gap, so the extra neighborhood was removed rather than carrying dead complexity. |
| 2026-06-01 | Raise exact pair-split combined-customer bound from 14 to 16 | `E-n76-k7`, `E-n76-k8`, `E-n101-k8` | 689 in 67.41 s, 746 in 44.68 s, 817 in 272.32 s | Did not improve any affected route-membership gap and slowed `E-n76-k8`, so the wider exact split bound was reverted. |
| 2026-06-01 | Add pair-sweep fragments from nearby route pairs into archive recombination | `E-n76-k8`, `E-n76-k7`, `E-n101-k8` | 746 in 37.18 s, 689 in 73.49 s, 817 in 291.25 s | The exact-cover pool did not assemble a better solution and slowed the long case, so the fragment-pool expansion was reverted. |
| 2026-06-01 | Scale three-route cluster refinement budgets with cluster and boundary-pool size | `E-n76-k14`, `E-n76-k8`, `E-n76-k7` | 1046 in 10.95 s, 746 in 44.83 s, 689 in 80.58 s | No quality improvement and slower on the longer `E-n76` gaps, so the larger refinement budget was reverted. |
| 2026-06-01 | Add strict group-size-2 cyclic exchange to deep VND | `E-n76-k14`, `E-n76-k8`, `E-n76-k7` | 1046 in 25.61 s, 746 in 152.81 s, killed at 692 after 28 s | Did not improve quality and made medium cases much slower, so the extra cyclic exchange was reverted. |
| 2026-06-01 | Adaptive-memory recombination from non-overlapping archived routes plus regret repair | `E-n76-k14`, `E-n76-k8`, `E-n76-k7` | 1046 in 17.07 s, 746 in 64.61 s, killed before completion | The partial route-pool repair added runtime without improving tested gaps, so it was reverted. |
| 2026-06-01 | Increase stagnant-round patience from 5 to 8 | `E-n76-k14`, `E-n76-k8`, `E-n76-k7` | 1046 in 13.23 s, 746 in 51.94 s, killed before completion | Extra rounds did not improve tested gaps and only increased runtime, so the stop condition was restored. |
| 2026-06-01 | Add demand-aware variants to Clarke-Wright savings initialization | `E-n76-k14`, `E-n76-k8`, `E-n76-k7` | 1046 in 11.66 s, 746 in 43.83 s, 689 in 69.44 s | The extra deterministic initializers did not improve tested gaps, so the savings scoring was restored. |
| 2026-06-01 | Score all exchange candidates with independent insertion plus per-candidate TSP polishing | `E-n76-k14`, `E-n76-k8`, `E-n76-k7` | 1033 in 53.30 s, killed at 771 after 295 s, killed at 692 after 35 s | The scoring fixed a real neighborhood blind spot and improved `E-n76-k14`, but the unbounded hot-loop TSP path was too slow and destabilized other cases, so it was replaced by a bounded strict multi-customer variant. |
| 2026-06-01 | Limit independent insertion scoring to shallow `1-2`, `2-1`, and `2-2` exchanges | `E-n76-k14`, `E-n76-k8`, `E-n76-k7`, `E-n76-k10` | 1033 in 14.87 s, 746 in 52.60 s, 689 in 72.61 s, 845 in 26.26 s | Preserved the `E-n76-k14` gain, but regressed the solved `E-n76-k10` case from 830 to 845, so the exchange scoring change was reverted. |
| 2026-06-01 | Add same-count route recycle LNS with boundary buffer and beam rebuild | `E-n76-k14`, `E-n76-k10`, `E-n76-k8`, `E-n76-k7` | 1046 in 10.89 s, 830 in 39.44 s, 746 in 42.43 s, 689 in 68.13 s | Safe but neutral, so the recycle operator was removed rather than carrying dead complexity. |
| 2026-06-01 | Widen route recycle beam to route-shape seed limit | `E-n76-k14`, `E-n76-k10`, `E-n76-k8`, `E-n76-k7` | 1046 in 14.21 s, 845 in 21.89 s, killed at 771, killed before completion | Did not improve the target gap and regressed solved `E-n76-k10`, so the wider recycle operator was removed. |
| 2026-06-01 | Add independent exchange as a late deep-VND fallback | `E-n76-k14`, `E-n76-k10`, `E-n76-k8`, `E-n76-k7` | 1046 in 11.61 s, 830 in 41.47 s, killed before completion, killed before completion | Safe on solved `E-n76-k10` but neutral on the target gap, so the late fallback was removed. |
| 2026-06-01 | Increase stagnant-round patience from 5 to 8 after exact cluster split | `E-n76-k14` | 1024 in 57.29 s | No quality improvement over the exact-cluster result and slower, so the patience constant was restored. |
| 2026-06-01 | Widen exact three-route cluster cap beyond the route-shape budget | `E-n76-k14` | 1032 in 74.14 s | Larger exact clusters changed the trajectory for the worse and slowed the run, so the wider cap was reverted. |
| 2026-06-01 | Use exact three-route cluster split as a forced diversification move | `E-n76-k14` | 1032 in 25.88 s | The non-improving forced cluster move displaced the better 1024 trajectory, so it was removed. |
| 2026-06-01 | Scale boundary-pair pool to about two thirds of average route size | `E-n76-k8`, `E-n76-k7` | 746 in 41.13 s, 689 in 66.71 s | The larger boundary pool did not improve dense-route gaps and slightly slowed `E-n76-k7`, so the profile formula was restored. |
| 2026-06-01 | Feed exact/boundary route-cluster candidates into archive recombination | `E-n76-k14`, `E-n76-k10` | 1035 in 17.62 s with in-step recombination; 833 in 36.83 s with passive archive | In-step recombination displaced the accepted 1024 trajectory, and passive archival regressed solved `E-n76-k10` from 830, so the candidate feed was removed. |
| 2026-06-01 | Restart from stored alternative Clarke-Wright/sweep initial solutions after stagnation | `E-n76-k14` | 1038 in 30.43 s | The deterministic multi-start restart left the accepted 1024 basin and produced a worse incumbent, so the restart mechanism was removed. |
| 2026-06-01 | Widen tabu nearest-neighbor candidate spread from `sqrt(n)/2` to `sqrt(n)` | `E-n76-k14` | 1039 in 16.25 s | A broader tabu candidate list changed the early trajectory away from the accepted 1024 solution, so the spread was restored. |
| 2026-06-01 | Initialize tabu-call best fitness from the incumbent instead of zero | `E-n76-k14` | 1056 in 16.17 s | Although this made aspiration semantics cleaner, it removed the exploratory first-candidate behavior the current search depends on and badly regressed quality, so it was reverted. |
| 2026-06-01 | Run full deep VND on every opt pass instead of only after forced perturbation | `E-n76-k14` | 1024 in 52.27 s | Quality and runtime were effectively neutral versus the accepted route-pool-pruned result, so the control flow was restored. |
| 2026-06-01 | Add bounded relaxed-capacity tabu search during stagnation | `E-n76-k14`, `E-n76-k10`, `E-n101-k8` | 1038/845 with direct relaxed return; 1024/830/817 with global-best-only return | Allowing relaxed tabu to return local improvements displaced good basins and regressed solved `E-n76-k10`; the safe global-best-only variant was quality-neutral and added complexity, so the relaxed path was removed. |
| 2026-06-01 | Strictly polish every initializer seed before route-pool recombination | `E-n76-k14` | 1044 in 56.13 s | Extra seed polishing changed the initial route pool away from the accepted 1024 basin, so the initializer was restored to the lighter accepted path. |
| 2026-06-01 | Rank route-pool candidates by cost per served customer before exact-cover truncation | `E-n76-k14` | 1044 in 51.37 s | Normalized route ranking admitted different route-pool candidates and displaced the accepted 1024 trajectory, so raw-cost ordering was restored. |
| 2026-06-01 | Select route-cluster triples with a bounded closest/highest-cost hybrid list | `E-n76-k14` | 1044 in 48.88 s | Replacing the closest-triple prefix with a hybrid selection displaced the accepted exact-cluster trajectory, so closest-first cluster selection was restored. |
| 2026-06-01 | Defer TSP polishing for top two-route boundary reassignment candidates | `E-n76-k8`, `E-n76-k10` | 746 in 46.29 s, 847 in 61.90 s | The bounded polish did not improve the dense open gap and regressed solved `E-n76-k10`, so the pair boundary split was restored to cheap greedy scoring. |
| 2026-06-01 | Add strict related ruin/recreate that requires changed route membership | `E-n76-k14` | 1044 in 49.28 s before exact cluster; 1044 in 51.22 s as last deep fallback | The strict changed-route repair disturbed the accepted exact-cluster trajectory in both placements and did not improve the small open gap, so it was removed. |
| 2026-06-02 | Add late strict non-contiguous pair relocation between routes | `E-n76-k14` | 1044 in 50.30 s | The generic move filled a real neighborhood gap, but as a late deep fallback it still displaced the accepted 1024 trajectory, so it was removed along with the stale header declaration. |
| 2026-06-02 | Select the best existing deep-VND move from a shared snapshot | `E-n76-k14`, `E-n76-k8`, `E-n101-k8` | 1024 in 17.16 s, 746 in 14.42 s, 817 in 79.09 s | Preserved quality but did not improve any canary and slightly slowed medium cases, so the first-improvement deep VND order was restored. |
| 2026-06-02 | Add route-boundary removal sets to related ruin/recreate | `E-n76-k14` | 1044 in 4.81 s | Boundary sets changed the early related-LNS trajectory away from the accepted 1024 basin, so the boundary candidates were removed and the nearest-only removal construction was restored. |
| 2026-06-02 | Add late four-route sweep split | `E-n76-k14`, `E-n76-k8`, `E-n101-k8` | 1024 in 16.12 s, 746 in 13.68 s, 817 in 90.22 s | Preserved quality on canaries but did not improve any gap and slowed the long `E-n101-k8` case, so the neighborhood was removed. |
| 2026-06-02 | Widen related beam reconstruction from 4 to 8 states | `E-n76-k8`, `E-n76-k10` | 746 in 17.66 s, 830 in 18.43 s | Preserved quality but did not improve the open `E-n76-k8` gap and slowed the solved guardrail, so the beam width was restored. |
| 2026-06-02 | Polish three-route sweep split candidates with exact small-route TSP | `E-n76-k8`, `E-n76-k14`, `E-n76-k10` | 746 in 17.34 s, 1026 in 19.52 s, 830 in 16.75 s | Did not improve `E-n76-k8`, regressed `E-n76-k14` from 1024 to 1026, and slowed the guardrail, so the extra polishing was removed. |
| 2026-06-02 | Archive route memberships after each successful VND step | `E-n76-k8`, `E-n76-k14`, `E-n76-k10`, `E-n51-k5` | 746 in 15.23 s, 1024 in 19.49 s, 833 in 14.76 s, 521 in 7.44 s | Enriched recombination material but did not improve open canaries and regressed solved `E-n76-k10`, so intermediate archiving was removed. |
| 2026-06-02 | Carry one near-best non-improving state across an extra outer cycle | `E-n76-k8`, `E-n76-k10`, `E-n76-k14`, `E-n51-k5` | 746 in 15.64 s, 830 in 15.79 s, 1038 in 7.20 s, 521 in 6.97 s | Preserved solved guardrails but displaced the accepted `E-n76-k14` trajectory badly, so immediate incumbent restore was restored. |
| 2026-06-02 | Evaluate every generated three-route cluster candidate | `E-n76-k8`, `E-n76-k14`, `E-n76-k10` | 746 in 14.27 s, 1029 in 14.63 s, 830 in 14.02 s | Did not improve `E-n76-k8` and regressed `E-n76-k14`, so the bounded closest-cluster cap was restored. |
| 2026-06-02 | Reuse one tabu thread pool across iterations | `E-n51-k5`, `E-n76-k10`, `E-n76-k8`, `E-n76-k14`, `E-n101-k8` | 521 in 6.79 s, 830 in 14.01 s, 746 in 14.08 s, 1024 in 17.50 s, 817 in 84.28 s | Preserved quality but did not improve any gap and slowed the long `E-n101-k8` case, so the per-iteration pool lifecycle was restored. |
| 2026-06-02 | Broaden Clarke-Wright savings lambda range to `0.1..3.2` | `E-n76-k8`, `E-n76-k14`, `E-n76-k10`, `E-n101-k14` | 746 in 13.39 s, 1030 in 5.47 s, 830 in 13.78 s, 1079 in 31.95 s | Added generic initializer diversity but selected a poor 14-route basin for `E-n76-k14`, so the accepted lambda range was restored. |
| 2026-06-02 | Try reversed insertion order for non-contiguous exchange groups | `E-n76-k14`, `E-n76-k10` | 1033 in 10 s, 845 in 21 s | Expanded a real orientation choice in `OptExchange`, but it destabilized the accepted `E-n76-k14` path and regressed solved `E-n76-k10`, so route-order exchange insertion was restored. |
| 2026-06-02 | Use a bounded beam repair while eliminating one route | `E-n76-k7`, `E-n76-k10`, `E-n76-k14`, `E-n51-k5` | 689 in 20 s, 830 in 14 s, 1024 in 16 s, 521 in 6 s | Preserved guardrails but did not remove the extra `E-n76-k7` route or improve any open gap, so the additional repair branch was removed. |
| 2026-06-02 | Preserve equal-assessment tabu diversification alternatives | `E-n76-k14`, `E-n76-k8`, `E-n101-k14`, `E-n76-k10` | 1036 in 5 s, 746 in 12 s, 1089 in 15 s, 845 in 7 s | Keeping more equal-cost non-best candidates changed the escape path for the worse and regressed solved `E-n76-k10`, so the assessment-only diversification memory was restored. |
| 2026-06-02 | Try a speculative forced-diversification portfolio before committing a perturbation | `E-n76-k14`, `E-n76-k8`, `E-n101-k14`, `E-n76-k10` | 1030 in 18 s, 746 in 13 s, 1083 in 17 s, 841 in 9 s | Running short VND branches before selecting a forced perturbation looked safer, but still displaced the accepted basin and regressed the solved guardrail, so the simple ranked perturbation switch was restored. |
| 2026-06-02 | Add route-order concatenations to two-route sweep split candidates | `E-n76-k8`, `E-n76-k7`, `E-n76-k14`, `E-n76-k10` | 746 in 14 s, 689 in 19 s, 1029 in 13 s, 845 in 9 s | The extra generic split orders did not improve open pair gaps and changed the accepted path for fragile and solved guardrails, so angular-only pair sweep splitting was restored. |
| 2026-06-02 | Use dense `Customer::graphIndex` route lookup in tabu candidate generation | `E-n101-k14`, `E-n101-k8`, `E-n51-k5`, `E-n76-k10`, `E-n76-k14`, `E-n76-k7`, `E-n76-k8` | 1079/817/521/830/1024/689/746 in 178377 ms total | Quality was preserved, but total runtime was slightly slower than chunking alone and the hot-path lookup code became more complex, so the dense lookup was reverted. |
| 2026-06-02 | Increase route-shape segment exchange/relocation cap from 4 to 5 | `E-n76-k7`, `E-n76-k8` | 689 in 19 s, 747 in 13 s | Longer contiguous segments did not improve `E-n76-k7` and regressed `E-n76-k8`, so the accepted segment profile was restored. |
| 2026-06-02 | Increase route-dense tabu depth from `customers` to `1.5 * customers` | `E-n76-k14` | 1024 in 15 s | More tabu depth for route-dense solutions displaced the accepted 1022 trajectory, so the route-dense schedule was restored to `customers` iterations. |
| 2026-06-02 | Archive broad Clarke-Wright lambda seeds without selecting them initially | `E-n76-k14`, `E-n101-k8`, `E-n101-k14`, `E-n76-k10` | 1022 in 18 s, 817 in 78 s, 1084 in 19 s, 830 in 13 s | Archive-only initializer diversity did not improve the close gaps and regressed `E-n101-k14`, so broad lambda archive seeding was removed. |
| 2026-06-02 | Enable deep tabu depth for both sparse and dense route shapes | `E-n101-k8`, `E-n76-k8` | 819 in 77 s, 746 in 10 s | Sparse-route deep tabu regressed the long sparse `E-n101-k8` gap from 817 to 819, so only the accepted dense-route gate was kept. |
| 2026-06-02 | Refine more two-route pair-sweep candidates | `E-n76-k8`, `E-n76-k7`, `E-n101-k8`, `E-n76-k10`, `E-n76-k14` | 746 in 10 s, 689 in 17 s, 817 in 59 s, 830 in 12 s, 1022 in 15 s | Broader pair-sweep refinement was quality-neutral and slower on guardrails, so the top-4 refinement cap was restored. |
| 2026-06-02 | Run exact pair split before approximate boundary/sweep/cluster splits | `E-n76-k14`, `E-n101-k14`, `E-n76-k10` | 1038 in 5 s, 1078 in 20 s, 830 in 10 s | Exact-pair-first badly displaced the fragile `E-n76-k14` trajectory, so the accepted VND order was restored. |
| 2026-06-02 | Restart tabu diversification from the middle retained non-best candidate | `E-n76-k14`, `E-n101-k14`, `E-n76-k10` | 1042 in 5 s, 1079 in 24 s, 845 in 4 s | Gentler fallback diversification regressed both fragile and solved guardrail cases, so the original most-divergent fallback was restored. |
| 2026-06-02 | Rank route-pool exact-cover choices by cost per customer | `E-n101-k14`, `E-n101-k8`, `E-n76-k10`, `E-n76-k14` | 1078 in 17 s, 817 in 64 s, 830 in 12 s, 1022 in 17 s | Normalized route-pool scoring preserved quality but was slower than raw-cost ordering in the selected full run, so it was removed. |
| 2026-06-02 | Evaluate tabu Or-opt segments for half of the neighbor list | `E-n101-k8`, `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14` | 817 in 56 s, 746 in 11 s, 689 in 16 s, 830 in 15 s, 1039 in 4 s | Wider segment candidates did not improve open gaps and regressed `E-n76-k14`, so the one-third segment-neighbor scope was restored. |
| 2026-06-02 | Evaluate `3 * route_count` route-cluster candidates | `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14` | 746 in 11 s, 689 in 13 s, 830 in 10 s, 1029 in 9 s | More cluster triples were quality-neutral on open medium cases and regressed the fragile dense case, so the `2 * route_count` cap was restored. |
| 2026-06-02 | Raise main exact fixed-route TSP polishing limit from 14 to 16 customers | `E-n101-k8`, `E-n76-k8`, `E-n76-k10`, `E-n76-k14` | 817 in 60 s, 746 in 11 s, 830 in 10 s, 1022 in 14 s | Exact polishing of longer routes did not close the `E-n101-k8` +2 gap and added runtime, so the 14-customer limit was restored. |
| 2026-06-02 | Remove extra tabu diversification tenure entirely | `E-n76-k14`, `E-n101-k14`, `E-n76-k10` | 1022 in 13 s, 1078 in 16 s, 845 in 5 s | Removing the extra tenure preserved fragile cases but regressed solved `E-n76-k10`; the selected 1x tenure clamp was restored. |
| 2026-06-02 | Add bounded one-for-one swap candidates to tabu search | `E-n76-k10`, `E-n76-k14` | 845 in 7 s, 1036 in 6 s | Tabu-level swaps gave the search too much destructive freedom and immediately regressed both the solved guardrail and the fragile dense case, so the swap candidate was removed. |
| 2026-06-02 | Add angular sweep petal routes to the initial exact-cover pool | `E-n76-k8`, `E-n76-k14`, `E-n76-k10`, `E-n101-k8`, `E-n101-k14` | 746 in 10 s, 1022 in 13 s, 830 in 10 s, 817 in 56 s, 1083 in 10 s | Petal candidates were neutral on medium gaps but changed `E-n101-k14` to a worse basin, so the petal expansion was removed. |
| 2026-06-02 | Double route-pool exact-cover node budget | `E-n101-k8`, `E-n76-k8`, `E-n76-k14`, `E-n76-k10` | 817 in 57 s, 746 in 12 s, 1022 in 15 s, 830 in 12 s | More exact-cover search did not improve route selection and slowed canaries, so the original budget multiplier was restored. |
| 2026-06-02 | Carry one near-best non-improving solution for an extra outer cycle | `E-n76-k10`, `E-n76-k14` | 830 in 9 s, 1038 in 4 s | Tight cost-drift exploration preserved the solved guardrail but displaced the fragile dense case, so immediate incumbent restore was kept. |
| 2026-06-02 | Run `2-opt*` earlier in the strict VND order | `E-n76-k14`, `E-n76-k10`, `E-n101-k8`, `E-n76-k8`, `E-n76-k7`, `E-n101-k14` | 1022 in 13 s, 830 in 10 s, 817 in 55 s, 746 in 11 s, 689 in 13 s, 1078 in 17 s | Earlier `2-opt*` priority was quality-neutral, so the accepted VND ordering was restored. |
| 2026-06-02 | Increase stagnant-round patience from 5 to 8 on the current fast solver | `E-n101-k8`, `E-n76-k8`, `E-n76-k14` | 817 in 72 s, 746 in 15 s, 1022 in 16 s | Extra patience still did not improve open gaps and added runtime, so the stop condition stayed at 5 stagnant rounds. |
| 2026-06-02 | Add nearest-neighbor giant-tour split initializer candidates | `E-n76-k8`, `E-n101-k8`, `E-n76-k14`, `E-n76-k10`, `E-n101-k14`, `E-n76-k7` | 746 in 11 s, 817 in 55 s, 1022 in 14 s, 830 in 10 s, 1078 in 16 s, 689 in 15 s | Route-first/cluster-second nearest splits were quality-neutral across the open gaps and guardrails, so the extra initializer was removed. |
| 2026-06-02 | Select the best deep-VND move from a shared snapshot | `E-n76-k14`, `E-n76-k10`, `E-n76-k8`, `E-n76-k7` | 1022 in 13 s, 845 in 5 s, 746 in 12 s, 689 in 14 s | The shared-snapshot policy preserved the fragile dense result but regressed solved `E-n76-k10`, so first-improvement deep VND was restored. |
| 2026-06-02 | Archive feasible exact/sweep/boundary route-cluster candidates for later recombination | `E-n76-k14`, `E-n76-k10`, `E-n76-k8`, `E-n76-k7` | 1022 in 16 s, 833 in 9 s, 746 in 11 s, 689 in 13 s | Candidate archiving did not improve open route-membership gaps and regressed solved `E-n76-k10`, so cluster-split archival was removed. |
| 2026-06-02 | Add route-boundary removal sets to strict related LNS | `E-n76-k14`, `E-n76-k10`, `E-n76-k8`, `E-n76-k7` | 1044 in 4 s, 833 in 8 s, 742 in 15 s, 689 in 14 s | Boundary sets improved `E-n76-k8` but badly displaced the fragile `E-n76-k14` trajectory and regressed the solved guardrail, so strict related LNS was restored. |
| 2026-06-02 | Restrict route-boundary removal sets to forced perturbation only | `E-n76-k14`, `E-n76-k10`, `E-n76-k8`, `E-n76-k7` | 1044 in 4 s, 833 in 8 s, 746 in 10 s, 689 in 15 s | Moving boundary sets to perturbation did not preserve the guardrails or improve the open gaps, so the perturbation-only variant was removed. |
| 2026-06-02 | Raise exact pair-split cap from 18 to 20 combined customers | `E-n76-k8` | 742 in 12 s | The wider cap did not improve beyond the selected 18-customer cap, so 18 was kept as the cheaper effective bound. |
| 2026-06-02 | Evaluate `3 * route_count` exact pair-split candidates | `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14`, `E-n101-k8` | 742 in 11 s, 689 in 13 s, 830 in 9 s, 1022 in 14 s, 817 in 56 s | Broader exact-pair candidate coverage preserved quality but did not improve any remaining gap, so the cheaper `2 * route_count` cap was restored. |
| 2026-06-02 | Evaluate all eligible exact pair-split candidates | `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14`, `E-n101-k8` | 742 in 12 s, 689 in 14 s, 830 in 10 s, 1022 in 14 s, 817 in 58 s | Exhausting all eligible exact pairs was still quality-neutral and slightly slower, so bounded closest/high-cost pair selection was kept. |
| 2026-06-02 | Retain three archived routes per customer for route-pool recombination | `E-n101-k14`, `E-n101-k8`, `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14` | 1078 in 16 s, 817 in 55 s, 742 in 12 s, 689 in 14 s, 830 in 9 s, 1022 in 14 s | A larger route archive preserved quality but did not close any gap and slightly slowed medium cases, so the selected archive limit was restored. |
| 2026-06-02 | Raise pair-sweep candidate exact TSP polishing limit from 14 to 18 customers | `E-n101-k8`, `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14`, `E-n101-k14` | 817 in 63 s, 742 in 12 s, 689 in 15 s, 830 in 9 s, 1022 in 15 s, 1078 in 17 s | Larger exact polishing inside pair-sweep candidates was quality-neutral and slowed the long sparse case, so the 14-customer polishing limit was restored. |
| 2026-06-02 | Add depot-neighborhood anchors to tabu candidate generation | `E-n101-k14`, `E-n101-k8`, `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14` | 1083 in 12 s, 822 in 43 s, 742 in 13 s, 689 in 21 s, 832 in 9 s, 1024 in 14 s | Depot anchors added broad relocation pressure but regressed both long gaps and solved/fragile guardrails, so customer-anchored candidate generation was restored. |
| 2026-06-02 | Generate non-improving exact pair-split fragments for route-pool recombination | `E-n101-k14`, `E-n101-k8`, `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14` | 1083 in 9 s, 817 in 54 s, 747 in 11 s, 689 in 14 s, 835 in 8 s, 1033 in 6 s | Temporary route fragments made recombination noisier and regressed solved/guardrail cases, so strict pair split was restored and only accepted archive routes feed recombination. |
| 2026-06-02 | Add late small-route refill with nearby same-size replacement sets | `E-n101-k14`, `E-n76-k8`, `E-n76-k10`, `E-n76-k14`, `E-n101-k8`, `E-n76-k7` | 1078 in 16 s, 742 in 12 s, 830 in 9 s, 1022 in 14 s, 817 in 55 s, 689 in 15 s | The move preserved all canary results but did not close any gap, so it was removed instead of carrying a neutral late-neighborhood fallback. |
| 2026-06-02 | Base tabu diversification tenure on accepted/average rejected pressure | `E-n101-k14`, `E-n101-k8`, `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14` | 1078 in 16 s, 817 in 56 s, 742 in 12 s, 689 in 15 s, 845 in 5 s, 1022 in 14 s | Pressure-based tenure was neutral on open gaps but regressed solved `E-n76-k10`, so the selected raw rejected-pressure tenure was restored. |
| 2026-06-02 | Use intermediate tabu depth for route-sparse solutions | `E-n101-k8`, `E-n76-k7`, `E-n76-k8`, `E-n76-k10`, `E-n76-k14`, `E-n101-k14` | 822 in 45 s, 689 in 23 s, 742 in 13 s, 830 in 10 s, 1022 in 14 s, 1078 in 16 s | Sparse-depth search repeated the earlier quality regression on `E-n101-k8` and slowed `E-n76-k7`, so the selected sparse schedule stayed at half-depth. |
| 2026-06-02 | Keep archive recombination passive instead of jumping active search state | `E-n101-k14`, `E-n101-k8`, `E-n76-k8`, `E-n76-k7`, `E-n76-k10`, `E-n76-k14` | 1078 in 16 s, 817 in 57 s, 742 in 12 s, 689 in 15 s, 830 in 11 s, 1022 in 16 s | Passive recombination preserved quality but slightly slowed long/fragile cases and added subtle state semantics, so active recombination was restored. |
| 2026-06-02 | Refill tiny routes from compact global customer groups | `E-n101-k14`, `E-n76-k10`, `E-n76-k14`, `E-n101-k8`, `E-n76-k8`, `E-n76-k7` | 1078 in 16 s, 830 in 10 s, 1022 in 13 s, 817 in 56 s, 742 in 12 s, 689 in 15 s | Global compact replacements preserved guardrails but did not close any open gap, so the late tiny-route refill operator was removed. |
| 2026-06-02 | Add underfilled-route supplements to route-cluster candidate selection | `E-n101-k14`, `E-n76-k8`, `E-n76-k10`, `E-n76-k14`, `E-n76-k7` | 1078, 738, 830, 1025, 689 | Added generic cluster coverage for below-average route sizes, but it regressed fragile `E-n76-k14` from 1022 to 1025, so the closest-triple cap was restored. |
| 2026-06-02 | Preserve load-shape diversity inside related beam repair | `E-n76-k10`, `E-n76-k14`, `E-n101-k14`, `E-n76-k8`, `E-n101-k8` | 830, 1022, 1083, 738, killed after reaching 817 | Beam diversity preserved medium guardrails but regressed `E-n101-k14` from 1078 to 1083, so cheapest-state beam pruning was restored. |
| 2026-06-02 | Prune route archive by overrepresented load buckets | `E-n101-k14`, `E-n101-k8`, `E-n76-k10`, `E-n76-k14` | 1078, killed after reaching 817, 830, 1022 | Load-bucket retention was quality-neutral and added archive complexity, so raw cost-per-customer pruning was restored. |
| 2026-06-02 | Add second fresh restart with related-cluster perturbation | `E-n76-k8`, `E-n101-k14`, `E-n101-k8`, `E-n76-k10` | 738 in 70 s, 1078 in 69 s, killed after reaching 817, 830 in 31 s | The second restart did not improve quality and roughly doubled affected runtimes, so the selected single fresh restart was restored. |
| 2026-06-02 | Add forced cyclic exchange as a stagnation perturbation | `E-n76-k14`, `E-n101-k14`, `E-n76-k8`, `E-n76-k10` | 1032, killed at 1078, killed at 742, 830 | The forced 3-route cycle was too disruptive for the fragile dense case, so cyclic exchange remains strict-only. |
| 2026-06-02 | Evaluate all eligible exact pair splits on `E-n101-k14` | `E-n101-k14` | 1078 in 23 s | Exhausting eligible exact pairs did not break the 1078 plateau, so the bounded closest/high-cost pair cap was restored. |
| 2026-06-02 | Evaluate all generated route-cluster triples on `E-n101-k14` | `E-n101-k14` | 1078 in 23 s | Exhausting generated cluster triples did not improve the long dense gap, so the `2 * route_count` cluster cap was restored. |
| 2026-06-02 | Add bounded tabu-level `2-opt*` route-pair candidates | `E-n76-k10`, `E-n76-k14` | 835 in 13 s, 1039 in 22 s | Tabu-level tail exchanges destabilized solved and fragile guardrails, so the move family was removed. |
| 2026-06-02 | Polish near-incumbent route-pool recombinations | `E-n76-k10` | 848 in 10 s | Near-cover recombination found locally better intermediate incumbents but displaced the trajectory that reaches the 830 optimum, so the near-candidate path was removed. |
| 2026-06-02 | Initialize tabu aspiration from current route fitness | `E-n76-k10` | 840 in 22 s | The change looked like a correctness cleanup, but it removed the current search trajectory that reaches the solved guardrail, so the zero-sentinel aspiration behavior was restored for now. |
| 2026-06-02 | Add final forced-diversification portfolio from the incumbent | `E-n76-k10` | 840 in 23 s | End-stage perturbation still changed the fragile solved trajectory and added runtime, so the final portfolio was removed. |
| 2026-06-02 | Evaluate intra-route 2-opt swaps sequentially instead of per-route parallel tasks | `E-n76-k10`, `E-n76-k14` | 830 in 17 s, 1022 in 56 s | Quality was preserved, but the fragile dense canary became much slower, so the parallel per-route 2-opt implementation was restored. |
| 2026-06-02 | Raise sparse exact pair-split profile to `avg + 8` with cap 22 | `E-n76-k7` | 689 in 51 s | Broader exact pair splitting did not move the sparse 689 plateau and added runtime, so the selected `avg + 6` / cap-18 profile was restored. |

## Selected Full Benchmark After Additional Rejected Experiments

Built with `make release LTO=ON NATIVE=ON` after `make format check` passed. Raw TSV: `/private/tmp/vrp-selected-full-clean-20260602-072602.tsv`.

| Instance | Target | Best | Gap | Routes | Solver ms | Wall s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `E-n101-k14` | 1067 | 1078 | +11 | 14 | 23085 | 23 |
| `E-n101-k8` | 815 | 817 | +2 | 8 | 90659 | 91 |
| `E-n51-k5` | 521 | 521 | 0 | 5 | 11242 | 11 |
| `E-n76-k10` | 830 | 830 | 0 | 10 | 15916 | 16 |
| `E-n76-k14` | 1021 | 1022 | +1 | 14 | 20040 | 20 |
| `E-n76-k7` | 682 | 689 | +7 | 7 | 27915 | 28 |
| `E-n76-k8` | 735 | 738 | +3 | 8 | 27170 | 27 |

## Final Selected Benchmark After Additional Sweep

`make format check` passed, then the binary was rebuilt with `make release LTO=ON NATIVE=ON`. Raw TSV:
`/private/tmp/vrp-final-selected-20260602-090308.tsv`. Raw logs:
`/private/tmp/vrp-final-selected-20260602-090308-logs`.

The selected code remains the fresh-incumbent tabu restart combination. Additional candidates tested after the previous
selection were rejected because they regressed solved guardrails or added runtime without closing gaps.

| Instance | Target | Best | Gap | Routes | Solver ms | Wall s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `E-n101-k14` | 1067 | 1078 | +11 | 14 | 45763 | 46 |
| `E-n101-k8` | 815 | 817 | +2 | 8 | 174096 | 175 |
| `E-n51-k5` | 521 | 521 | 0 | 5 | 19550 | 19 |
| `E-n76-k10` | 830 | 830 | 0 | 10 | 22672 | 22 |
| `E-n76-k14` | 1021 | 1022 | +1 | 14 | 34858 | 35 |
| `E-n76-k7` | 682 | 689 | +7 | 7 | 43225 | 43 |
| `E-n76-k8` | 735 | 738 | +3 | 8 | 41742 | 42 |

## Consolidated Current-Code Baseline

Experiments were stopped after this baseline. The binary was rebuilt with `make release LTO=ON NATIVE=ON`,
then each JSON instance in `instances/VRP-Set-E/` was run once. Raw TSV:
`doc/benchmarks/2026-06-02-current-baseline/results.tsv`. Raw logs:
`doc/benchmarks/2026-06-02-current-baseline/logs/`.

| Instance | Target | Best | Gap | Routes | Solver ms | Wall s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `E-n101-k14` | 1067 | 1078 | +11 | 14 | 29596 | 29 |
| `E-n101-k8` | 815 | 817 | +2 | 8 | 128007 | 128 |
| `E-n51-k5` | 521 | 521 | 0 | 5 | 15218 | 16 |
| `E-n76-k10` | 830 | 830 | 0 | 10 | 20154 | 20 |
| `E-n76-k14` | 1021 | 1022 | +1 | 14 | 26981 | 27 |
| `E-n76-k7` | 682 | 689 | +7 | 7 | 37430 | 38 |
| `E-n76-k8` | 735 | 738 | +3 | 8 | 40222 | 40 |

## Dead-Code Cleanup Guardrail

After removing legacy initializers, unused move routines, and the obsolete graph adjacency layer, `make check` passed.
The binary was rebuilt with `make release LTO=ON NATIVE=ON`, then solved guardrail instances were run once. Raw TSV:
`doc/benchmarks/2026-06-02-dead-code-cleanup-guardrail/results.tsv`. Raw logs:
`doc/benchmarks/2026-06-02-dead-code-cleanup-guardrail/logs/`.

| Instance | Target | Best | Gap | Routes | Solver ms | Wall s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `E-n51-k5` | 521 | 521 | 0 | 5 | 9117 | 9 |
| `E-n76-k10` | 830 | 830 | 0 | 10 | 12882 | 13 |
