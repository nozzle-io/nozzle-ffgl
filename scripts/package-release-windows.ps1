[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('windows')]
    [string] $Platform,

    [Parameter(Mandatory = $true)]
    [string] $PackageName,

    [Parameter(Mandatory = $true)]
    [string] $PackageRoot,

    [Parameter(Mandatory = $true)]
    [string] $BuildDir
)

$ErrorActionPreference = 'Stop'

$receive = Get-ChildItem -Path $BuildDir -Recurse -File -Filter 'NozzleReceive.dll' |
    Sort-Object -Property FullName |
    Select-Object -First 1
$send = Get-ChildItem -Path $BuildDir -Recurse -File -Filter 'NozzleSend.dll' |
    Sort-Object -Property FullName |
    Select-Object -First 1
if ($null -eq $receive -or $null -eq $send) {
    throw "missing NozzleReceive.dll or NozzleSend.dll under $BuildDir"
}

Remove-Item -LiteralPath 'package' -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $PackageName -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath 'verify-package' -Recurse -Force -ErrorAction SilentlyContinue

$packageDir = Join-Path 'package' $PackageRoot
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null
Copy-Item -LiteralPath $receive.FullName -Destination (Join-Path $packageDir 'NozzleReceive.dll')
Copy-Item -LiteralPath $send.FullName -Destination (Join-Path $packageDir 'NozzleSend.dll')
Copy-Item -LiteralPath 'README.md' -Destination (Join-Path $packageDir 'README.md')
Copy-Item -LiteralPath 'LICENSE' -Destination (Join-Path $packageDir 'LICENSE')
Copy-Item -LiteralPath 'THIRD-PARTY-NOTICES.md' -Destination (Join-Path $packageDir 'THIRD-PARTY-NOTICES.md')

foreach ($dll in @('NozzleReceive.dll', 'NozzleSend.dll')) {
    $binaryPath = Join-Path $packageDir $dll
    $bytes = [System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $binaryPath))
    if ($bytes.Length -lt 2 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) {
        throw "not a PE/MZ binary: $binaryPath"
    }
    & dumpbin /headers $binaryPath | Tee-Object -FilePath "$dll.headers.txt"
    Select-String -Path "$dll.headers.txt" -Pattern 'DLL' | Select-Object -First 1
    & dumpbin /exports $binaryPath | Tee-Object -FilePath "$dll.exports.txt"
    Select-String -Path "$dll.exports.txt" -Pattern '\bplugMain\b' | Select-Object -First 1
}

Compress-Archive -Path $packageDir -DestinationPath $PackageName -CompressionLevel Optimal
if (!(Test-Path -LiteralPath $PackageName -PathType Leaf)) {
    throw "package was not created: $PackageName"
}

Expand-Archive -Path $PackageName -DestinationPath 'verify-package'
$required = @(
    "$PackageRoot/NozzleReceive.dll",
    "$PackageRoot/NozzleSend.dll",
    "$PackageRoot/README.md",
    "$PackageRoot/LICENSE",
    "$PackageRoot/THIRD-PARTY-NOTICES.md"
)
foreach ($entry in $required) {
    $entryPath = Join-Path 'verify-package' $entry
    if (!(Test-Path -LiteralPath $entryPath -PathType Leaf)) {
        throw "package is missing: $entry"
    }
    Write-Output $entry
}
