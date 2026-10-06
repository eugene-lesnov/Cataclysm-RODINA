# Building Cataclysm: RODINA

The supported desktop build configurations are Linux x64 and Windows x64. See the [desktop build guide](../RODINA_DESKTOP_BUILDS.md) for the GitHub Actions workflow, archive contents, resource provenance, and verification steps.

For a local Linux build, install the tools and development libraries checked by `tools/build-linux.sh`, then run:

```sh
JOBS=3 ./tools/build-linux.sh
```

The script builds the graphical game and its archive, then checks that the core game data loads. Windows archives are built in GitHub Actions. RODINA does not maintain a macOS build configuration.
