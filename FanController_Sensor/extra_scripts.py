"""
extra_scripts.py — PlatformIO pre-build fix for DigisparkCDC assembler file.

The DigisparkCDC library in the framework-arduino-avr-digistump package ships
two assembler source files for the USB driver:
  - usbdrvasm.S   — GNU Assembler (GAS) compatible format, compiles correctly
  - usbdrvasm.asm — Atmel Studio / avrasm2 format; contains 'END' directive
                    which is unknown to GAS, causing a build error

Both files implement the same USB bit-bang driver. Only the .S version is
needed. This script renames the .asm file to .asm.bak so PlatformIO's source
scanner does not attempt to compile it.

The rename is idempotent: if already renamed (or file absent) nothing happens.
"""
Import("env")  # noqa: F821 — PlatformIO SCons environment
import os

_cdc_dir = os.path.join(
    os.path.expanduser("~"), ".platformio", "packages",
    "framework-arduino-avr-digistump", "libraries", "DigisparkCDC"
)

_asm = os.path.join(_cdc_dir, "usbdrvasm.asm")
_bak = _asm + ".bak"

if os.path.isfile(_asm):
    os.rename(_asm, _bak)
    print("[extra_scripts] Renamed usbdrvasm.asm -> usbdrvasm.asm.bak "
          "(Atmel Studio format; GAS-compatible .S version is used instead)")
