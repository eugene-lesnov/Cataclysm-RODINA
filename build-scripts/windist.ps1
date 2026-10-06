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

$extras = "data", "doc", "gfx", "LICENSE.txt", "LICENSE-OFL-Terminus-Font.txt", "README.md", "VERSION.txt"
ForEach ($extra in $extras) {
	cp -r $extra bindist
}
Compress-Archive -Force -Path bindist/* -DestinationPath "cataclysm-rodina-0.1-dev.zip"
