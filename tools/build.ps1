$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$QtRoot = "C:\Qt\6.7.3\msvc2019_64"
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if (-not (Test-Path $QtRoot)) { throw "Qt not found at $QtRoot" }
if (-not (Test-Path $vcvars)) { throw "VS BuildTools vcvars64.bat not found" }

cmd /c "`"$vcvars`" && cd /d `"$Root`" && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=`"$QtRoot`" && cmake --build build --config Release && `"$QtRoot\bin\windeployqt.exe`" --release --no-translations build\WfmMonitor.exe"
Write-Host "Built: $Root\build\WfmMonitor.exe"
