#!/bin/bash

# Check Installation supports this example
checkinstall.exe -p install.pkg --nobanner || exit

# Select CrossCompiler for RISCV32/riscv (see alternatives in README)
CROSS=RISCV32

# Build Application
make -C application CROSS=${CROSS}

# Build Platform
make -C platform    CROSS=${CROSS}

# platform/platform.${IMPERAS_ARCH}.exe -program application/application.${CROSS}.elf

platform/platform.${IMPERAS_ARCH}.exe -program application/application.${CROSS}.elf \
--gdbegui --eguicommands "-breakonstartup main --continueonstartup" \
