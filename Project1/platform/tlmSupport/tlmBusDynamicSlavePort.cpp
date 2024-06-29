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

#include "tlm/tlmBusPort.hpp"
#include "tlm/tlmMmc.hpp"
#include "tlm/tlmPeripheral.hpp"

namespace cw {

bool tlmBusDynamicSlavePort::traceBuses(void) {
    if(peripheralParent) {
        return peripheralParent->traceBuses();
    }
    if(mmcParent) {
        return mmcParent->traceBuses();
    }
    return False;
}

tlmBusDynamicSlavePort::tlmBusDynamicSlavePort(tlmModule &parent, tlmPeripheral *pse, std::string name, Uns32 addrBitsIgnored)
    : bus1            (parent, pse->hierName() + "." + name, 64)    // Artifact bus, size must contain mapped port so use max
    , peripheralParent(pse)
    , mmcParent       (0)
    , mappings        (0)
    , socket          (/* makes it's own name*/)
{
    socket.register_b_transport  (this, &tlmBusDynamicSlavePort::b_transport);
    socket.register_transport_dbg(this, &tlmBusDynamicSlavePort::debug_transport);
    bus1.connectSlave(*peripheralParent, name);
    peripheralParent->addPortMapCB(name, staticSetPortAddress, (void*)this);
}

void tlmBusDynamicSlavePort::transport(tlm_generic_payload &payload)
{
    Addr            address  = payload.get_address();     // memory address
    tlm_command     command  = payload.get_command();     // memory command
    unsigned char  *data     = payload.get_data_ptr();    // data pointer
    unsigned  int   length   = payload.get_data_length(); // data length
    bool            ok       = true;

    tlmInitiatorExtension *ext;
    payload.get_extension(ext);

    if(ext) {
        processor *proc = ext->getProcessor();

        if (traceBuses()) {
            opPrintf("TLM: %s try [" FMT_Ax "] \n", bus1.name().c_str(), address);
        }

        tlmDynamicPortMapping *mapping;
        for (mapping = mappings; mapping; mapping = mapping->getNext()) {
            if (mapping->inRegion(address)) {
                switch(command) {
                    case TLM_WRITE_COMMAND:
                        if (traceBuses()) {
                            opPrintf("TLM: %s write [" FMT_Ax "] ", bus1.name().c_str(), address);
                            tlmDiagnostics::printBytes(data, length);
                            opPrintf("\n");
                        }
                        bus1.write(proc, address, (void*)data, length, false);
                        break;
                    case TLM_READ_COMMAND:
                        bus1.read(proc, address, (void*)data, length, false);
                        if (peripheralParent->traceBuses()) {
                            opPrintf("TLM: %s read  %d bytes [" FMT_Ax "] ", bus1.name().c_str(), length, address);
                            tlmDiagnostics::printBytes(data, length);
                            opPrintf("\n");
                        }
                        break;
                    default:
                        ok = false;
                }
                if (ok) {
                    payload.set_response_status(TLM_OK_RESPONSE);
                }
                break;
            }
        }
    }
}

// target regular method
void tlmBusDynamicSlavePort::b_transport( tlm::tlm_generic_payload &payload, sc_time &delay)
{
    transport(payload);
}

// target debug method
unsigned int tlmBusDynamicSlavePort::debug_transport(tlm::tlm_generic_payload &payload)
{
    transport(payload);
    return 1;
}

void tlmBusDynamicSlavePort::staticSetPortAddress(void *userData, Addr lo, Addr hi, Bool map) {
    tlmBusDynamicSlavePort *ptr = (tlmBusDynamicSlavePort*)userData;
    ptr->setPortAddress(lo, hi, map);
}

void tlmBusDynamicSlavePort::setPortAddress(Addr lo, Addr hi, Bool map) {
    if (traceBuses()) {
        const char *what = map ? "mapping" : "unmapping";
        opPrintf("TLM: %s %s [" FMT_Ax ":" FMT_Ax "]\n", bus1.name().c_str(), what, lo, hi);
    }
    if(map) {
        tlmDynamicPortMapping *mapping = new tlmDynamicPortMapping(lo,hi);
        mapping->setNext(mappings);
        mappings = mapping;
    } else {
        tlmDynamicPortMapping **old = &mappings;
        while(*old) {
            if ((*old)->matches(lo, hi)) {
                *old = (*old)->getNext();
                break;
            }
            old = (*old)->getNextP();
        }
    }
}

} // end namespace cw
