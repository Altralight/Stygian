# Headless Comparison Summary

## Scope

- `Count` and `Mutate` are logical scene driver inputs, not actual primitive, glyph, or element counts.
- `headless-primitive` rows are CPU-side scene build only.
- `cpu-authoring-full-build` rows are Stygian full scene authoring every frame with backend render skipped.
- `raw-gpu-resident` rows are Stygian offscreen render passes with real GPU, upload, and text expansion cost.
- `eval-only-replay` rows are Stygian author/eval/replay with replay allowed and backend render skipped.
- These modes should not be collapsed into a single flat leaderboard.

## CPU Builder Rows

These are the closest thing to an apples-to-apples CPU churn lane in this harness.
Stygian's `cpu-authoring-full-build` mode rebuilds both scopes every frame and skips backend rendering.

| Library | Mode | Scene | Scenario | Logical Items | Mutate | FPS | Avg Frame ms | Avg Commands | Avg Vertices | Avg Indices |
|---------|------|-------|----------|---------------|--------|-----|--------------|--------------|--------------|-------------|
| clay | headless-primitive | cards | fullhot | 10000 | 100 | 3.35 | 298.4653 | 20000 | 0 | 0 |
| clay | headless-primitive | cards | sparse | 10000 | 100 | 3.21 | 311.1197 | 20000 | 0 | 0 |
| clay | headless-primitive | cards | static | 10000 | 100 | 3.24 | 308.8195 | 20000 | 0 | 0 |
| clay | headless-primitive | graph | fullhot | 10000 | 100 | 2.88 | 347.4649 | 20000 | 0 | 0 |
| clay | headless-primitive | graph | sparse | 10000 | 100 | 3.06 | 327.1406 | 20000 | 0 | 0 |
| clay | headless-primitive | graph | static | 10000 | 100 | 3.05 | 327.9462 | 20000 | 0 | 0 |
| clay | headless-primitive | hierarchy | fullhot | 10000 | 100 | 0.17 | 5944.2617 | 50000 | 0 | 0 |
| clay | headless-primitive | hierarchy | sparse | 10000 | 100 | 0.20 | 4988.6092 | 50000 | 0 | 0 |
| clay | headless-primitive | hierarchy | static | 10000 | 100 | 0.19 | 5252.7671 | 50000 | 0 | 0 |
| clay | headless-primitive | inspector | fullhot | 10000 | 100 | 0.65 | 1533.6727 | 40000 | 0 | 0 |
| clay | headless-primitive | inspector | sparse | 10000 | 100 | 0.70 | 1435.5966 | 40000 | 0 | 0 |
| clay | headless-primitive | inspector | static | 10000 | 100 | 0.83 | 1201.6384 | 40000 | 0 | 0 |
| clay | headless-primitive | textwall | fullhot | 10000 | 100 | 3.20 | 312.0302 | 10000 | 0 | 0 |
| clay | headless-primitive | textwall | sparse | 10000 | 100 | 3.23 | 309.7703 | 10000 | 0 | 0 |
| clay | headless-primitive | textwall | static | 10000 | 100 | 3.26 | 306.5680 | 10000 | 0 | 0 |
| egui | headless-primitive | cards | fullhot | 10000 | 100 | 49.41 | 20.2398 | 1 | 46656 | 160704 |
| egui | headless-primitive | cards | sparse | 10000 | 100 | 43.10 | 23.2009 | 1 | 46656 | 160704 |
| egui | headless-primitive | cards | static | 10000 | 100 | 44.39 | 22.5256 | 1 | 46656 | 160704 |
| egui | headless-primitive | graph | fullhot | 10000 | 100 | 26.21 | 38.1462 | 1 | 26620 | 101550 |
| egui | headless-primitive | graph | sparse | 10000 | 100 | 22.31 | 44.8148 | 1 | 26620 | 101550 |
| egui | headless-primitive | graph | static | 10000 | 100 | 24.58 | 40.6837 | 1 | 26620 | 101550 |
| egui | headless-primitive | hierarchy | fullhot | 10000 | 100 | 28.81 | 34.7047 | 1 | 23684 | 81048 |
| egui | headless-primitive | hierarchy | sparse | 10000 | 100 | 26.86 | 37.2292 | 1 | 23684 | 81048 |
| egui | headless-primitive | hierarchy | static | 10000 | 100 | 27.23 | 36.7224 | 1 | 23684 | 81048 |
| egui | headless-primitive | inspector | fullhot | 10000 | 100 | 24.45 | 40.8986 | 1 | 26117 | 88423 |
| egui | headless-primitive | inspector | sparse | 10000 | 100 | 23.91 | 41.8182 | 1 | 26192 | 88536 |
| egui | headless-primitive | inspector | static | 10000 | 100 | 23.07 | 43.3469 | 1 | 26192 | 88536 |
| egui | headless-primitive | textwall | fullhot | 10000 | 100 | 64.08 | 15.6044 | 1 | 17280 | 25920 |
| egui | headless-primitive | textwall | sparse | 10000 | 100 | 46.71 | 21.4085 | 1 | 17280 | 25920 |
| egui | headless-primitive | textwall | static | 10000 | 100 | 63.45 | 15.7602 | 1 | 17280 | 25920 |
| imgui | headless-primitive | cards | fullhot | 10000 | 100 | 27.32 | 36.6071 | 2 | 520000 | 960000 |
| imgui | headless-primitive | cards | sparse | 10000 | 100 | 28.97 | 34.5171 | 2 | 520000 | 960000 |
| imgui | headless-primitive | cards | static | 10000 | 100 | 32.21 | 31.0492 | 2 | 520000 | 960000 |
| imgui | headless-primitive | graph | fullhot | 10000 | 100 | 15.69 | 63.7507 | 2 | 949748 | 1784622 |
| imgui | headless-primitive | graph | sparse | 10000 | 100 | 15.27 | 65.4874 | 2 | 949748 | 1784622 |
| imgui | headless-primitive | graph | static | 10000 | 100 | 16.81 | 59.4725 | 2 | 949748 | 1784622 |
| imgui | headless-primitive | hierarchy | fullhot | 10000 | 100 | 12.32 | 81.1752 | 2 | 1018000 | 2019000 |
| imgui | headless-primitive | hierarchy | sparse | 10000 | 100 | 11.92 | 83.8861 | 2 | 1018000 | 2019000 |
| imgui | headless-primitive | hierarchy | static | 10000 | 100 | 13.43 | 74.4695 | 2 | 1018000 | 2019000 |
| imgui | headless-primitive | inspector | fullhot | 10000 | 100 | 15.82 | 63.2140 | 2 | 918167 | 1737250 |
| imgui | headless-primitive | inspector | sparse | 10000 | 100 | 16.57 | 60.3408 | 2 | 923152 | 1744729 |
| imgui | headless-primitive | inspector | static | 10000 | 100 | 15.65 | 63.8975 | 2 | 923204 | 1744806 |
| imgui | headless-primitive | textwall | fullhot | 10000 | 100 | 51.41 | 19.4507 | 2 | 360000 | 540000 |
| imgui | headless-primitive | textwall | sparse | 10000 | 100 | 47.32 | 21.1340 | 2 | 360000 | 540000 |
| imgui | headless-primitive | textwall | static | 10000 | 100 | 49.18 | 20.3341 | 2 | 360000 | 540000 |
| nuklear | headless-primitive | cards | fullhot | 10000 | 100 | 366.39 | 2.7293 | 20003 | 0 | 0 |
| nuklear | headless-primitive | cards | sparse | 10000 | 100 | 314.79 | 3.1768 | 20003 | 0 | 0 |
| nuklear | headless-primitive | cards | static | 10000 | 100 | 337.90 | 2.9595 | 20003 | 0 | 0 |
| nuklear | headless-primitive | graph | fullhot | 10000 | 100 | 133.13 | 7.5112 | 94320 | 0 | 0 |
| nuklear | headless-primitive | graph | sparse | 10000 | 100 | 182.50 | 5.4793 | 94320 | 0 | 0 |
| nuklear | headless-primitive | graph | static | 10000 | 100 | 161.50 | 6.1919 | 94320 | 0 | 0 |
| nuklear | headless-primitive | hierarchy | fullhot | 10000 | 100 | 263.16 | 3.7999 | 50003 | 0 | 0 |
| nuklear | headless-primitive | hierarchy | sparse | 10000 | 100 | 245.21 | 4.0781 | 50003 | 0 | 0 |
| nuklear | headless-primitive | hierarchy | static | 10000 | 100 | 226.81 | 4.4091 | 50003 | 0 | 0 |
| nuklear | headless-primitive | inspector | fullhot | 10000 | 100 | 129.84 | 7.7019 | 40003 | 0 | 0 |
| nuklear | headless-primitive | inspector | sparse | 10000 | 100 | 116.02 | 8.6190 | 40003 | 0 | 0 |
| nuklear | headless-primitive | inspector | static | 10000 | 100 | 93.15 | 10.7357 | 40003 | 0 | 0 |
| nuklear | headless-primitive | textwall | fullhot | 10000 | 100 | 929.07 | 1.0763 | 10003 | 0 | 0 |
| nuklear | headless-primitive | textwall | sparse | 10000 | 100 | 823.36 | 1.2145 | 10003 | 0 | 0 |
| nuklear | headless-primitive | textwall | static | 10000 | 100 | 715.13 | 1.3983 | 10003 | 0 | 0 |
| stygian | cpu-authoring-full-build | cards | fullhot | 10000 | 100 | 28.62 | 34.9376 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | cards | sparse | 10000 | 100 | 30.14 | 33.1804 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | cards | static | 10000 | 100 | 27.40 | 36.4935 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | graph | fullhot | 10000 | 100 | 9.46 | 105.7267 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | graph | sparse | 10000 | 100 | 16.26 | 61.4976 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | graph | static | 10000 | 100 | 19.18 | 52.1308 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | hierarchy | fullhot | 10000 | 100 | 9.66 | 103.4879 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | hierarchy | sparse | 10000 | 100 | 18.14 | 55.1164 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | hierarchy | static | 10000 | 100 | 17.16 | 58.2833 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | inspector | fullhot | 10000 | 100 | 7.44 | 134.4107 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | inspector | sparse | 10000 | 100 | 8.30 | 120.5351 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | inspector | static | 10000 | 100 | 14.83 | 67.4424 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | textwall | fullhot | 10000 | 100 | 12.60 | 79.3718 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | textwall | sparse | 10000 | 100 | 24.86 | 40.2306 | 0 | 0 | 0 |
| stygian | cpu-authoring-full-build | textwall | static | 10000 | 100 | 27.26 | 36.6836 | 0 | 0 | 0 |

## Stygian Native Modes

These rows show Stygian's native GPU path. Logical Items can expand into far more actual Stygian elements.
They show Stygian's native strengths, but they are not fair CPU-builder comparisons.

| Mode | Scene | Scenario | Logical Items | Mutate | FPS | Avg Frame ms | Avg Elements | Avg Upload Bytes | Avg Upload Ranges | Replay Hit Rate |
|------|-------|----------|---------------|--------|-----|--------------|--------------|------------------|-------------------|-----------------|
| eval-only-replay | cards | fullhot | 10000 | 100 | 36.83 | 27.1506 | 120000 | 0 | 0 | 100% |
| eval-only-replay | cards | sparse | 10000 | 100 | 37.26 | 26.8404 | 110100 | 0 | 0 | 100% |
| eval-only-replay | cards | static | 10000 | 100 | 35.85 | 27.8969 | 110000 | 0 | 0 | 100% |
| eval-only-replay | graph | fullhot | 10000 | 100 | 6.62 | 150.9788 | 374874 | 0 | 0 | 100% |
| eval-only-replay | graph | sparse | 10000 | 100 | 17.12 | 58.4223 | 188278 | 0 | 0 | 100% |
| eval-only-replay | graph | static | 10000 | 100 | 23.28 | 42.9525 | 187437 | 0 | 0 | 100% |
| eval-only-replay | hierarchy | fullhot | 10000 | 100 | 10.68 | 93.6063 | 365000 | 0 | 0 | 100% |
| eval-only-replay | hierarchy | sparse | 10000 | 100 | 21.80 | 45.8713 | 184325 | 0 | 0 | 100% |
| eval-only-replay | hierarchy | static | 10000 | 100 | 22.14 | 45.1593 | 182500 | 0 | 0 | 100% |
| eval-only-replay | inspector | fullhot | 10000 | 100 | 8.09 | 123.6224 | 364089 | 0 | 0 | 100% |
| eval-only-replay | inspector | sparse | 10000 | 100 | 16.92 | 59.0920 | 185109 | 0 | 0 | 100% |
| eval-only-replay | inspector | static | 10000 | 100 | 14.86 | 67.2953 | 183301 | 0 | 0 | 100% |
| eval-only-replay | textwall | fullhot | 10000 | 100 | 14.13 | 70.7925 | 200000 | 0 | 0 | 100% |
| eval-only-replay | textwall | sparse | 10000 | 100 | 36.50 | 27.3951 | 101000 | 0 | 0 | 100% |
| eval-only-replay | textwall | static | 10000 | 100 | 38.04 | 26.2875 | 100000 | 0 | 0 | 100% |
| raw-gpu-resident | cards | fullhot | 10000 | 100 | 29.12 | 34.3419 | 120000 | 2116608 | 120 | 100% |
| raw-gpu-resident | cards | sparse | 10000 | 100 | 27.83 | 35.9326 | 110100 | 57408 | 6 | 100% |
| raw-gpu-resident | cards | static | 10000 | 100 | 35.41 | 28.2388 | 110000 | 0 | 0 | 100% |
| raw-gpu-resident | graph | fullhot | 10000 | 100 | 6.61 | 151.3893 | 374874 | 50252064 | 2849 | 80% |
| raw-gpu-resident | graph | sparse | 10000 | 100 | 23.39 | 42.7450 | 188277 | 4510288 | 276 | 100% |
| raw-gpu-resident | graph | static | 10000 | 100 | 19.76 | 50.6197 | 187437 | 0 | 0 | 100% |
| raw-gpu-resident | hierarchy | fullhot | 10000 | 100 | 8.11 | 123.2497 | 365000 | 38007424 | 2142 | 100% |
| raw-gpu-resident | hierarchy | sparse | 10000 | 100 | 22.04 | 45.3733 | 184325 | 427024 | 27 | 100% |
| raw-gpu-resident | hierarchy | static | 10000 | 100 | 23.93 | 41.7880 | 182500 | 0 | 0 | 100% |
| raw-gpu-resident | inspector | fullhot | 10000 | 100 | 6.96 | 143.6056 | 364091 | 37873888 | 2136 | 100% |
| raw-gpu-resident | inspector | sparse | 10000 | 100 | 13.85 | 72.1772 | 185109 | 430612 | 27 | 100% |
| raw-gpu-resident | inspector | static | 10000 | 100 | 20.30 | 49.2520 | 183301 | 0 | 0 | 100% |
| raw-gpu-resident | textwall | fullhot | 10000 | 100 | 13.47 | 74.2218 | 200000 | 20833280 | 1176 | 100% |
| raw-gpu-resident | textwall | sparse | 10000 | 100 | 35.61 | 28.0842 | 101000 | 241280 | 15 | 100% |
| raw-gpu-resident | textwall | static | 10000 | 100 | 39.33 | 25.4289 | 100000 | 0 | 0 | 100% |

## Interpretation

- Use CPU Builder Rows for cross-library CPU scene-build cost.
- Use Stygian Native Modes for GPU-native Stygian behavior, including replay and upload cost.
- Treat Logical Items as scene-driver inputs, not as fully normalized primitive counts.

Artifacts:
- summary.csv for machine-readable results
- summary.log for raw harness stdout and STYGIANDETAIL lines
- summary.md separates CPU authoring rows from Stygian native rows
