 
  Copyright (c) 2005-2021 Imperas Software Ltd. All Rights Reserved.

  The contents of this file are provided under the Software License
  Agreement that you accepted before downloading this file.

  This source forms part of the Software and can be used for educational,
  training, and demonstration purposes but cannot be used for derivative
  works except in cases where the derivative works require OVP technology
  to run.

  For open source models released under licenses that you can use for
  derivative works, please visit www.ovpworld.org or www.imperas.com
  for the location of the open source models.


File:Imperas/Examples/HelloWorld/usingSystemC/README.txt

INTRODUCTION -------------------------------------------------------
This example shows a simple Application which will print
    Hello World
to the console
This Application can be cross compiled to any supported processor type
or endian. This example application is compiled for the RISCV and includes
a platform that is written to support the RISCV processor type but can be 
easily modified to support others.

FILES --------------------------------------------------------------
There are three parts to the example
1. An application; this is found as application/application.c
2. A CpuManager/OVPsim SystemC TLM2.0 platform; this is in platform/platform.cpp
3. A Makefile to build the application in 1; this is in application/Makefile,
   In addition a standard Makefile is used to build the platform. This is included
   into platform/Makefile and is found at IMPERAS_HOME/ImperasLib/buildutils/Makefile.TLM.platform

BUILDING THE SIMULATION --------------------------------------------

-- Application
> make -C application CROSS=RISCV32
This will build the application 'application.RISCV32.elf' for processor riscv,
this is also the default.

-- Platform
> make -C platform NOVLNV=1 CROSS=RISCV32

This will build the platform 'platform.IMPERAS_ARCH.exe'

RUNNING THE EXAMPLE ------------------------------------------------

> platform/platform.IMPERAS_ARCH.exe -program application/application.RISCV32.elf 


BUILDING FOR OTHER PROCESSORS (ARM and MIPS32) ----------------------

If the value for 'CROSS' in the call to make the application is replaced with 
either IMG_MIPS32R2 or ARM7TDMI, the elf files which are built will be for the
mips32 or arm processors respectively.

If the value for 'CROSS' in the call to make the platform is replaced with 
either IMG_MIPS32R2 or ARM7TDMI, the platform will include the processor 
definition for the mips32_r1r5 or arm respectively.

The platform modification is to include the processor tlm interface file and 
use the class that it defines.

for example ARM, include the ARM7TDMI interface file

#include "arm.ovpworld.org/processor/arm/1.0/tlm/arm_ARM7TDMI.igen.hpp"

and change the cpu definition to be of this class 

    arm_ARM7TDMI          cpu1;

for example MIPS32, include the 4KEm interface file

#include "mips.ovpworld.org/processor/mips32/1.0/tlm/mips32_r1r5_4KEm.igen.hpp"

and change the cpu definition to be of this class

    mips32_r1r5_4KEm      cpu1;


Be sure to clean from the previous build
> make -C platform NOVLNV=1 CROSS=RISCV32 clean

and build the required platform

The modified platform executable can be run from the command line in the same 
way as the RISC-V. Pass in the correct elf file to execute.
