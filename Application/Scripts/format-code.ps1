param(
    [string]$Root = ".."
)

$ErrorActionPreference = "Stop"

$clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $clangFormat) {
    Write-Error "clang-format was not found in PATH."
}

$rootInputPath = if ([System.IO.Path]::IsPathRooted($Root)) {
    $Root
} else {
    Join-Path $PSScriptRoot $Root
}

$rootPath = (Resolve-Path -LiteralPath $rootInputPath).Path

# Add folders or files here if they should be skipped during formatting.
$ignorePaths = @(
    ".codex",
    ".settings",
    ".vscode",
    "build",
    "cmake"
    "Core",
    "Drivers"
)

$sourceExtensions = @(
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx"
)

function Test-IgnoredPath {
    param(
        [string]$FullName
    )

    foreach ($ignorePath in $ignorePaths) {
        $ignoredFullPath = Join-Path $rootPath $ignorePath
        if ($FullName -like "$ignoredFullPath*") {
            return $true
        }
    }

    return $false
}

$files = Get-ChildItem -Path $rootPath -Recurse -File | Where-Object {
    ($sourceExtensions -contains $_.Extension.ToLowerInvariant()) -and
    (-not (Test-IgnoredPath -FullName $_.FullName))
}

if (-not $files) {
    Write-Host "No C/C++ source files found to format."
    exit 0
}

foreach ($file in $files) {
    Write-Host "Formatting $($file.FullName)"
    & $clangFormat.Source -i --style=file $file.FullName
}

Write-Host "Formatted $($files.Count) file(s)."
