# Smart City Disaster Module (OMNeT++ / INET)

This folder is a cleaned GitHub-ready export of the smart city disaster simulation project.

It keeps the project files that appear to be custom to your work and excludes heavy/generated files such as:

- OMNeT++ build output
- `.exe` binaries
- `.vec`, `.sca`, `.vci` result files
- generated plot images
- IDE/runtime cache files

## Included custom areas

- `samples/EarthquakeDisaster/`
- `samples/inet4/src/inet/common/earthquake/`

## Important note

This export assumes your authored project is mainly:

- the `EarthquakeDisaster` sample project
- the custom INET earthquake module under `inet/common/earthquake`

If you also modified other INET files outside those folders, they are not included yet.

## Suggested GitHub repo structure

Upload this folder as the repository root.

## How to use later

To run this project on another machine, the user will typically need:

1. OMNeT++ 5.7.1
2. A matching INET workspace
3. These exported folders placed back into the same relative workspace structure

## Included analysis scripts

The Python plotting scripts from:

- `samples/EarthquakeDisaster/simulations/results/Disaster Module plots/`

are included, but the generated images are intentionally omitted.
