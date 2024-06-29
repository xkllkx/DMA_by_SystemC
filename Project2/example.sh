#!/bin/bash

# Check Installation supports this example
checkinstall.exe -p install.pkg --nobanner || exit

# Select CrossCompiler for RISCV32/riscv (see alternatives in README)
CROSS=RISCV32

# Build Application
make -C application CROSS=${CROSS}

# Build Platform
make -C platform    CROSS=${CROSS}

platform/platform.${IMPERAS_ARCH}.exe --program top.cpu1=application/riscv1.RISCV32.elf --program top.cpu2=application/riscv2.RISCV32.elf \
#--gdbegui --eguicommands "--breakonstartup main --continueonstartup" \
