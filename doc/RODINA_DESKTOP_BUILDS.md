# RODINA desktop builds

RODINA currently provides Linux x64 and Windows x64 archives. Both are built from the same Git commit by the manual [RODINA Desktop Build](../.github/workflows/desktop-build.yml) workflow. The workflow uploads GitHub Actions artifacts; it does not publish a Release. macOS builds are not supported.

| Platform | Archive inside the Actions artifact | Configuration |
| --- | --- | --- |
| Linux x64 | `Cataclysm-RODINA-<full commit SHA>-linux-x64.tar.gz` | Tiles, sound, Russian catalog; system SDL2 libraries required at runtime. |
| Windows x64 | `Cataclysm-RODINA-<full commit SHA>-windows-x64.zip` | MSVC Release/x64 with tiles, sound, and Russian catalog. |

## Run the GitHub build

Open **Actions → RODINA Desktop Build → Run workflow** and select a branch to build. To build a tag containing this workflow, run `gh workflow run desktop-build.yml --ref <tag>` with GitHub CLI. The Linux and Windows jobs check that their checkout matches `GITHUB_SHA`. Each job uploads its archive under a name containing that full SHA.

The workflow is not triggered by pushes or pull requests. The separate `RODINA CI` workflow runs validation automatically; its long Linux build and tests run only when that workflow is started manually.

To verify a desktop build, check that both jobs succeeded and both archives were uploaded. Download the artifacts, extract the game archives inside them, and compare `RESOURCE-TREES.txt` in each archive. The `data` and `gfx` tree IDs must match across platforms. Launch the extracted game on each supported platform to check the packaged executable, graphics, sound, and game data.

## Build locally on Linux

Run `JOBS=3 ./tools/build-linux.sh` after installing the build dependencies listed in the script. It builds the graphical game, compiles the Russian catalog, packages `cataclysm-rodina-0.1-dev.tar.gz`, and performs a game-data smoke check. Windows builds are maintained through GitHub Actions.

## Source and resource provenance

The game resources under `data/` and `gfx/` are exported from `HEAD` with `git archive` for both archives. This keeps packaged resources tied to the same commit as the compiled sources. The repository contains its tilesets and the basic soundpack; the build does not download future versions of those resources from another repository.

Build dependencies are less strictly pinned. Windows uses a fixed vcpkg commit and pinned action revisions. Linux installs packages from Ubuntu 22.04 repositories. Runner images, MSVC, Ubuntu packages, and MSYS2 packages can change over time, so rebuilding a commit preserves its source and resources but does not yet guarantee byte-for-byte identical binaries.
