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

#include "tlm/tlmDecoder.hpp"
#include "tlm/tlmBusPort.hpp"

namespace cw {

tlmDecoder::tlmDecoder(tlmModule &parent, sc_module_name name, Uns32 targets,  Uns32 initiators)
    : sc_module(name)
    , NR_OF_INITIATORS(initiators)
    , NR_OF_TARGETS(targets)
    , traceOn(false)
    , initiatorPortCount(0)
    , targetPortCount(0)
    , parentModule(parent)
{
    target_socket    = new targetSocketType[NR_OF_TARGETS];
    initiator_socket = new initiatorSocketType[NR_OF_INITIATORS];
    decodes          = new portMapping*[NR_OF_INITIATORS];

    for (Uns32 i = 0; i < NR_OF_TARGETS; ++i) {
        target_socket[i].register_b_transport       (this, &tlmDecoder::initiatorBTransport, i);
        target_socket[i].register_transport_dbg     (this, &tlmDecoder::transportDebug,      i);
        target_socket[i].register_get_direct_mem_ptr(this, &tlmDecoder::getDMIPointer,       i);
    }
    for (Uns32 i = 0; i < NR_OF_INITIATORS; ++i) {
        initiator_socket[i].register_invalidate_direct_mem_ptr(this, &tlmDecoder::invalidateDMIPointers, i);
        decodes[i] = 0;
    }
    if (getenv("IMPERAS_TLM_DECODE_TRACE")) {
        traceOn = true;
    }
}

void tlmDecoder::setDecode(Uns32 portId, Addr lo, Addr hi, Addr offset) {
    if (portId >= NR_OF_INITIATORS) {
        opPrintf(
            "Error configuring tlmDecoder %s: portId (%d) cannot be greater than the number of targets (%d)\n",
            name(),
            portId,
            NR_OF_INITIATORS
        );
        return;
    }
    if (lo > hi) {
        opPrintf(
            "Error configuring tlmDecoder %s: lo (0x" FMT_Ax ") cannot be greater than hi (0x" FMT_Ax ")\n",
            name(),
            lo,
            hi
        );
        return;
    }
    portMapping *decode = new portMapping(lo, hi, offset);
    decode->setNext(decodes[portId]);
    decodes[portId] = decode;
}

// connect to Imperas bus slave
void tlmDecoder::connect(tlmBusSlavePort &slave, Addr lo, Addr hi) {
    Uns32 port = nextInitiatorPort();
    initiator_socket[port](slave.socket);
    setDecode(port, lo, hi);
}

// connect to Imperas bus slave without addresses
void tlmDecoder::connect(tlmBusSlavePort &slave) {
    Uns32 port = nextInitiatorPort();
    initiator_socket[port](slave.socket);
    setDecode(port, 0, 0xffffffffffffffffLL);
}

// connect to Imperas bus slave
void tlmDecoder::connect(tlmBusDynamicSlavePort &slave) {
    Uns32 port = nextInitiatorPort();
    initiator_socket[port](slave.socket);
}

// connect to tlm RAM
void tlmDecoder::connect(ramSocketType &slave, Addr lo, Addr hi) {
    Uns32 port = nextInitiatorPort();
    initiator_socket[port](slave);
    setDecode(port, lo, hi);
}

// No address specified; plug-and-play
void tlmDecoder::connect(ramSocketType &slave) {
    Uns32 port = nextInitiatorPort();
    initiator_socket[port](slave);
}

// connect to another decoder
void tlmDecoder::connect(tlmDecoder &targetDecoder, Addr lo, Addr hi) {
    // next local port
    Uns32 port = nextInitiatorPort();

    // next remote target
    targetSocketType *next = targetDecoder.nextTarget();

    initiator_socket[port](*next);
    setDecode(port, lo, hi);
}

// connect to another decoder, with an output offset
void tlmDecoder::connect(tlmDecoder &targetDecoder, Addr lo, Addr hi, Addr offset) {
    // next local port
    Uns32 port = nextInitiatorPort();

    // next remote target
    targetSocketType *next = targetDecoder.nextTarget();

    initiator_socket[port](*next);
    setDecode(port, lo, hi, offset);
}

// connect to another decoder
void tlmDecoder::connect(tlmDecoder &targetDecoder) {
    // next local port
    Uns32 port = nextInitiatorPort();

    // next remote target
    targetSocketType *next = targetDecoder.nextTarget();

    initiator_socket[port](*next);
}

// connect to Imperas bus master port
void tlmDecoder::connect(tlmBusMasterPort &master) {
    Uns32 port = nextTargetPort();
    master.socket(target_socket[port]);
}

void tlmDecoder::connect(tlmProcessor &processor, std::string portName, Uns32 addrBits){
    tlmBusMasterPort *master = new tlmBusMasterPort(parentModule, &processor, portName, addrBits);
    connect(*master);
}

void tlmDecoder::connect(tlmPeripheral &peripheral, std::string portName, Uns32 addrBits){
    tlmBusMasterPort *master = new tlmBusMasterPort(parentModule, &peripheral, portName, addrBits);
    connect(*master);
}

void tlmDecoder::connect(tlmMMC &mmc, std::string portName, Uns32 addrBits){
    tlmBusMasterPort *master = new tlmBusMasterPort(parentModule, &mmc, portName, addrBits);
    connect(*master);
}

void tlmDecoder::connect(tlmMMC &mmc, std::string portName){
    tlmBusSlavePort *slave = new tlmBusSlavePort(parentModule, &mmc, portName);
    connect(*slave);
}

tlmDecoder::initiatorSocketType *tlmDecoder::nextInitiatorSocket(Addr lo, Addr hi) {
    Uns32 port = nextInitiatorPort();

    setDecode(port, lo, hi);
    return &initiator_socket[port];
}

portMapping *tlmDecoder::getMapping(int port, Addr address) {
    portMapping *decode;
    for (decode = decodes[port]; decode; decode = decode->getNext()) {
        if (decode->inRegion(address)) {
            return decode;
        }
    }
    return 0;
}

int tlmDecoder::getPortId(const Addr address, Addr& offset)
{
    for(Uns32 i = 0; i < NR_OF_INITIATORS; i++) {
        portMapping *decode = getMapping(i, address);
        if (decode) {
            offset = decode->offsetInto(address);
            return (int)i;
        }
    }
    return -1;
}

//
// LT protocol
// - forward each call to the target/initiator
//
void tlmDecoder::initiatorBTransport(int SocketId,
                         transaction_type& trans,
                         sc_time& t) {
    Addr orig        = trans.get_address();
    Addr offset;
    int  portId      = getPortId(orig, offset);
    if(portId >= 0) {
        if(traceOn) {
            opPrintf("TLM: %s decode:0x" FMT_64x " -> initiator_socket[%d]\n", name(), orig, portId);
        }
        // It is a static port ?
        initiatorSocketType *decodeSocket     = &initiator_socket[portId];
        trans.set_address(offset);
        (*decodeSocket)->b_transport(trans, t);
    } else {
        // might be a dynamic port, so try each un-set port till one responds

        bool match = false;
        for(Uns32 i = 0; i < NR_OF_INITIATORS; i++) {
            if (!decodes[i]) {
                match = true;
                initiatorSocketType *decodeSocket = &initiator_socket[i];
                (*decodeSocket)->b_transport(trans, t);
                if (trans.get_response_status() == TLM_OK_RESPONSE) {
                    if(traceOn) {
                        opPrintf("TLM: %s dynamic:0x" FMT_64x " -> initiator_socket[%d]\n", name(), orig, i);
                    }
                    break;
                }
            }
        }
        if(!match && traceOn) {
            opPrintf("TLM: %s decode:0x" FMT_64x " no mapping\n", name(), orig);
        }
    }
}

unsigned int tlmDecoder::transportDebug(int SocketId, transaction_type& trans)
{
    Addr orig        = trans.get_address();
    Addr offset;
    int  portId      = getPortId(orig, offset);
    if(portId >= 0) {
        if(traceOn) {
            opPrintf("TLM: %s dbg decode:0x" FMT_64x " -> initiator_socket[%d]\n",
                    name(), orig, portId);
        }
        // It is a static port ?
        initiatorSocketType *decodeSocket = &initiator_socket[portId];
        trans.set_address(offset);
        return (*decodeSocket)->transport_dbg(trans);
    } else {
        for(Uns32 i = 0; i < NR_OF_TARGETS; i++) {
            if (!decodes[i]) {
                initiatorSocketType *decodeSocket = &initiator_socket[i];
                int r = (*decodeSocket)->transport_dbg(trans);
                if (r) {
                    if(traceOn) {
                        opPrintf("TLM: %s dbg dynamic:0x" FMT_64x " -> initiator_socket[%d]\n",
                                name(), orig, i);
                    }
                    return r;
                }
            }
        }
        return 0;
    }
}

//
// The range has come from the DMI device, but needs to be
// adjusted by what the decoder does to the addresses.
//
void tlmDecoder::adjustRange(int portId, Addr orig, Addr& low, Addr& high)
{
    if(traceOn) {
        opPrintf("TLM: %s adjustRange port %d input lo:" FMT_64x " hi:" FMT_64x "\n", name(), portId, low, high);
    }
    Addr inRange = high - low;
    portMapping *map = getMapping(portId, orig);
    Addr portLo, portHi, portOffset;

    map->getRegion(portLo, portHi, portOffset);

    if(DMI_DEBUG) {
        opPrintf("TLM: %s adjustRange region addr:" FMT_64x " (lo:" FMT_64x " hi:" FMT_64x ")\n", name(), orig, portLo, portHi);
    }

    // low is always correct
    low  = low + portLo - portOffset;

    // if high > decoder, it's the special case of the device not knowing it's size
    // or just being larger than the decoder
    Addr maxDecode = portHi - portLo;
    if(inRange > maxDecode) {
        high = low + maxDecode;
    } else {
        high = low + inRange;
    }

    if(traceOn) {
        opPrintf("TLM: %s adjustRange addr:" FMT_64x " changed: lo:" FMT_64x " hi:" FMT_64x "\n\n", name(), orig, low, high);
    }
    Addr outRange = high - low;

    assert(outRange <= inRange);
    assert(high >= low);
    assert(high - low <= maxDecode);
}

//
// Cannot use DMI through plug & play devices. This is probably OK for present.
//
bool tlmDecoder::getDMIPointer(int SocketId, transaction_type& trans, tlm_dmi&  dmi_data)
{
    Addr address = trans.get_address();
    Addr offset;
    int  portId = getPortId(address, offset);
    Bool result = false;

    if(portId >= 0) {
        initiatorSocketType* decodeSocket = &initiator_socket[portId];

        // send on the transaction with the new address, adjusted for the decoder offset
        trans.set_address(offset);
        result = (*decodeSocket)->get_direct_mem_ptr(trans, dmi_data);

        // put the address back how it was
        trans.set_address(address);

        // Should always succeed
        Addr start = dmi_data.get_start_address();
        Addr end   = dmi_data.get_end_address();

        if (result) {
            // Range must contain address
            assert(start <= offset);
            assert(end   >= offset);
        }

        adjustRange(portId, address, start, end);

        dmi_data.set_start_address(start);
        dmi_data.set_end_address(end);
    }

    return result;
}

void tlmDecoder::invalidateDMIPointers(int port_id,  sc_dt::uint64 start_range,  sc_dt::uint64 end_range)
{
    // For each decode on this port, adjust the range for the decode and pass the call on.
    portMapping *decode;
    for (decode = decodes[port_id]; decode; decode = decode->getNext()) {
        sc_dt::uint64 start = decode->offsetOutOf(start_range);
        sc_dt::uint64 end   = decode->offsetOutOf(end_range);
        for (Uns32 i = 0; i < NR_OF_TARGETS; ++i) {
            (target_socket[i])->invalidate_direct_mem_ptr(start, end);
        }
    }
}

} // end namespace cw
