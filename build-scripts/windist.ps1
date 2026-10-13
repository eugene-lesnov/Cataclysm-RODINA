$ErrorActionPreference = 'Stop'

if (-not $env:RODINA_VERSION) {
  $env:RODINA_VERSION = '0.1-dev'
}

if (Test-path bindist) {
  rm -Force -Recurse bindist
}

mkdir bindist
cp cataclysm-rodina-tiles.exe bindist/cataclysm-rodina-tiles.exe
cp cataclysm-rodina-tiles.stripped.pdb bindist/cataclysm-rodina-tiles.pdb
cp tools/format/json_formatter.exe bindist/json_formatter.exe
cp zzip.exe bindist/zzip.exe

mkdir bindist/lang
cp -r lang/mo bindist/lang

$extras = "doc", "CREDITS.md", "LICENSE.txt", "LICENSE-Apache-Robot-Font.txt", "LICENSE-OFL-Terminus-Font.txt", "README.md", "VERSION.txt"
ForEach ($extra in $extras) {
	cp -r $extra bindist
}
if ($env:RELEASE_MANIFEST -eq '1') {
  cp BUILD-MANIFEST.json bindist
}

$resourceArchive = Join-Path $env:TEMP ("rodina-resources-" + [guid]::NewGuid() + ".tar")
git archive --format=tar --output="$resourceArchive" HEAD data gfx
if ($LASTEXITCODE -ne 0) { throw "Could not archive desktop resources from HEAD." }
tar -xf $resourceArchive -C bindist
if ($LASTEXITCODE -ne 0) { throw "Could not extract desktop resources." }
Remove-Item $resourceArchive
$dataTree = git rev-parse HEAD:data
$gfxTree = git rev-parse HEAD:gfx
"data $dataTree`ngfx $gfxTree" | Set-Content -Encoding ascii bindist/RESOURCE-TREES.txt
Compress-Archive -Force -Path bindist/* -DestinationPath "cataclysm-rodina-$($env:RODINA_VERSION).zip"
