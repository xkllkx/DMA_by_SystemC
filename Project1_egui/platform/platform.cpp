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

#include "systemc.h"
#include "tlm/tlmModule.hpp"
#include "tlm/tlmDecoder.hpp"
#include "tlm/tlmMemory.hpp"

// Processor configuration
#ifdef RISCV32
#include "riscv.ovpworld.org/processor/riscv/1.0/tlm/riscv_RV32I.igen.hpp"
#endif
#ifdef ARM7TDMI
#include "arm.ovpworld.org/processor/arm/1.0/tlm/arm_ARM7TDMI.igen.hpp"
#endif
#ifdef IMG_MIPS32R2
#include "mips.ovpworld.org/processor/mips32_r1r5/1.0/tlm/mips32_r1r5_4KEm.igen.hpp"
#endif

using namespace sc_core;

////////////////////////////////////////////////////////////////////////////////
//                                  DMA                                       //
////////////////////////////////////////////////////////////////////////////////

// ! DMA

SC_MODULE(DMA) {
    /// CPU (master) -> DMA (slave)
    tlm_utils::simple_target_socket<DMA> slave_C2D; // slave
    /// DMA (master) <-> MEM (slave)
    tlm_utils::simple_initiator_socket<DMA> master_D2M; // master
    
    /// interrupt port
    // sc_out<bool> interrupt;
    tlm::tlm_analysis_port<unsigned int> interrupt;

    sc_in<bool> clk, reset;
    sc_signal<sc_uint<32> > SOURCE, TARGET, SIZE;
    sc_signal<bool> START_CLEAR;

    sc_signal<sc_uint<32> > offset;

    void dma_p();
    void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);

    /// buffer
    unsigned int data;
    /// address
    unsigned int dma_base_addr;
    unsigned int regmem[4];

    DMA(sc_module_name _name, unsigned int _address=0x00000000): sc_module(_name), slave_C2D("slave_C2D"), master_D2M("master_D2M"), dma_base_addr(_address)
    {
        SC_HAS_PROCESS(DMA);
        
        // Register callback for incoming b_transport interface method call
        slave_C2D.register_b_transport(this, &DMA::b_transport);
        
        SC_CTHREAD(dma_p, clk.pos());
        async_reset_signal_is(reset, false);
    }
};

void DMA::dma_p() { /// DMA (master) <-> MEM (slave)
    SOURCE = 0x0;
    TARGET = 0x0;
    SIZE = 0x0;
    START_CLEAR = false;
    // interrupt = false;
    interrupt.write(0);

    for(int i=0;i<4;i++)
        regmem[i] = 0;

    offset = 0;

    tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload;
    sc_time delay = sc_time(10, SC_NS);

    unsigned int addr_s, addr_t, data_s, data_t;

    while(1) {
        wait();

        if(!START_CLEAR.read()) { // The SOURCE, TARGET, SIZE info is not recieved yet.
            // Recieve SOURCE, TARGET, SIZE info from CPU.

            SOURCE = regmem[0];
            TARGET = regmem[1];
            SIZE = regmem[2];
            START_CLEAR = regmem[3];
            
            offset = 0;
            // interrupt = false;
            interrupt.write(0);
        }
        // The SOURCE, TARGET, SIZE info is recieved and START_CLEAR is on.
        else if(SIZE.read()-offset.read() > 0) {
            addr_s = SOURCE.read();
            addr_t = TARGET.read();

            // Read data from source address.
            cout << "Read from source: " << endl;
            // tlm::tlm_command cmd = tlm::TLM_READ_COMMAND;
            tlm::tlm_command cmd = static_cast<tlm::tlm_command>(0);
            trans->set_command( cmd );
            trans->set_address( addr_s+offset.read() );
            trans->set_data_ptr( reinterpret_cast<unsigned char*>(&data_s) );
            trans->set_data_length( 4 );
            trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
            trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
            trans->set_dmi_allowed( false ); // Mandatory initial value
            trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
            master_D2M->b_transport( *trans, delay );  // Blocking transport call
            // Initiator obliged to check response status and delay
            if ( trans->is_response_error() )
                SC_REPORT_ERROR("TLM-2", "Response error from b_transport");

            cout << "trans = { " << (cmd ? 'W' : 'R') << ", " << hex << addr_s+offset.read()
                << " } , data_s = " << hex << data_s << " at time " << sc_time_stamp()
                << " delay = " << delay << endl;

            data_t = data_s;    // Data transfer.
            
            // Write data to source address.
            cout << "Write to target: " << endl;
            // cmd = tlm::TLM_WRITE_COMMAND;
            cmd = static_cast<tlm::tlm_command>(1);
            trans->set_command( cmd );
            trans->set_address( addr_t+offset.read() );
            trans->set_data_ptr( reinterpret_cast<unsigned char*>(&data_t) );
            trans->set_data_length( 4 );
            trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
            trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
            trans->set_dmi_allowed( false ); // Mandatory initial value
            trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
            master_D2M->b_transport( *trans, delay );  // Blocking transport call
            // Initiator obliged to check response status and delay
            if ( trans->is_response_error() )
                SC_REPORT_ERROR("TLM-2", "Response error from b_transport");

            cout << "trans = { " << (cmd ? 'W' : 'R') << ", " << hex << addr_t+offset.read()
                << " } , data_t = " << hex << data_t << " at time " << sc_time_stamp()
                << " delay = " << delay << endl;
            
            offset = offset.read() + 4;
            // interrupt = false;
        }
        else {
            START_CLEAR = regmem[3];

            // interrupt = true;
            interrupt.write(1);
            
            // cout << "Finish" << endl;
        }
    }

    return;
};

void DMA::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
    // TLM-2 blocking transport method

    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64    adr = trans.get_address();
    unsigned char*   ptr = trans.get_data_ptr();
    // unsigned int     len = trans.get_data_length();
    // unsigned char*   byt = trans.get_byte_enable_ptr();
    // unsigned int     wid = trans.get_streaming_width();

    // Obliged to check address range and check for unsupported features,
    //   i.e. byte enables, streaming, and bursts
    // Can ignore DMI hint and extensions
    // Using the SystemC report handler is an acceptable way of signalling an error

    if(cmd == tlm::TLM_WRITE_COMMAND)
    {
        // Obliged to implement read and write commands
        memcpy(regmem+(adr-dma_base_addr)/4, ptr, sizeof(int));
        
        cout << "dma receive: ";
        for(int i=0; i<4; i+=1)
            cout << regmem[i] <<" ";
        cout << endl;

        wait(delay);
    }

    // Obliged to set response status to indicate successful completion
    trans.set_response_status( tlm::TLM_OK_RESPONSE );

    return;
};

////////////////////////////////////////////////////////////////////////////////
//                      BareMetal Class                                       //
////////////////////////////////////////////////////////////////////////////////

class BareMetal : public sc_module {
public:
    BareMetal (sc_module_name name);

    tlmModule             Platform;
    tlmDecoder            bus1;
    tlmRam                ram1;
#ifdef RISCV32
    riscv_RV32I           cpu1;
#endif
#ifdef ARM7TDMI
    arm_ARM7TDMI          cpu1;
#endif
#ifdef IMG_MIPS32R2
    mips32_r1r5_4KEm      cpu1;
#endif 
    DMA                   dma;
private:

    params paramsForcpu1() {
        params p;
        p.set("defaultsemihost", true);
        return p;
    }

}; /* BareMetal */

BareMetal::BareMetal (sc_module_name name)
    : sc_module (name)
    , Platform ("")
    , bus1 (Platform, "bus1", 3, 2) // ! master:3 slave:2
    , ram1 (Platform, "ram1", 0xffffffef) // ! DMA | 0x0  0x00000010 | , MEM | 0x00000010 0xffffffff |
    , cpu1 (Platform, "cpu1",  paramsForcpu1())
    , dma("dma", 0x00000000)
{
    bus1.connect(cpu1.INSTRUCTION);
    bus1.connect(cpu1.DATA);
    bus1.connect(ram1.sp1, 0x00000010, 0xffffffff);
    //-------------------------------------
    //-----slave port (DMA)-----
    tlmDecoder::initiatorSocketType* bus_master_port = bus1.nextInitiatorSocket(0x00000000, 0x00000010);
    bus_master_port->bind(dma.slave_C2D);
    //-----master port (DMA)------
    dma.master_D2M.bind(*bus1.nextTargetSocket());
    //-------------------------------------
    dma.interrupt(cpu1.MExternalInterrupt);
    //-------------------------------------
}

////////////////////////////////////////////////////////////////////////////////
//                               SC_MAIN                                      //
////////////////////////////////////////////////////////////////////////////////

int sc_main (int argc, char *argv[]) {
    // start the CpuManager session
    session s;

    // create a standard command parser and parse the command line
    parser  p(argc, (const char**) argv);

    // create an instance of the platform
    BareMetal top ("top");
    
	sc_clock clk("clk", 500, SC_NS);
	sc_signal<bool> rst;
    
	top.dma.reset(rst);
	top.dma.clk(clk);
    
	rst.write(true);

    // start SystemC
    sc_start();
    return 0;
}

