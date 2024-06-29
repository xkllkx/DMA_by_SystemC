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

#include "tlm/tlmPeripheral.hpp"
#include "tlm/tlmNetPort.hpp"
#include "tlm/tlmBusPort.hpp"
#include "tlm/tlmDecoder.hpp"

namespace cw {

tlmPeripheral::tlmPeripheral(tlmModule &parent, string path, sc_module_name name)
    : sc_module        (name)
    , peripheral       (parent, path, sc_object::name())
    , traceBusesOn     (false)
    , traceBusErrorsOn (false)
    , traceSignalsOn   (false)
    , parentModule     (parent)
{
    setTracing();
}


tlmPeripheral::tlmPeripheral(tlmModule &parent, string path, sc_module_name name, params p)
    : sc_module        (name)
    , peripheral       (parent, path, sc_object::name(), p)
    , traceBusesOn     (false)
    , traceBusErrorsOn (false)
    , traceSignalsOn   (false)
    , parentModule     (parent)
{
    setTracing();
}

void tlmPeripheral::connect(string localPort, tlmPeripheral &p, string remotePort) {

    tlmNetOutputPort *o = new tlmNetOutputPort(parentModule, this, localPort);
    tlmNetInputPort  *i = new tlmNetInputPort (parentModule, &p,  remotePort);
    o->bind(*i);
}

void tlmPeripheral::connect(string localPort, tlmProcessor &p, string remotePort) {

    tlmNetOutputPort *o = new tlmNetOutputPort(parentModule, this, localPort);
    tlmNetInputPort  *i = new tlmNetInputPort (parentModule, &p,  remotePort);
    o->bind(*i);
}

//
// with fixed decode
//
void tlmPeripheral::connect(tlmDecoder &target, string portName, Addr lo, Addr hi) {

    tlmBusSlavePort  *s = new tlmBusSlavePort(parentModule, this, portName, hi-lo+1);
    target.connect(*s, lo, hi);
}

//
// decode set at run time
//
void tlmPeripheral::connect(tlmDecoder &target, string portName) {

    tlmBusDynamicSlavePort  *s = new tlmBusDynamicSlavePort(parentModule, this, portName, 0);
    target.connect(*s);
}

void tlmPeripheral::connect(tlmDecoder &initiator, string portName, Uns32 addrBits) {

    tlmBusMasterPort  *s = new tlmBusMasterPort(parentModule, this, portName, addrBits);
    initiator.connect(*s);
}

} // end namespace
