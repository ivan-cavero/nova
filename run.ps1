param(
    [string]$IsoPath = "$env:USERPROFILE\Desktop\Dev\Kernel\myKernel.iso",
    [switch]$Gdb,
    [switch]$NoGraphic,
    [int]$Memory = 64
)

if (-not (Test-Path $IsoPath)) {
    $IsoPath = "\\wsl.localhost\Debian\home\ivang\osdev\kernel\myKernel.iso"
}

if (-not (Test-Path $IsoPath)) {
    Write-Error "ISO not found. Run 'make iso' in WSL first."
    exit 1
}

$qemu = "C:\Program Files\qemu\qemu-system-x86_64.exe"

$args = @(
    "-cdrom", """$IsoPath""",
    "-m", $Memory.ToString(),
    "-no-reboot",
    "-no-shutdown"
)

if ($Gdb) {
    $args += "-s", "-S"
}

if ($NoGraphic) {
    $args += "-nographic"
} else {
    $args += "-serial", "stdio"
}

$cmd = "& ""$qemu"" $args"
Write-Host "Running: $cmd" -ForegroundColor Cyan
Invoke-Expression $cmd