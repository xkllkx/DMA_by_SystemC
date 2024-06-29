/*
 * Copyright (c) 2005-2021 Imperas Software Ltd., www.imperas.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "tlm/tlmMmc.hpp"                                // TLM headers
#include "tlm/tlmDecoder.hpp"
#include "tlm/tlmBusPort.hpp"

namespace cw {

tlmMMC::tlmMMC(tlmModule &parent, string path, sc_module_name name)
    : sc_module (name)
    , MMC(parent, path, sc_object::name())
    , traceBusesOn (false)
    , parentModule (parent)
{
}

//
tlmMMC::tlmMMC(tlmModule &parent, string path, sc_module_name name, params p)
    : sc_module (name)
    , MMC(parent, sc_object::name(), path, p)
    , traceBusesOn (false)
    , parentModule (parent)
{
}

void tlmMMC::connect(tlmDecoder &target, string portName) {

    tlmBusSlavePort  *s = new tlmBusSlavePort(parentModule, this, hierCName() + portName);
    target.connect(*s);
}

void tlmMMC::connect(tlmDecoder &initiator, string portName, Uns32 addrBits) {

    tlmBusMasterPort  *s = new tlmBusMasterPort(parentModule, this, hierCName() + portName, addrBits);
    initiator.connect(*s);
}

} // end namespace cw
