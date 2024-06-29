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
#include "tlm/tlmProcessor.hpp"
#include "tlm/tlmPeripheral.hpp"
#include "tlm/tlmMmc.hpp"

#include "op/op.hpp"

namespace cw {

tlmBusMasterPort::tlmBusMasterPort(tlmModule &parent, tlmProcessor *p, std::string name, Uns32 addrBits)

    : busStub(parent, p->hierName() + name, addrBits)
    , slave(busStub, name, p, socket)
    , socket(/*creates its own name*/)
{
    // Connect processor to bus
    busStub.connect(*p, name);
    socket.register_invalidate_direct_mem_ptr(&slave, &cw::tlmDMIslave::killDMI);
}

tlmBusMasterPort::tlmBusMasterPort(tlmModule &parent, tlmPeripheral *p, std::string name, Uns32 addrBits)

    : busStub(parent, p->hierName() + "_" + name, addrBits)
    , slave(busStub, name, p, socket)
    , socket(/*creates its own name*/)
{
    // Connect peripheral to bus
    busStub.connectMaster(*p, name);
}

tlmBusMasterPort::tlmBusMasterPort(tlmModule &parent, tlmMMC *p, std::string name, Uns32 addrBits)

    : busStub(parent, p->hierName() + "_" + name, addrBits)
    , slave(busStub, name, p, socket)
    , socket(/*creates its own name*/)
{
    // Connect MMC to bus
    busStub.connect(*p, name);
}

tlmBusStub::tlmBusStub(tlmModule &parent, std::string name, Uns32 addrBits)
    : bus(parent, name, addrBits) {
}

} // end namespace cw
