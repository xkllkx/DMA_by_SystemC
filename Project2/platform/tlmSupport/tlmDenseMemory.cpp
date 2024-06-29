/**********************************************************************
  The following code is derived, directly or indirectly, from the SystemC
  source code Copyright (c) 1996-2008 by all Contributors.
  All Rights reserved.

  The contents of this file are subject to the restrictions and limitations
  set forth in the SystemC Open Source License Version 3.0 (the "License");
  You may not use this file except in compliance with such restrictions and
  limitations. You may obtain instructions on how to receive a copy of the
  License at http://www.systemc.org/. Software distributed by Contributors
  under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
  ANY KIND, either express or implied. See the License for the specific
  language governing rights and limitations under the License.
 *********************************************************************/

/* Derived from TLM examples memory.cpp by Imperas Software LTD.
 *
 * This is an example of a memory model with a TLM interface.
 *
 * It is not sparse, so requires as much virtual memory as it's size
 * in bytes.. For example, on the 32bit host a memory > 2G will generally
 * fail to load.
 *
 * The memory supports DMI, which is turned ON by default.
 *
 * To turn off DMI and invalidate DMI active regions.
 * instance.stopDMI();
 *
 * To turn DMI on again.
 * instance.startDMI();
 */

#include "tlm.h"                                // TLM headers
#include "tlm_utils/simple_target_socket.h"

#include "op/op.hpp"
#include "tlm/tlmDenseMemory.hpp"

using namespace std;
using namespace sc_core;

namespace cw {

tlmMemory::tlmMemory
(
      sc_dt::uint64      high_address        // memory size (bytes)
    , unsigned int       memory_width        // memory width (bytes)
)
    : m_high_address     (high_address)
    , m_memory_width     (memory_width)
{
    size_t high = (size_t) m_high_address;
    size_t size = high+1;

    if (size && (m_high_address==high)) {
        m_memory = new unsigned char[size];
    } else {
        m_memory = 0;
    }
    if (!m_memory) {
        cout << "TLM Memory Error: Cannot allocate tlmMemory with bounds 0x0:0x" << hex << high_address << endl;
        abort();
    }
    memset(m_memory, 0, size);
}

void tlmMemory::operation
(
     tlm::tlm_generic_payload  &gp
   , sc_time          &delay_time   ///< transaction delay
) {
    sc_dt::uint64    address   = gp.get_address();     // memory address
    tlm::tlm_command command   = gp.get_command();     // memory command
    unsigned char    *data     = gp.get_data_ptr();    // data pointer
    unsigned  int     length   = gp.get_data_length(); // data length

    tlm::tlm_response_status response_status = check_address(gp)
            ? tlm::TLM_OK_RESPONSE
            : tlm::TLM_ADDRESS_ERROR_RESPONSE;

    if (gp.get_byte_enable_ptr()) {
        gp.set_response_status(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
    }
    else if (gp.get_streaming_width() != gp.get_data_length()) {
        gp.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
    }
    switch (command)
    {
        default:
        {
            gp.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            break;
        }

        case tlm::TLM_WRITE_COMMAND:
        {
            if (response_status == tlm::TLM_OK_RESPONSE) {
                for (unsigned int i = 0; i < length; i++) {
                    m_memory[address++] = data[i];        // move the data to memory
                }
            }
            break;
        }

        case tlm::TLM_READ_COMMAND: {
            if (response_status == tlm::TLM_OK_RESPONSE) {
                for (unsigned int i = 0; i < length; i++)
                {
                    data[i] = m_memory[address++];         // move the data from memory
                }
            }
            break;
        }
    } // end switch
    gp.set_response_status(response_status);
}

unsigned int tlmMemory::debug
(
     tlm::tlm_generic_payload  &gp
   , sc_time          &delay_time   ///< transaction delay
) {
    sc_dt::uint64    address   = gp.get_address();     // memory address
    tlm::tlm_command command   = gp.get_command();     // memory command
    unsigned char    *data     = gp.get_data_ptr();    // data pointer
    unsigned  int     length   = gp.get_data_length(); // data length
    bool              ok       = check_address(gp);

    if(ok) {
        switch (command)
        {
            case tlm::TLM_WRITE_COMMAND:
                for (unsigned int i = 0; i < length; i++) {
                    m_memory[address++] = data[i];         // move the data to memory
                }
                gp.set_response_status(tlm::TLM_OK_RESPONSE);
                break;

            case tlm::TLM_READ_COMMAND:
                for (unsigned int i = 0; i < length; i++) {
                    data[i] = m_memory[address++];         // move the data from memory
                }
                gp.set_response_status(tlm::TLM_OK_RESPONSE);
                break;

            default:
                ok = 0;
                break;

        }
    } // end switch

    return ok ? length : 0;
}

unsigned char* tlmMemory::get_mem_ptr(void)
{
    return m_memory;
}


//==============================================================================
///  @fn memory::check_address
//
///  @brief Method to check if the gp is in the address range of this memory
//
///  @details
///    This routine used to check for errors in address space
//
//==============================================================================
bool tlmMemory::check_address (
    tlm::tlm_generic_payload  &gp
) {
    sc_dt::uint64    address   = gp.get_address();     // memory address
    unsigned  int     length   = gp.get_data_length(); // data length

    if  ( (address < 0) || (address > m_high_address) ) {
        return 0;
    } else if ( (address + length-1) > m_high_address ) {
        return 0;
    } else {
        return 1;
    }
} // end check address

//SC_HAS_PROCESS(tlmRam);

tlmRam::tlmRam (
      tlmModule      &parent
    , sc_module_name module_name    // module name
    , sc_dt::uint64  high_address            // memory size (bytes)
    , unsigned int   memory_width            // memory width (bytes)
)
    : sc_module        (module_name)             /// init module name
    , sp1              ("sp1")                   /// init socket name
    , m_high_address   (high_address)            /// init memory size (bytes-1)
    , m_memory_width   (memory_width)            /// init memory width (bytes)
    , m_target_memory  (high_address, memory_width)
    , dmiOn            (true)
{
    sp1.register_b_transport(this, &tlmRam::custom_b_transport);
    sp1.register_transport_dbg(this, &tlmRam::debug_transport);
    sp1.register_get_direct_mem_ptr(this, &tlmRam::get_direct_mem_ptr);
}

void tlmRam::custom_b_transport( tlm::tlm_generic_payload &payload, sc_time &delay_time)
{
    sc_time  mem_op_time;
    payload.set_dmi_allowed(dmiOn);
    m_target_memory.operation(payload, mem_op_time);
    return;
}

unsigned int  tlmRam::debug_transport (tlm::tlm_generic_payload &payload)
{
    sc_time  mem_op_time;
    payload.set_dmi_allowed(dmiOn);
    return m_target_memory.debug(payload, mem_op_time);
}

bool tlmRam::get_direct_mem_ptr(tlm::tlm_generic_payload &trans, tlm::tlm_dmi &dmi_data)
{
    sc_dt::uint64 address = trans.get_address();
    if (address > m_high_address) {
        // should not happen
        cerr << "tlmRam::get_direct_mem_ptr: address overflow";
        return false;
    }

    // if (m_invalidate) m_invalidate_dmi_event.notify(m_invalidate_dmi_time);
    dmi_data.allow_read_write();
    dmi_data.set_start_address(0x0);
    dmi_data.set_end_address(m_high_address);

    unsigned char *ptr = m_target_memory.get_mem_ptr();
    trans.set_dmi_allowed(dmiOn);
    dmi_data.set_dmi_ptr(ptr);
    return true;
}

void tlmRam::dmi(bool on) {
    dmiOn = on;
    if(!on) {
        sp1->invalidate_direct_mem_ptr(0, m_high_address);
    }
}

} // end namespace cw
