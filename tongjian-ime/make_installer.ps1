# Build a self-extracting installer EXE with the built-in IExpress tool.
# Run from any directory; it works in an ASCII temp dir to avoid path issues.
$ErrorActionPreference = 'Stop'
$root = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$nl = [char]13 + [char]10

$base = Join-Path $env:TEMP 'TongjianBuild'
$dist = Join-Path $base 'dist'
$proj = Join-Path $base 'project'
Remove-Item $base -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $dist | Out-Null
New-Item -ItemType Directory -Force -Path $proj | Out-Null

# copy project (exclude dist/build)
Get-ChildItem $root -Force | Where-Object { $_.Name -notin @('dist', 'build', 'payload.zip') } |
    ForEach-Object { Copy-Item $_.FullName -Destination $proj -Recurse -Force }

# bootstrap.bat
$bl = @(
    '@echo off',
    'cd /d "%~dp0"',
    'echo Extracting...',
    'powershell -NoProfile -ExecutionPolicy Bypass -Command "Expand-Archive -Path ''%~dp0payload.zip'' -DestinationPath ''%~dp0project'' -Force"',
    'if errorlevel 1 ( echo [ERROR] extraction failed & pause & exit /b 1 )',
    'cd /d "%~dp0project"',
    'call setup.bat'
)
[System.IO.File]::WriteAllText((Join-Path $dist 'bootstrap.bat'), (($bl -join $nl) + $nl), [System.Text.Encoding]::ASCII)

# payload.zip
$zip = Join-Path $dist 'payload.zip'
Compress-Archive -Path (Join-Path $proj '*') -DestinationPath $zip -Force

# setup.sed (IExpress directive file)
$target = Join-Path $dist 'TongjianIME-Setup.exe'
$sedDir = $dist + '\'
$sl = @(
    '[Version]',
    'Class=IEXPRESS',
    'SEDVersion=3',
    '[Options]',
    'PackagePurpose=InstallApp',
    'ShowInstallProgramWindow=1',
    'HideExtractAnimation=1',
    'UseLongFileName=1',
    'InsideCompressed=0',
    'CAB_FixedSize=0',
    'CAB_ResvCodeSigning=0',
    'RebootMode=N',
    'InstallPrompt=%InstallPrompt%',
    'DisplayLicense=%DisplayLicense%',
    'FinishMessage=%FinishMessage%',
    'TargetName=%TargetName%',
    'FriendlyName=%FriendlyName%',
    'AppLaunched=%AppLaunched%',
    'PostInstallCmd=%PostInstallCmd%',
    'AdminQuietInstCmd=%AdminQuietInstCmd%',
    'UserQuietInstCmd=%UserQuietInstCmd%',
    'SourceFiles=SourceFiles',
    '[Strings]',
    'InstallPrompt=',
    'DisplayLicense=',
    'FinishMessage=',
    ('TargetName=' + $target),
    'FriendlyName=TongjianIME',
    'AppLaunched=cmd /c bootstrap.bat',
    'PostInstallCmd=<None>',
    'AdminQuietInstCmd=',
    'UserQuietInstCmd=',
    'FILE0="bootstrap.bat"',
    'FILE1="payload.zip"',
    '[SourceFiles]',
    ('SourceFiles0=' + $sedDir),
    '[SourceFiles0]',
    '%FILE0%=',
    '%FILE1%='
)
$sedPath = Join-Path $dist 'setup.sed'
[System.IO.File]::WriteAllText($sedPath, (($sl -join $nl) + $nl), [System.Text.Encoding]::ASCII)

# run IExpress
Push-Location $dist
& "$env:SystemRoot\System32\iexpress.exe" /N $sedPath
Pop-Location
Start-Sleep -Seconds 2

if (Test-Path $target) {
    $dstDir = Join-Path $root 'dist'
    New-Item -ItemType Directory -Force -Path $dstDir | Out-Null
    Copy-Item $target (Join-Path $dstDir 'TongjianIME-Setup.exe') -Force
    $sz = (Get-Item $target).Length
    Write-Output ("OK: {0} ({1:N1} MB)" -f (Join-Path $dstDir 'TongjianIME-Setup.exe'), ($sz / 1MB))
} else {
    Write-Output 'FAILED: no exe produced. Check the SED file.'
}
