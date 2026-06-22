@echo off
setlocal enabledelayedexpansion

set QEMU="C:\Program Files\qemu\qemu-system-x86_64.exe"
set ISO=\\wsl.localhost\Debian\home\ivang\osdev\kernel\myKernel.iso

if not exist %ISO% (
    echo [!] ISO no encontrada. Ejecuta 'make iso' en WSL primero.
    pause
    exit /b 1
)

echo [*] Booting myKernel v0.1 en QEMU...
echo [*] Deberias ver texto en pantalla.
echo [*] Presiona Ctrl+Alt+2 para monitor QEMU (Ctrl+Alt+1 para volver).
echo.
%QEMU% -cdrom %ISO% -m 64 -display sdl

if %ERRORLEVEL% NEQ 0 (
    echo [!] Fallo con SDL. Probando con GTK...
    %QEMU% -cdrom %ISO% -m 64
)

pause