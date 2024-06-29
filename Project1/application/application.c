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

typedef unsigned int  Uns32;
typedef unsigned char Uns8;
//typedef bool Uns1;

#define DMA_BASE                  0x00000000
#define DMA_SOURCE_ADDR           0x00
#define DMA_TARGET_ADDR           0x04
#define DMA_SIZE_ADDR             0x08
#define DMA_CLEAR_ADDR            0x0c
#define DMA_INT_TC_STATUS         0x10

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "riscvInterrupts.h"

void int_init(void (*handler)()) {

	// Set MTVEC register to point to handler function in direct mode
	int handler_int = (int) handler & ~0x1;
	write_csr(mtvec, handler_int);

	// Enable Machine mode external interrupts
    set_csr(mie, MIE_MEIE);
}

void int_enable() {
    set_csr(mstatus, MSTATUS_MIE);
}
// r/w func-----------
static inline void writeReg32(Uns32 address, Uns32 offset, Uns32 value)
{
    *(volatile Uns32*) (address + offset) = value;
}
static inline Uns32 readReg32(Uns32 address, Uns32 offset)
{
    return *(volatile Uns32*) (address + offset);
}
static inline void writeReg8(Uns32 address, Uns32 offset, Uns8 value)
{
    *(volatile Uns8*) (address + offset) = value;
}
static inline Uns8 readReg8(Uns32 address, Uns32 offset)
{
    return *(volatile Uns8*) (address + offset);
}
//static inline void writeReg(Uns32 address, Uns32 offset, bool value)
//{
//    *(volatile Uns1*) (address + offset) = value;
//}
//dma-----------
static void dmaBurst(void *from, void *to, Uns32 bytes)
{
    printf("dmaBurst bytes:%d\n", bytes);
    printf("SRC: %u\n", (Uns32)from);
    writeReg32(DMA_BASE, DMA_SOURCE_ADDR, (Uns32)from); // ! DMA_BASE + DMA_SOURCE_ADDR
    writeReg32(DMA_BASE, DMA_TARGET_ADDR, (Uns32)to);
    writeReg32(DMA_BASE, DMA_SIZE_ADDR, bytes);
    writeReg32(DMA_BASE, DMA_CLEAR_ADDR, 0x1);
}
// interrupt----
volatile static Uns32 interruptCount = 0;

void interruptHandler(void)
{
    Uns32 intStatus = 0x1;//readReg8(DMA_BASE, DMA_INT_TC_STATUS);    // read interrupt status

    // check channel 0 interrupts enabled and status indicates interrupt set
    if (intStatus && readReg8(DMA_BASE, DMA_CLEAR_ADDR))
    {
        printf("Interrupt ch0 0x%x (0x%02x)\n", readReg8(DMA_BASE, DMA_CLEAR_ADDR), intStatus);
        writeReg32(DMA_BASE, DMA_CLEAR_ADDR, 0x0);      // disable ch0 interrupt
        writeReg32(DMA_BASE, DMA_INT_TC_STATUS, 0x0);   // clear ch0 interrupt
        interruptCount++;
    }
}
//--------------------------------------
int main () {
    static unsigned int* src_adr = (int*)0xfff00000;  //SOURCE address
    writeReg32((Uns32)src_adr, 0x0, 123);
    writeReg32((Uns32)src_adr, 0x4, 456); 
    writeReg32((Uns32)src_adr, 0x8, 789);
    writeReg32((Uns32)src_adr, 0xc, 818);
    
    static unsigned int* dst_adr = (int*)0xffff0000;  //TARGET address
    writeReg32((Uns32)dst_adr, 0x0, 0);
    writeReg32((Uns32)dst_adr, 0x4, 0);
    writeReg32((Uns32)dst_adr, 0x8, 0);
    writeReg32((Uns32)dst_adr, 0xc, 0);

    // int data_length[3] = {1, 1, 2};
    int data_length = 3;

    int_init(trap_entry); // ! in trap.S
    int_enable();

    dmaBurst(src_adr,dst_adr, data_length*sizeof(int));

    printf("Waiting for interrupts\n");
    wfi(); // ! in riscvInterrupts.h

    /* check results */
    printf("DMA result dst1:\n");
    
    for(int j =0; j<4; j++) {
        printf("'%u' ", *(dst_adr + j));
	printf("\n");
    }

    printf("Hello World\n");
}
