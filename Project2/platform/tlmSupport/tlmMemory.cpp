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

#include "tlm.h"                                // TLM headers
#include "tlm_utils/simple_target_socket.h"
#include <iostream>
#include <string.h>

#include "op/op.hpp"
#include "tlm/tlmMemory.hpp"

using namespace std;
using namespace sc_core;
using namespace tlm;

namespace cw {

Uns32 memorySegment::segIndex = 0;

////////////////////////////////////////// RAM ////////////////////////////////////////////////////

tlmRam::tlmRam (
      tlmModule      &parent        // module parent (not used, but same as others)
    , sc_module_name module_name    // module name
    , Addr           high_address   // memory size (bytes)
    , Uns32          memory_width   // memory width (bytes)
    , bool           dmi
    , bool           onePage
    , bool           tracing
)
    : sc_module     (module_name)             /// init module name
    , trace         (tracing)
    , maxAddress    (high_address)
    , maxPage       (0)
    , segments      (0)
    , bytes         (0)
    , head          (0)
    , cache         (0)
    , addrMask      (0)
    , memoryWidth   (memory_width)            /// init memory width (bytes)
    , dmiOn         (dmi)
    , sp1           ("sp1")                   /// init socket name
{
    calculatePageSize(high_address, onePage);

    sp1.register_b_transport       (this, &tlmRam::custom_b_transport);
    sp1.register_transport_dbg     (this, &tlmRam::debug_transport);
    sp1.register_get_direct_mem_ptr(this, &tlmRam::get_direct_mem_ptr);

    if(getenv("IMPERAS_TLM_MEMORY_TRACE")) {
        trace = true;
    }

    if(trace) {
        opPrintf("TLM: %s maxAddress:0x" FMT_Ax " page:0x" FMT_64x " addrMask:0x" FMT_64x "\n",
                name(),
                maxAddress,
                maxPage,
                addrMask
            );
    }
}

void tlmRam::custom_b_transport( tlm_generic_payload &payload, sc_time &delay_time)
{
    payload.set_dmi_allowed(dmiOn);

    Addr            address  = payload.get_address();     // memory address
    tlm_command     command  = payload.get_command();     // memory command
    Uns8           *data     = payload.get_data_ptr();    // data pointer
    Uns32           bytes    = payload.get_data_length(); // data length

    tlm_response_status response_status = checkPayloadAddress(payload)
            ? TLM_OK_RESPONSE
            : TLM_ADDRESS_ERROR_RESPONSE;

    if (payload.get_byte_enable_ptr()) {
        payload.set_response_status(TLM_BYTE_ENABLE_ERROR_RESPONSE);
    }
    else if (payload.get_streaming_width() != payload.get_data_length()) {
        payload.set_response_status(TLM_BURST_ERROR_RESPONSE);
    }

    if(response_status == TLM_OK_RESPONSE) {

        switch (command)
        {
            default:                response_status = TLM_COMMAND_ERROR_RESPONSE; break;
            case TLM_WRITE_COMMAND: rwrite(address, bytes, data);                 break;
            case TLM_READ_COMMAND:  rread(address, bytes, data);                  break;
        }
    }
    payload.set_response_status(response_status);
}


Uns32 tlmRam::debug_transport (tlm_generic_payload &payload)
{
    sc_time  mem_op_time;
    payload.set_dmi_allowed(dmiOn);

    Addr           address  = payload.get_address();     // memory address
    tlm_command    command  = payload.get_command();     // memory command
    Uns8          *data     = payload.get_data_ptr();    // data pointer
    Uns32          bytes    = payload.get_data_length(); // data length
    bool           ok       = checkPayloadAddress(payload);

    if(ok) {
        switch (command)
        {
            default:                ok = false;                        break;
            case TLM_WRITE_COMMAND: ok = rwrite(address, bytes, data);  break;
            case TLM_READ_COMMAND:  ok = rread(address, bytes, data);   break;
        }
    }
    return ok ? bytes : 0;
}


bool tlmRam::get_direct_mem_ptr(tlm_generic_payload &trans, tlm_dmi &dmi_data)
{
    Addr  address = trans.get_address();
    Uns32 bytes   = trans.get_data_length();

    Addr addrLo, addrHi;
    Uns8 *ptr = getDMI(address, bytes, addrLo, addrHi);

    dmi_data.set_start_address(addrLo);
    dmi_data.set_end_address(addrHi);

    if(ptr) {
        // if (m_invalidate) m_invalidate_dmi_event.notify(m_invalidate_dmi_time);
        dmi_data.allow_read_write();

        trans.set_dmi_allowed(dmiOn);
        dmi_data.set_dmi_ptr(ptr);
        return true;
    } else {
        trans.set_dmi_allowed(0);
        dmi_data.set_dmi_ptr(0);
        return false;
    }
}

Uns8 *tlmRam::getDMI(
    Addr        addr,
    Uns32       bytes,
    Addr       &addrLo,
    Addr       &addrHi
) {
    if(checkAddressRange(addr, bytes)) {

        memorySegment *seg = findAddSeg(addr, bytes);

        addrLo    = seg->base();
        addrHi    = seg->top();

        Uns8 *ptr = seg->native();

        if(trace) {
            opPrintf("TLM: %s : DMI : 0x" FMT_64x " (%u bytes)  > %s [0x" FMT_64x ":0x" FMT_64x "]\n",
                name(),
                addr,
                bytes,
                ptr ? "Y" : "N",
                addrLo,
                addrHi
            );
        }
        return ptr;
    } else {
        return 0;
    }
}

bool tlmRam::rread(Addr addr, Uns32 bytes, void* data) {

    Uns8 *native = getInPage(addr, bytes);

    // If we get to here, there's been a read from uninitialized memory
    switch(bytes) {
        case 1:    *(Uns8* )data = *(Uns8 *)native;    break;
        case 2:    *(Uns16*)data = *(Uns16*)native;    break;
        case 4:    *(Uns32*)data = *(Uns32*)native;    break;
        default:   memcpy(data, native, bytes);        break;
    }
    return true;
}

bool tlmRam::rwrite(Addr addr, Uns32 bytes, void* data) {

    Uns8 *native = getInPage(addr, bytes);

    switch(bytes) {
       case 1:    *(Uns8* )native = *(Uns8* )data;    break;
       case 2:    *(Uns16*)native = *(Uns16*)data;    break;
       case 4:    *(Uns32*)native = *(Uns32*)data;    break;
       default:   memcpy(native, data, bytes);        break;
    }
    return true;
}

//
// Return the native address of the requested simulated address,
// allocating the space if required
//
Uns8 *tlmRam::getInPage(Addr addr, Addr bytes) {

    memorySegment *seg    = findAddSeg(addr, bytes);
    assert(addr >= seg->base() && addr <= seg->top());
    Addr           offset = addr - seg->base();
    return offset + seg->native();
}


//
// put newSeg immediately after n in the list
//
void tlmRam::linkSegAfter(memorySegment *newSeg, memorySegment *n) {
    assert(n);

    memorySegment *next = n->next;

    n->next = newSeg;
    newSeg->prev = n;

    if(next) {
        next->prev = newSeg;
        newSeg->next = next;
    }
}

//
// put newSeg first in the list
//
void tlmRam::linkSegFirst(memorySegment *newSeg) {
    memorySegment *next = head;
    head = newSeg;
    if(next) {
        newSeg->next = next;
        next->prev   = newSeg;
    }
}

//
// Insert in the right place in the existing list
// Assumes no overlap with the existing segments
//
void tlmRam::linkSeg(memorySegment *seg) {

    if(0) { printSegments("linkSeg:BEFORE"); }

    cache = seg;

    if(head) {
        Addr bot = seg->base();
        Addr top = seg->top();
        if(top < head->base()) {
            // first place
            linkSegFirst(seg);
        } else {
            // not first, so look for place to insert
            for (memorySegment *s = head; s; s = s->next) {
                if(bot > s->top()) {
                    memorySegment *n = s->next;
                    if(n) {
                        if(top < n->base()) {
                            linkSegAfter(seg, s);
                        } else {
                            continue;
                        }
                    } else {
                        linkSegAfter(seg, s);
                    }
                    break;
                }
            }
        }
    } else {
        linkSegFirst(seg);
    }
    if(0) { printSegments("linkSeg:AFTER"); }
}

//
// Find existing or create new contiguous segment
//
memorySegment *tlmRam::findAddSeg(Addr addr, Addr bytes) {

    Addr bot = addr;
    Addr top = addr + bytes - 1;

    if(trace) { opPrintf("FindAddSeg %s: ", name()); memorySegment::printRange(bot, top); opPrintf("\n"); }

    if(cache) {
        if (cache->fallsWithin(bot, top)) {
            if(trace) { opPrintf("    Existing (cache) "); cache->print(); opPrintf("\n"); }
            return cache;
        }
    }

    Uns32 i = 0;
    for (memorySegment *seg = head; seg; seg = seg->next) {
        i++;

        if (seg->fallsWithin(bot, top)) {
            // falls within an existing segment, so we are done
            if(trace) { opPrintf("    Existing  (searched %u)", i); seg->print(); opPrintf("\n"); }
            cache = seg;
            return seg;
        }
    }

    if(overlapsAny(bot, top)) {
        // This range overlaps with an existing segment, so need to merge segments

        // find the entire range of existing and new
        Addr botRange = bot;
        Addr topRange = top;

        //if(trace) { opPrintf("  coverRange ip "); memorySegment::printRange(botRange, topRange); opPrintf("\n"); }
        coverRange(botRange, topRange);
        //if(trace) { opPrintf("  coverRange op "); memorySegment::printRange(botRange, topRange); opPrintf("\n"); }

        // allocate a new segment
        memorySegment *newSeg = new memorySegment(botRange, topRange);

        if(trace) { opPrintf("    Realloc   ");  newSeg->print(); opPrintf("\n");  }
        // copy contents of overlapping segments
        for (memorySegment *seg = head; seg; seg = seg->next) {

            if (seg->hasOverlap(botRange, topRange)) {

                Addr offset = seg->base() - botRange;
                Uns8 *src   = seg->native();
                Uns8 *dst   = newSeg->native() + offset;
                Addr bytes = seg->bytes();

                memcpy(dst, src, bytes);
                if(trace) { opPrintf("  Copy in   "); seg->print(); opPrintf(" dst:%p src:%p (" FMT_64x " bytes)\n", dst, src, bytes); }
            }
        }

        // Invalidate the DMI region
        if(trace) { opPrintf("    Invalidate "); memorySegment::printRange(botRange, topRange); opPrintf("\n"); }

        sp1->invalidate_direct_mem_ptr(botRange, topRange);

        // Free the old segments
        memorySegment *nxt;

        for (memorySegment *seg = head; seg; seg = nxt) {

            nxt = seg->next;
            if (seg->hasOverlap(botRange, topRange)) {
                if(trace) { opPrintf("    Delete    "); seg->print(); opPrintf("\n"); }
                unlinkSeg(seg);
            }
        }

        // insert the new segment in the correct place
        linkSeg(newSeg);
        return newSeg;

    } else {
        // no overlap with existing so make a new one
        roundToPageBoundaries(bot, top);
        memorySegment *newSeg = new memorySegment(bot, top);
        if(trace) { opPrintf("    New       ");  newSeg->print(); opPrintf("\n"); }
        linkSeg(newSeg);
        return newSeg;
    }
}

} // end namespace cw
