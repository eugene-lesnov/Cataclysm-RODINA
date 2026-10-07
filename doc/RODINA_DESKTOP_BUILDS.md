# RODINA desktop builds

RODINA supports Linux x64 and Windows x64 archives. macOS builds are not supported.

## Continuous integration

[RODINA CI](../.github/workflows/rodina-ci.yml) runs on pull requests to `master`, pushes to `master`, and `rodina-v*` tag pushes. It validates JSON, C++ formatting, and every translation catalog in `lang/po/`. The same source commit is then built on Linux and Windows. Both platforms load the RODINA core data and run the regular C++ test suite, excluding slow and hidden tests.

Pull request builds cancel older runs for the same pull request. Builds on `master` and release tags run to completion. CI also has a manual `workflow_dispatch` entry point.

## Desktop archives

The reusable [RODINA Desktop Build](../.github/workflows/desktop-build.yml) workflow builds both platforms. It packages and uploads Actions artifacts when called by a release tag or started manually. Pull requests and `master` pushes run the builds and tests without a second Linux rebuild for packaging.

| Platform | Archive | Configuration |
| --- | --- | --- |
| Linux x64 | `Cataclysm-RODINA-<build ID>-linux-x64.tar.gz` | Tiles, sound, Russian catalog; system SDL2 libraries required at runtime. |
| Windows x64 | `Cataclysm-RODINA-<build ID>-windows-x64.zip` | MSVC Release/x64 with tiles, sound, and Russian catalog. |

For a manual build, open **Actions → RODINA Desktop Build → Run workflow** and select a branch or use `gh workflow run desktop-build.yml --ref <branch-or-tag>`. Its build ID is the full source commit SHA for branch builds. A release tag uses the tag name instead.

## Tag releases

Create a tag such as `rodina-v0.1.0-rc.1` on a commit from `master` after its CI run passes, then push the tag. The tag runs validation and both desktop builds at the tagged commit. The game version and `VERSION.txt` use `0.1.0-rc.1` inside those archives.

After both platforms succeed, CI checks archive integrity, verifies the version and commit in `VERSION.txt`, compares the `data` and `gfx` tree IDs in `RESOURCE-TREES.txt`, and creates a **draft prerelease** with both archives and `SHA256SUMS.txt`. Review the draft and launch the extracted game on each supported platform before publishing it. A failed validation, build, test, or package check leaves no new draft release.

The game resources under `data/` and `gfx/` are exported from the tagged commit with `git archive` for both platforms. Windows uses a fixed vcpkg commit and pinned action revisions. Linux installs Ubuntu 22.04 packages. These builds preserve source and resource identity but are not guaranteed to be byte-for-byte reproducible across runner image or package updates.

## Build locally on Linux

Run `JOBS=3 ./tools/build-linux.sh` after installing the dependencies listed in the script. It builds the graphical game, packages `cataclysm-rodina-0.1-dev.tar.gz`, and checks game-data loading. Windows builds are maintained through GitHub Actions.
