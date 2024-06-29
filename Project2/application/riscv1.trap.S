/*
 *
 * Copyright (c) 2005-2021 Imperas Software Ltd., www.imperas.com
 *
 * The contents of this file are provided under the Software License
 * Agreement that you accepted before downloading this file.
 *
 * This source forms part of the Software and can be used for educational,
 * training, and demonstration purposes but cannot be used for derivative
 * works except in cases where the derivative works require OVP technology
 * to run.
 *
 * For open source models released under licenses that you can use for
 * derivative works, please visit www.OVPworld.org or www.imperas.com
 * for the location of the open source models.
 *
 */


#ifndef TRAP_S
#define TRAP_S

  .section      .text.entry
  .globl trap_entry


trap_entry:
  addi sp, sp, -32*4

  sw x1, 0 * 4(sp)
  sw x2, 1 * 4(sp)
  sw x3, 2 * 4(sp)
  sw x4, 3 * 4(sp)
  sw x5, 4 * 4(sp)
  sw x6, 5 * 4(sp)
  sw x7, 6 * 4(sp)
  sw x8, 7 * 4(sp)
  sw x9, 8 * 4(sp)
  sw x10, 9 * 4(sp)
  sw x11, 10 * 4(sp)
  sw x12, 11 * 4(sp)
  sw x13, 12 * 4(sp)
  sw x14, 13 * 4(sp)
  sw x15, 14 * 4(sp)
  sw x16, 15 * 4(sp)
  sw x17, 16 * 4(sp)
  sw x18, 17 * 4(sp)
  sw x19, 18 * 4(sp)
  sw x20, 19 * 4(sp)
  sw x21, 20 * 4(sp)
  sw x22, 21 * 4(sp)
  sw x23, 22 * 4(sp)
  sw x24, 23 * 4(sp)
  sw x25, 24 * 4(sp)
  sw x26, 25 * 4(sp)
  sw x27, 26 * 4(sp)
  sw x28, 27 * 4(sp)
  sw x29, 28 * 4(sp)
  sw x30, 29 * 4(sp)
  sw x31, 30 * 4(sp)

  jal interruptHandler

  lw x1, 0 * 4(sp)
  lw x2, 1 * 4(sp)
  lw x3, 2 * 4(sp)
  lw x4, 3 * 4(sp)
  lw x5, 4 * 4(sp)
  lw x6, 5 * 4(sp)
  lw x7, 6 * 4(sp)
  lw x8, 7 * 4(sp)
  lw x9, 8 * 4(sp)
  lw x10, 9 * 4(sp)
  lw x11, 10 * 4(sp)
  lw x12, 11 * 4(sp)
  lw x13, 12 * 4(sp)
  lw x14, 13 * 4(sp)
  lw x15, 14 * 4(sp)
  lw x16, 15 * 4(sp)
  lw x17, 16 * 4(sp)
  lw x18, 17 * 4(sp)
  lw x19, 18 * 4(sp)
  lw x20, 19 * 4(sp)
  lw x21, 20 * 4(sp)
  lw x22, 21 * 4(sp)
  lw x23, 22 * 4(sp)
  lw x24, 23 * 4(sp)
  lw x25, 24 * 4(sp)
  lw x26, 25 * 4(sp)
  lw x27, 26 * 4(sp)
  lw x28, 27 * 4(sp)
  lw x29, 28 * 4(sp)
  lw x30, 29 * 4(sp)
  lw x31, 30 * 4(sp)

  addi sp, sp, 32*4
  mret

#endif /* TRAP_S */

