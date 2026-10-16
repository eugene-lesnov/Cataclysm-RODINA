# RODINA desktop builds and releases

RODINA supports Linux x64 and Windows x64 archives. macOS builds are not supported.

## Versions

RODINA uses its own `major.minor.patch` versions and `rodina-<version>` release tags. `0.0.x` is reserved for technical releases that prove the build and distribution pipeline before the setting conversion. `rodina-0.0.1` is the technical milestone. `rodina-0.1.0` is the first setting slice: the evacuee start, Emergency Service shelters, the route to the district camp, and the first camp mission. Development builds currently identify themselves as `0.1-dev` and are not releases.

The CDDA base is recorded separately. RODINA `0.0.1` is based on CDDA `0.I-1`; CDDA's version is never used as RODINA's version. The main menu, `--version`, `VERSION.txt`, release notes, and build manifests identify the base. The technical release retains CDDA core gameplay and content with RODINA branding, the RODINA build pipeline, and the repository's `lang/po/ru.po`. Earlier infrastructure work removed optional upstream mods, so the package is not a byte-for-byte CDDA distribution. It does not claim a converted setting.

## Continuous integration

[RODINA CI](../.github/workflows/rodina-ci.yml) runs on pull requests to `master` and pushes to `master`. It validates JSON, C++ formatting, and every translation catalog in `lang/po/`. The same source commit is built on Linux and Windows. Both platforms load the RODINA core data; Linux also runs the regular C++ test suite, excluding slow and hidden tests. A tag push does not create a release.

The reusable [RODINA Desktop Build](../.github/workflows/desktop-build.yml) workflow builds both platforms. It packages and uploads Actions artifacts when started manually or called by the release workflow on a release tag. Manual branch builds use the full source commit SHA as their build ID and `0.1-dev` as their game version. Release builds use the tag name as their build ID and its RODINA version as their game version.

| Platform | Archive | Configuration |
| --- | --- | --- |
| Linux x64 | `Cataclysm-RODINA-<build ID>-linux-x64.tar.gz` | Tiles, sound, Russian catalog; system SDL2 libraries required at runtime. |
| Windows x64 | `Cataclysm-RODINA-<build ID>-windows-x64.zip` | MSVC Release/x64 with tiles, sound, and Russian catalog. |

## Build manifest

Each release archive contains `BUILD-MANIFEST.json`, `VERSION.txt`, and `RESOURCE-TREES.txt`. The manifest records the RODINA version, source Git SHA, CDDA base, platform, Git identities of bundled `data/`, `gfx/`, `data/sound/Basic`, and `lang/po/ru.po`, plus the compiler, build tools, gettext version, and relevant dependency versions or vcpkg commit. The release also attaches both manifests separately and includes them in `SHA256SUMS.txt`.

No game assets are downloaded from external repositories during these builds. `external_game_resources` is empty because the packaged resources come from the tagged source commit. Linux installs build dependencies from Ubuntu 22.04 repositories; Windows uses a pinned vcpkg commit and pinned action revisions. Runner images and system packages can still change, so the builds are traceable but are not guaranteed to be byte-for-byte reproducible.

## Manual release

Commit and push the release source to `master`, then create and push `rodina-0.1.0` on that commit. Run the release workflow explicitly on the pushed tag:

```sh
gh workflow run release.yml --ref rodina-0.1.0
```

The [RODINA Release](../.github/workflows/release.yml) workflow accepts only `workflow_dispatch`. It checks that the tag has the expected version format and points to a commit on `master`, validates the source, builds both platforms, checks archive integrity and manifests, and creates a **draft prerelease** with two archives, two manifests, and `SHA256SUMS.txt`. Validation or build failure prevents draft creation; an upload failure can leave a partial draft that should be inspected before retrying. Review the draft and launch the extracted game on Linux and Windows before publishing it manually. No push automatically publishes a release.

## Build locally on Linux

Run `JOBS=3 ./tools/build-linux.sh` after installing the dependencies listed in the script. It builds the graphical development version, packages `cataclysm-rodina-0.1-dev.tar.gz`, and checks game-data loading. Windows builds are maintained through GitHub Actions.
