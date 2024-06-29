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
#include "tlm/tlmProcessor.hpp"
#include "tlm/tlmPeripheral.hpp"
#include "tlm/tlmMmc.hpp"

namespace cw {

tlmDMIslave::tlmDMIslave(bus &b, std::string name, tlmProcessor *proc, socketType &s)
   : busDMISlave(b, proc->hierName() + name, proc, true)
   , sock(s)
   , delay(SC_ZERO_TIME)
   , processorParent(proc)
   , peripheralParent(0)
   , mmcParent(0)
   , tmpr()
   , tmpw()
   { }

tlmDMIslave::tlmDMIslave(bus &b, std::string name, tlmPeripheral *periph, socketType &s)
   : busDMISlave(b, periph->hierName() + name, true)
   , sock(s)
   , delay(SC_ZERO_TIME)
   , processorParent(0)
   , peripheralParent(periph)
   , mmcParent(0)
   , tmpr()
   , tmpw()
   { }

tlmDMIslave::tlmDMIslave(bus &b, std::string name, tlmMMC *mmc, socketType &s)
   : busDMISlave(b, mmc->hierName() + name, true)
   , sock(s)
   , delay(SC_ZERO_TIME)
   , processorParent(0)
   , peripheralParent(0)
   , mmcParent(mmc)
   , tmpr()
   , tmpw()
   { }


void *tlmDMIslave::getDMI(
    Addr        addr,
    Uns32       bytes,
    Addr       &addrLo,
    Addr       &addrHi,
    DMIaccess  &rw,
    void       *userData
) {
    bool ok, rok, wok;

    tlm_generic_payload   trans;

    trans.set_address(addr);
    trans.set_read();
    trans.set_data_length(bytes);
    tmpr.set_granted_access(tlm_dmi::DMI_ACCESS_NONE);
    tmpr.set_dmi_ptr(0);
    ok   = sock->get_direct_mem_ptr(trans, tmpr);
    rok  = ok && tmpr.is_read_allowed();

    trans.set_address( addr );
    trans.set_write();
    trans.set_data_length(bytes);

    tmpw.set_granted_access(tlm_dmi::DMI_ACCESS_NONE);
    tmpw.set_dmi_ptr(0);
    ok  = sock->get_direct_mem_ptr(trans, tmpw);
    wok = ok && tmpw.is_write_allowed();

    Addr  lor  = tmpr.get_start_address();
    Addr  hir  = tmpr.get_end_address();
    Addr  low  = tmpw.get_start_address();
    Addr  hiw  = tmpw.get_end_address();

    if(lor!=low || hir!=hiw) {
        if(traceBuses()) {
            opPrintf(
                "TLM: %s DMI read [" FMT_Ax "-" FMT_Ax "] != write [" FMT_Ax "-" FMT_Ax "]\n",
                "unknown", lor, hir, low, hiw
            );
        }
        // invalidate the largest range. Not sure what else we could do.
        if(low < lor) lor = low;
        if(hiw < hir) hir = hiw;
        rok = wok = false;
    }

    addrLo = lor;
    addrHi = hir;
    if(rok && wok) {
        rw = RW;
    } else if (rok) {
        rw = RD;
    } else if (wok) {
        rw = WR;
    } else {
        return 0;
    }

    return tmpr.get_dmi_ptr();
}

void tlmDMIslave::killDMI(long long unsigned int lo, long long unsigned int hi) {
    invalidateDMI(lo,hi);
}

bool tlmDMIslave::read(
    object       *initiator,
    Addr          addr,
    Uns32         bytes,
    void*         data,
    void*         userData,
    Addr          VA,
    Bool          isFetch,
    bool          &dmiAllowed
) {
    Bool ok = false;

    tlm_generic_payload trans;
    tlmInitiatorExtension extension(initiator); // an extension to carry the processor pointer

    trans.set_extension(&extension);
    trans.set_read();
    trans.set_address        (addr);
    trans.set_data_length    (bytes);
    trans.set_data_ptr       ((unsigned char*)data);
    trans.set_streaming_width(bytes);
    trans.set_response_status(TLM_INCOMPLETE_RESPONSE);
    trans.set_dmi_allowed    (false);

    if(traceBuses()) {
        opPrintf("TLM: Read:0x%x bytes:%d %s\n", (Uns32) addr, bytes, initiator ? "trans":"dbg");
    }

    if (initiator) {
        sock->b_transport( trans, delay );
        if (trans.get_response_status() == TLM_OK_RESPONSE) {
            dmiAllowed = trans.is_dmi_allowed();
            ok = true;
        } else {
            initiator->readAbort(addr);
        }
    } else {
        Uns32 actual = sock->transport_dbg(trans);
        if(actual == bytes) {
            dmiAllowed = trans.is_dmi_allowed();
            ok = true;
        } else {
            //readAbort(addr);
        }
    }

    if(traceBuses()) {
        opPrintf("TLM: Read:0x%x  bytes:%d %s Complete %s", (Uns32) addr, bytes, initiator ? "trans":"dbg", ok ? "ok" : "fault");
        tlmDiagnostics::printBytes((Uns8*)data, bytes);
        opPrintf("\n");
    }

    trans.clear_extension(&extension);
    return ok;
}

bool tlmDMIslave::write(
    object       *initiator,
    Addr          addr,
    Uns32         bytes,
    const void*   data,
    void*         userData,
    Addr          VA,
    bool          &dmiAllowed
) {
    Bool ok = false;

    tlm_generic_payload trans;
    tlmInitiatorExtension extension(initiator); // an extension to carry the processor pointer

    trans.set_extension(&extension);

    trans.set_write();
    trans.set_address        (addr);
    trans.set_data_length    (bytes);
    trans.set_data_ptr       ((unsigned char*)data);
    trans.set_streaming_width(bytes);
    trans.set_response_status(TLM_INCOMPLETE_RESPONSE);
    trans.set_dmi_allowed    (false);

    if(traceBuses()) {
        opPrintf("TLM: Write:0x%x  bytes:%d %s ", (Uns32) addr, bytes, initiator ? "trans":"dbg");
        tlmDiagnostics::printBytes((Uns8*)data, bytes);
        opPrintf("\n");
    }

    if (initiator) {
        sock->b_transport( trans, delay );
        if (trans.get_response_status() == TLM_OK_RESPONSE) {
            dmiAllowed = trans.is_dmi_allowed();
            ok = true;
        } else {
            initiator->writeAbort(addr);
        }
    } else {
        Uns32 actual = sock->transport_dbg(trans);
        if(actual == bytes) {
            dmiAllowed = trans.is_dmi_allowed();
            ok = true;
        } else {
            //there is no initiator
        }
    }

    if(traceBuses()) {
        opPrintf("TLM: Write:0x%x  bytes:%d %s Complete %s\n", (Uns32) addr, bytes, initiator ? "trans":"dbg", ok ? "ok" : "fault");
    }

    trans.clear_extension(&extension);
    return ok;
}

bool tlmDMIslave::traceBuses(void) {
    if (processorParent) {
        return processorParent->traceBuses();
    }
    if (peripheralParent) {
        return peripheralParent->traceBuses();
    }
    return False;
}


} // end namespace cw

