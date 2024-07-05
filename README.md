# DMA_by_SystemC

This repository demonstrates how to port RISC-V cores on the Imperas M*SDK/OVP platform, using SystemC to simulate DMA behavior for data exchange via shared memory.
The application is divided into two stage (Project1, Project2):

### Project1 (Single DMA, Single core)
### Goal：Implement the DMA module with a SC_CTHREAD process for data exchange via shared memory.
#### SPEC
- DMA controller
  - 32-bit master port developed with TLM 2.0 blocking initiator port.
  - 32-bit slave port developed with TLM 2.0 blocking target port.
  - 1-bit interrupt pin developed with sc_out.
  - 1-bit reset pin developed with sc_in.
  - One static address register to store its base address.
- 4 control registers
  - SOURCE: 32-bit, the starting source address, at 0x0
  - TARGET: 32-bit, the starting target address, at 0x4
  - SIZE: 32-bit, the data size, at 0x8
  - START/CLEAR: 1-bit, the start and clear signal, at 0xC
- Behavior
  1. Through the slave port and according to the addresses received, passively write control data, i.e. source address, target address, size, and start/clear, to four (4) control registers.
  2. Once the value 1 is stored in the START/CLEAR control register (0xC = 1), via the master port actively start moving data from the SOURCE address to the TARGET address until the SIZE is reached.
  3. When the data movement is completed, first pull the interrupt signal (interrupt) to high and wait for the interrupt clear control (START/CLEAR, 0X0C = 0). Only after the CLEAR arrives then the DMA resets the interrupt to low and resume the original state.
  4. The reset pin (reset) is synchronous and active low. At reset, all 4 control registers are cleared as 0 and the interrupt signal is pulled to low.

<img src="https://github.com/xkllkx/DMA_by_SystemC/blob/main/Project1/Project1.png" width="50%" height="50%">

---

### Project2 (Single DMA, Double cores)
### Goal：RISC-V-1 and RISC-V-2 write 1KB data to RAM, then use DMA to transfer it between RAM3 and RAM4, signaling completion via interrupts.
#### SPEC
- System
  - 1 DMA controller uses two sets of control registers for each core with *First-Come-First-Serve arbitration*.
  - 4 1MB RAM blocks
  - 4 TLM 2.0 buses
- Behavior
After setting the addresses for CPU1 and CPU2, the program initiates CPU1. The read and write functions operate similarly to Project 1, but a new address, 0x200800, is introduced to prevent simultaneous read and write operations by CPU1 and CPU2. CPU1 works when the data at address 0x200800 is 0, while CPU2 works when the data is 1.
- Ex：
  1  First iter (RISC-V-1) : dmaBurst -> interruptHandler -> ... for 256 round
  2. Second iter (RISC-V-2) : dmaBurst -> interruptHandler -> ... for 256 round

<img src="https://github.com/xkllkx/DMA_by_SystemC/blob/main/Project2/Project2.png" width="50%" height="50%">



# Essential
- Imperas M*SDK/OVP platform
- systemc-2.3.4

# How to use this repo
### Project1
```bash
cd Project1/application
make clean

cd Project1/platform
make clean

cd Project1
make
./example.sh
```

### Project2
```bash
cd Project2/application
make clean

cd Project2/platform
make clean

cd Project2
make
./run.sh
```
