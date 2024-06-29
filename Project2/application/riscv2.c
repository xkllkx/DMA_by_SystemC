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

#define DMA_BASE                  0x100000
#define DMA_SOURCE_ADDR1          0x00
#define DMA_TARGET_ADDR1          0x04
#define DMA_SIZE_ADDR1            0x08
#define DMA_CLEAR_ADDR1           0x0c
#define DMA_SOURCE_ADDR2          0x10
#define DMA_TARGET_ADDR2          0x14
#define DMA_SIZE_ADDR2            0x18
#define DMA_CLEAR_ADDR2           0x1c

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

static void dmaBurst2(void *from, void *to, Uns32 bytes)
{
    printf("dmaBurst 2, bytes:%d\n", bytes);
    //printf("SRC: %u\n", (Uns32)from);
    writeReg32(DMA_BASE, DMA_SOURCE_ADDR2, (Uns32)from);
    writeReg32(DMA_BASE, DMA_TARGET_ADDR2, (Uns32)to);
    writeReg32(DMA_BASE, DMA_SIZE_ADDR2, bytes);
    writeReg32(DMA_BASE, DMA_CLEAR_ADDR2, 0x1);
}
// interrupt----
volatile static Uns32 interruptCount = 0;

void interruptHandler(void)
{
    if (readReg8(DMA_BASE, DMA_CLEAR_ADDR2))  //interrupt2
    {
        printf("Interrupt 2 0x%x (0x%02x)\n", readReg8(DMA_BASE, DMA_CLEAR_ADDR2), 0x1);
        writeReg32(DMA_BASE, DMA_CLEAR_ADDR2, 0x0); //Pull down START/CLEAR
    }
}
//--------------------------------------
int main () {

    printf("[CPU2]Start\n");

    //static unsigned int* src_adr1 = (int*)0x200400;  //SOURCE address
    static unsigned int* adr_a = (int*)0x200400;
    static unsigned int* adr_b = (int*)0x300400;
    static unsigned int* adr_c = (int*)0x300000;
    static unsigned int* adr_d = (int*)0x200000;
    unsigned int offset, data;
    
    //RISCV-2 writes 1KB data into 0x300400 to 0x3007FF.
    printf("[CPU2]Writing data to 0x300400 ~ 0x3007FF...\n");
    offset = 0x0;
    data = 0xf;
    for(unsigned int i=0; i<1024; i+=1)
    {
        //Transfer every bytes(8 bits, two data)
        printf("Write %x at %x\n", (data*16 + data+1), adr_a+offset);
        writeReg8((Uns32)adr_b, offset, (data*16 + data-1));
        offset += 1;
        data = (data == 0x1)? 0xf: data-2;
    }
    
    int_init(trap_entry);
    int_enable();
    
    for(int k=0; k<3; k+=1)
    {
        //Monitor data in 0x200800.
        printf("[CPU2]Wait for data in 0x200800 to be 0x1\n");
        while(readReg8(0x200800, 0) != (Uns32)0x1)
            /*printf("[CPU2]\tWaiting...\n")*/;
        
        //RISCV-2 programs DMA’s 2nd set of control registers to move a 1KB data starting at 0x300400 (on RAM4) to 0x200000 (on RAM3).
        for(unsigned int i=0; i<256; i+=1)
        {
            dmaBurst2(adr_b+i, adr_d+i, 4);
            wfi();
        }
        writeReg8(0x200800, 0, 0x0);    //RISCV-2  writes a flag 0x0 to address 0x200800 to indicate the end of data transmission.
    }

    printf("[CPU2]Finish\n");
}
