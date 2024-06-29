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

#include "tlm/tlmNetPort.hpp"
#include "tlm/tlmProcessor.hpp"
#include "tlm/tlmPeripheral.hpp"

namespace cw {

tlmNetInputPort::tlmNetInputPort(tlmModule &parent, tlmProcessor *p, std::string name)
    : proc(p)
    , periph(0)
    , n1(parent, p->hierName() + name)
{
     n1.connect(*p, name);
}

tlmNetInputPort::tlmNetInputPort(tlmModule &parent, tlmPeripheral *p, std::string name)
    : proc(0)
    , periph(p)
    , n1(parent, p->hierName() + name)
{
    n1.connect(*p, name);
}

void tlmNetInputPort::write(const Uns32 &value) {
    if (0 /*periph->traceSignals()*/) {
        opPrintf("TLM: %s = %d\n", n1.hierCName(), value);
    }
    n1.write(value);
}

tlmNetOutputPort::tlmNetOutputPort(tlmModule &parent, tlmPeripheral *p, std::string name)
: netWithCallback(parent, p->hierName() + name)
, periph(p)
, proc(0)
{
    connect(*p, name);
}

tlmNetOutputPort::tlmNetOutputPort(tlmModule &parent, tlmProcessor *p, std::string name)
: netWithCallback(parent, p->hierName() + name)
, periph(0)
, proc(p)
{
    connect(*p, name);
}

tlmNetInoutPort::tlmNetInoutPort(tlmModule &parent, tlmPeripheral *p, std::string name)
    : netWithCallback(parent, p->hierName() + name)
    , periph(p)
{
    connect(*p, name);
}

} // end namespace cw
