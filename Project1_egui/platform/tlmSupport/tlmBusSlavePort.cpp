/*
 *
 * Copyright (c) 2005-2021 Imperas Software Ltd., All Rights Reserved.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND TRADE SECRETS
 * OF IMPERAS SOFTWARE LTD. USE, DISCLOSURE, OR REPRODUCTION IS PROHIBITED
 * EXCEPT AS MAY BE PROVIDED FOR IN A WRITTEN AGREEMENT WITH
 * IMPERAS SOFTWARE LTD.
 *
 */

#include "tlm/tlmBusPort.hpp"
#include "tlm/tlmBusPort.hpp"
#include "tlm/tlmProcessor.hpp"
#include "tlm/tlmPeripheral.hpp"
#include "tlm/tlmMmc.hpp"

namespace cw {

const char *tlmBusSlavePort::hierCName(void) {
    if(peripheralParent) {
        return peripheralParent->hierCName();
    }
    if(mmcParent) {
        return mmcParent->hierCName();
    }
    return "(unknown name)";
}

bool tlmBusSlavePort::traceBuses(void) {
    if(peripheralParent) {
        return peripheralParent->traceBuses();
    }
    if(mmcParent) {
        return mmcParent->traceBuses();
    }
    return False;
}

tlmBusSlavePort::tlmBusSlavePort(tlmModule &parent, tlmPeripheral *p, std::string name, Addr b)
    : bus1            (parent, p->hierName() + "" + name, 32)
    , bytes           (b)
    , peripheralParent(p)
    , mmcParent       (0)
{
    socket.register_b_transport  (this, &cw::tlmBusSlavePort::b_transport);
    socket.register_transport_dbg(this, &cw::tlmBusSlavePort::debug_transport);
    bus1.connectSlave(*p, name, 0, b-1);
}

tlmBusSlavePort::tlmBusSlavePort(tlmModule &parent, tlmPeripheral *p, std::string name)
    : bus1            (parent, p->hierName() + "_" + name, 32)
    , bytes           (0xffffffffffffffffLL)
    , peripheralParent(p)
    , mmcParent       (0)
{
    socket.register_b_transport  (this, &cw::tlmBusSlavePort::b_transport);
    socket.register_transport_dbg(this, &cw::tlmBusSlavePort::debug_transport);
    bus1.connectSlave(*p, name, 0, 0xffffffffffffffffLL);
}

tlmBusSlavePort::tlmBusSlavePort(tlmModule &parent, tlmMMC *p, std::string name)
    : bus1            (parent, p->hierName() + "_" + name, 32)
    , bytes           (0xffffffffffffffffLL)
    , peripheralParent(0)
    , mmcParent       (p)
{
    socket.register_b_transport  (this, &cw::tlmBusSlavePort::b_transport);
    socket.register_transport_dbg(this, &cw::tlmBusSlavePort::debug_transport);
    bus1.connect(*p, name);
}

void tlmBusSlavePort::transport(tlm_generic_payload &payload)
{
    Addr            address  = payload.get_address();     // memory address
    tlm_command     command  = payload.get_command();     // memory command
    unsigned char  *data     = payload.get_data_ptr();    // data pointer
    unsigned  int   length   = payload.get_data_length(); // data length
    bool            ok       = true;

    tlmInitiatorExtension *ext;
    payload.get_extension(ext);

    if(ext) {
        processor *proc = ext->getProcessor(); // (processor*)ext->initiator;

        // does the access go beyond max port address?
        if (address + length > bytes) {
            ok = false;
        }

        // Sanity check. Is an initator making an unreasonably long access (16 is a guess)?
        Uns32 max = 16;
        if (length > max) {
           if (traceBuses()) {
                 opPrintf("TLM: %s access [" FMT_Ax "] exceeds maximum number of bytes (%u), raising a TLM error\n", hierCName(), address, max);
            }
           ok = false;
        }

        switch(command) {
            case TLM_WRITE_COMMAND:
                if (traceBuses()) {
                    opPrintf("TLM: %s write [" FMT_Ax "] ", hierCName(), address);
                    tlmDiagnostics::printBytes(data, length);
                    opPrintf("\n");
                }
                bus1.write(proc, address, (void*)data, length, false);
                break;
            case TLM_READ_COMMAND:
                bus1.read(proc, address, (void*)data, length, false);
                if (traceBuses()) {
                    opPrintf("TLM: %s read  %d bytes [" FMT_Ax "] ", hierCName(),  length, address);
                    tlmDiagnostics::printBytes(data, length);
                    opPrintf("\n");
                }
                break;
            default:
                ok = false;
        }
    } else {
        ok =false;
    }
    payload.set_response_status(ok ? TLM_OK_RESPONSE : TLM_ADDRESS_ERROR_RESPONSE);
}

// target regular method
void tlmBusSlavePort::b_transport( tlm_generic_payload &payload, sc_time &delay)
{
    transport(payload);
}

// target debug method
Uns32 tlmBusSlavePort::debug_transport(tlm_generic_payload &payload)
{
    transport(payload);
    return 1;
}

} // end namespace cw
