/*
 *
 * Copyright (c) 2005-2021 Imperas Software Ltd., www.imperas.com
 *
 * The contents of this file are provided under the Software License Agreement
 * that you accepted before downloading this file.
 *
 * This header forms part of the Software but may be included and used unaltered
 * in derivative works.
 *
 * For more information, please visit www.OVPworld.org or www.imperas.com
 */

#include "op/op.hpp"

#include <iostream>


namespace op {

using namespace std;

// Create a class wrapper only if required

#define GET_METHOD(_CLASS, _TYPE)                  \
_CLASS *_CLASS::classGet(_TYPE existing) {         \
    _CLASS *r = 0;                                 \
    if(existing) {                                 \
        r = (_CLASS*)opObjectClass(existing);      \
        if(!r) {                                   \
            r = new _CLASS(existing);              \
            r->allocated = true;                   \
            r->classSet();                         \
        }                                          \
    }                                              \
    return r;                                      \
}

#define EXISTING_CONSTRUCTOR(_CLASS, _TYPE)        \
_CLASS::_CLASS(_TYPE existing) {                   \
    h = existing;                                  \
    classSet();                                    \
}

#define CPP_SUPRESS_UNUSED(_argName)  (void)(_argName)

#define BY_NAME_METHOD(_CLASS, _CONTAINER, _TYPE, _ENUM) \
_CLASS *_CONTAINER::_CLASS ## ByName (std::string name) { \
    _CLASS *r = 0; \
    optObjectP  op = opVoidByName(h, name.c_str(), _ENUM);\
    if(op) { \
        r = (_CLASS*)opObjectClass(op); \
        if(!r) { \
            r = new _CLASS((_TYPE)op); \
            r->allocated = true; \
            r->classSet(); \
        } \
    } \
    return r; \
}

#define BY_NAME_PTR_METHOD(_CLASS, _CONTAINER, _TYPE, _ENUM) \
_CLASS *_CONTAINER::_CLASS ## ByName (std::string name) { \
    return _CLASS::classGet((_TYPE)opVoidByName(h, name.c_str(), _ENUM)); \
}

//
// Iterate over objects contained in a module
//
#define NEXT_METHOD(_CLASS,_TYPE,_ITERATOR) \
_CLASS *module::_CLASS ## Next(_CLASS *prev) { \
    _CLASS *r = 0; \
    _TYPE op = _ITERATOR (handle(), prev ? prev->handle() : 0); \
    if(op) { \
        r = (_CLASS*)opObjectClass(op); \
        if(!r) { \
            r = new _CLASS((_TYPE)op); \
            r->allocated = true; \
            r->classSet(); \
        } \
    } \
    return r; \
}

//
// For classes that are cast from the OP object
//
#define NEXT_PTR_METHOD(_CLASS, _PARENT, _OP_FUNC) \
_CLASS *_PARENT::_CLASS ## Next(_CLASS *prev ) { \
    return _CLASS::classGet(_OP_FUNC(handle(), prev->handle())); \
}

#define DEST(_CLASS,_TYPE)                      \
    (void) userData;                            \
    _CLASS *n = (_CLASS*)opObjectClass(object); \
    if(n && n->allocated) {                     \
        delete n;                               \
    }

///////////////////////////////////////////  OBJECT ///////////////////////////////////////////////

void object::readAbort(Addr addr) {
    switch (opObjectType(h)) {
        case OP_PROCESSOR_EN:
            opProcessorReadAbort((optProcessorP)h, addr);
            break;

        case  OP_PERIPHERAL_EN:
            // TODO
            break;

        default:
            // No action
            break;
    }
}

void object::writeAbort(Addr addr) {
    switch (opObjectType(h)) {
        case OP_PROCESSOR_EN:
            opProcessorWriteAbort((optProcessorP)h, addr);
            break;

        case  OP_PERIPHERAL_EN: {
            // TODO
            break;

        default:
            // No action
            break;
        }
    }
}

/////////////////////////////////////////// BUS ///////////////////////////////////////////////

bus::bus(module &parent, std::string name, Uns32 bits, params p) {
    h = opBusNew(parent.handle(), name.c_str(), bits, 0, p.handle());
    classSet();
}

bus::bus(module &parent, std::string name, Uns32 bits) {
    h = opBusNew(parent.handle(), name.c_str(), bits, 0, 0);
    classSet();
}

GET_METHOD(bus, optBusP)

EXISTING_CONSTRUCTOR(bus, optBusP)

busSlave *bus::busSlaveNext (busSlave *o) {
    optBusSlaveP bs = opBusSlaveNext(handle(), o->handle());
    return busSlave::classGet(bs);
}

///////////////////////////////////////// BUS PORTCONN ////////////////////////////////////////////

busPortConn *bus::connect(processor &p, std::string portName) {
    return busPortConn::classGet(opProcessorBusConnect(p.handle(), handle(), portName.c_str()));
}

busPortConn *bus::connect(memory &p, std::string portName, Addr lo, Addr hi) {
    return busPortConn::classGet(opMemoryBusConnect(p.handle(), handle(), portName.c_str(), lo, hi));
}

busPortConn *bus::connectSlave(peripheral &p, std::string portName, Addr lo, Addr hi) {
    return busPortConn::classGet(opPeripheralBusConnectSlave(p.handle(), handle(), portName.c_str(), lo, hi));
}

busPortConn *bus::connectSlave(peripheral &p, std::string portName) {
    return busPortConn::classGet(opPeripheralBusConnectSlaveDynamic(p.handle(), handle(), portName.c_str()));
}

busPortConn *bus::connectMaster(peripheral &p, std::string portName) {
    return busPortConn::classGet(opPeripheralBusConnectMaster(p.handle(), handle(), portName.c_str(), 0, 0));
}

busPortConn *bus::connectSlave(bridge &p, std::string portName, Addr lo, Addr hi) {
    return busPortConn::classGet(opBridgeBusConnect(p.handle(), handle(), portName.c_str(), 0, lo, hi));
}

busPortConn *bus::connectMaster(bridge &p, std::string portName) {
    return busPortConn::classGet(opBridgeBusConnect(p.handle(), handle(), portName.c_str(), 1, 0, 0));
}

busPortConn *bus::connectMaster(bridge &p, std::string portName, Addr lo, Addr hi) {
    return busPortConn::classGet(opBridgeBusConnect(p.handle(), handle(), portName.c_str(), 1, lo, hi));
}

busPortConn *bus::connect(MMC &p, std::string portName) {
    return busPortConn::classGet(opMMCBusConnect(p.handle(), handle(), portName.c_str()));
}

busPortConn *bus::connect (peripheral &p, std::string portName) {
    return connectMaster(p, portName);
}

busPortConn *bus::connect (peripheral &p, std::string portName, Addr lo, Addr hi) {
    return connectSlave(p, portName, lo, hi);
}

watchpoint *bus::accessWatchpointNew(Addr lo, Addr hi, void *userData, optAddrWatchpointConditionFn notifierCB) {
    return watchpoint::classGet(opBusAccessWatchpointNew(handle(), lo, hi, userData, notifierCB));
}

watchpoint *bus::readWatchpointNew(Addr lo, Addr hi, void *userData, optAddrWatchpointConditionFn notifierCB) {
    return watchpoint::classGet(opBusReadWatchpointNew(handle(), lo, hi, userData, notifierCB));
}

watchpoint *bus::writeWatchpointNew(Addr lo, Addr hi, void *userData, optAddrWatchpointConditionFn notifierCB) {
    return watchpoint::classGet(opBusWriteWatchpointNew(handle(), lo, hi, userData, notifierCB));
}

/////////////////////////////////////////// BUS PORT //////////////////////////////////////////////

MMRegister *busPort::MMRegisterNext(MMRegister *prev) {
    return MMRegister::classGet(opBusPortMMRegisterNext(handle(), prev->handle()));
}

//////////////////////////////////////////// MODULE ///////////////////////////////////////////////

// regular module instance
module::module(module &parent, std::string path, std::string name) {
    h = opModuleNew(parent.handle(), path.c_str(), name.c_str(), 0, 0);
    classSet();
}

// regular module instance with params
module::module(module &parent, std::string path, std::string name, params p) {
    h = opModuleNew(parent.handle(), path.c_str(), name.c_str(), 0, p.handle());
    classSet();
}

module::module(void) {
    h = opRootModuleNew(0, "", 0);
    classSet();
}

module::module(std::string name) {
    h = opRootModuleNew(0, name.c_str(), 0);
    classSet();
}

// Root module with params
module::module(std::string name, params p) {
    h = opRootModuleNew(0, name.c_str(), p.handle());
    classSet();
}

// Root module with params
module::module(const char *name, params p) {
    h = opRootModuleNew(0, name, p.handle());
    classSet();
}

EXISTING_CONSTRUCTOR(module, optModuleP)

// Root module simulate returns the current processor
processor* module::simulate(void){
    return processor::classGet(opRootModuleSimulate(handle()));
}

//
// find the OP object by name
// (use the opVoid function because we cannot return a union in C++)
// If there is already a C++ object, return that, else make one.
//

BY_NAME_METHOD(bridge,     module, optBridgeP,     OP_BRIDGE_EN)
BY_NAME_METHOD(memory,     module, optMemoryP,     OP_MEMORY_EN)
BY_NAME_METHOD(MMC,        module, optMMCP,        OP_MMC_EN)
BY_NAME_METHOD(module,     module, optModuleP,     OP_MODULE_EN)
BY_NAME_METHOD(peripheral, module, optPeripheralP, OP_PERIPHERAL_EN)
BY_NAME_METHOD(processor,  module, optProcessorP,  OP_PROCESSOR_EN)

BY_NAME_METHOD(bus,        module, optBusP,        OP_BUS_EN)
BY_NAME_METHOD(FIFO,       module, optFIFOP,       OP_FIFO_EN)
BY_NAME_METHOD(net,        module, optNetP,        OP_NET_EN)
BY_NAME_METHOD(packetnet,  module, optPacketnetP,  OP_PACKETNET_EN)

BY_NAME_PTR_METHOD(busPort,    module, optBusPortP,    OP_BUSPORT_EN)
BY_NAME_PTR_METHOD(netPort,    module, optNetPortP,    OP_NETPORT_EN)

NEXT_METHOD(bridge    , optBridgeP    , opBridgeNext     )
NEXT_METHOD(memory    , optMemoryP    , opMemoryNext     )
NEXT_METHOD(MMC       , optMMCP       , opMMCNext        )
NEXT_METHOD(module    , optModuleP    , opModuleNext     )
NEXT_METHOD(peripheral, optPeripheralP, opPeripheralNext )
NEXT_METHOD(processor , optProcessorP , opProcessorNext  )

NEXT_METHOD(bus       , optBusP       , opBusNext        )
NEXT_METHOD(FIFO      , optFIFOP      , opFIFONext       )
NEXT_METHOD(net       , optNetP       , opNetNext        )
NEXT_METHOD(packetnet , optPacketnetP , opPacketnetNext  )

docNode *module::docNodeNext(docNode *prev) {
    return docNode::classGet(opObjectDocNodeNext(handle(), prev->handle()));
}

docNode *module::docSectionAdd(std::string name) {
    return docNode::classGet(opModuleDocSectionAdd(handle(), name.c_str()));
}

NEXT_PTR_METHOD(busPort,           module, opObjectBusPortNext);
NEXT_PTR_METHOD(FIFOPort,          module, opObjectFIFOPortNext);
NEXT_PTR_METHOD(netPort,           module, opObjectNetPortNext);
NEXT_PTR_METHOD(packetnetPort,     module, opObjectPacketnetPortNext);

NEXT_PTR_METHOD(busPortConn,       module, opObjectBusPortConnNext);
NEXT_PTR_METHOD(FIFOPortConn,      module, opObjectFIFOPortConnNext);
NEXT_PTR_METHOD(netPortConn,       module, opObjectNetPortConnNext);
NEXT_PTR_METHOD(packetnetPortConn, module, opObjectPacketnetPortConnNext);

watchpoint *module::watchpointNext() {
    return watchpoint::classGet(opRootModuleWatchpointNext(handle()));
}

command *module::commandByName(std::string name) {
    return command::classGet((optCommandP)opVoidByName(handle(), name.c_str(), OP_COMMAND_EN));
}

///////////////////////////////////////////// BRIDGE ////////////////////////////////////////////////

bridge::bridge(module &parent, std::string name, params p) {
    h = opBridgeNew(parent.handle(), name.c_str(), 0, p.handle());
    classSet();
}

bridge::bridge(module &parent, std::string name) {
    h = opBridgeNew(parent.handle(), name.c_str(), 0, 0);
    classSet();
}

GET_METHOD(bridge, optBridgeP)

EXISTING_CONSTRUCTOR(bridge, optBridgeP)

///////////////////////////////////////////// COMMANDS /////////////////////////////////////////////

commands::commands() {
}

void commands::all(processor &proc) {
    opProcessorCommandIterAll(proc.handle(), eachCommandStatic, this);
}

void commands::eachCommand(command *c) {
    CPP_SUPRESS_UNUSED(c);
}

typedef command commandType;

OP_COMMAND_FN(commands::eachCommandStatic) {
    CPP_SUPRESS_UNUSED(command);
    commands *classPtr = (commands*) userData;
    commandType *cmdPtr = (commandType*)command;
    classPtr->eachCommand(cmdPtr);
}

//////////////////////////////////////////// COMMAND ARGS /////////////////////////////////////////

commandArgs::commandArgs() {
}

void commandArgs::all(command &cmd) {
    opCommandArgIterAll(cmd.handle(), eachCommandArgStatic, this);
}

void commandArgs::all(command *cmd) {
    opCommandArgIterAll(cmd->handle(), eachCommandArgStatic, this);
}

void commandArgs::eachCommandArg(commandArg *c) {
    CPP_SUPRESS_UNUSED(c);
}

typedef commandArg commandArgType;

OP_COMMAND_ARG_FN(commandArgs::eachCommandArgStatic) {
    commandArgs *classPtr = (commandArgs*) userData;
    commandArgType *cmdArgPtr = (commandArgType*)arg;
    classPtr->eachCommandArg(cmdArgPtr);
}

//////////////////////////////////////////// EXTENSION ////////////////////////////////////////////

GET_METHOD(extension, optExtensionP)

EXISTING_CONSTRUCTOR(extension, optExtensionP)

extension::extension(module &parent, std::string path, std::string name, params p) {
    h = opExtensionNew(parent.handle(), path.c_str(), name.c_str(), p.handle());
}

extension::extension(module &parent, std::string path, std::string name) {
    h = opExtensionNew(parent.handle(), path.c_str(), name.c_str(), 0);
}

extension::extension(processor &parent, std::string path, std::string name, params p) {
    h = opProcessorExtensionNew(parent.handle(), path.c_str(), name.c_str(), p.handle());
}

extension::extension(processor &parent, std::string path, std::string name) {
    h = opProcessorExtensionNew(parent.handle(), path.c_str(), name.c_str(), 0);
}

extension::extension(peripheral &parent, std::string path, std::string name, params p) {
    h = opPeripheralExtensionNew(parent.handle(), path.c_str(), name.c_str(), p.handle());
}

extension::extension(peripheral &parent, std::string path, std::string name) {
    h = opPeripheralExtensionNew(parent.handle(), path.c_str(), name.c_str(), 0);
}

///////////////////////////////////////////// EXTELAB /////////////////////////////////////////////

GET_METHOD(extElab, optExtElabP)

EXISTING_CONSTRUCTOR(extElab, optExtElabP)

command *extElab::commandNext(command *prev) {
    return command::classGet(opObjectCommandNext(handle(), prev? prev->handle() : 0));
}

formal *extElab::formalNext(formal *prev) {
    return formal::classGet(opObjectFormalNext(handle(), prev? prev->handle() : 0, OP_PARAM_ALL));
}

////////////////////////////////////////////// FORMAL /////////////////////////////////////////////

GET_METHOD(formal, optFormalP)

EXISTING_CONSTRUCTOR(formal, optFormalP)

/////////////////////////////////////////////// PARSER ////////////////////////////////////////////

parser::parser(int argc, const char *argv[]) {
    h = opCmdParserNew(argv[0], OP_AC_ALL);
    opCmdErrorHandler(h, errorCallback, this);
    opCmdParseArgs(h, argc, argv);
}

parser::parser(std::string name, optArgClass useMask) {
     h = opCmdParserNew(name.c_str(), useMask);
     opCmdErrorHandler(h, errorCallback, this);
}

OP_CMD_ERROR_HANDLER_FN(parser::errorCallback) {
    parser *classPtr = (parser*) userData;
    return classPtr->error(flag);
}

// default error handler - overload if required
Bool parser::error(const char *flag) {
    CPP_SUPRESS_UNUSED(flag);
    // return to say this function did not handle the error event
    return False;
}

///////////////////////////////////////////// PROCESSOR ///////////////////////////////////////////

processor::processor(module &parent, std::string path, std::string name) {
    h = opProcessorNew(parent.handle(), path.c_str(), name.c_str(), 0, 0);
    classSet();
}

processor::processor(module &parent, std::string path, std::string name, params p) {
    h = opProcessorNew(parent.handle(), path.c_str(), name.c_str(), 0, p.handle());
    classSet();
}

BY_NAME_PTR_METHOD(busPort,    processor, optBusPortP,    OP_BUSPORT_EN)
BY_NAME_PTR_METHOD(netPort,    processor, optNetPortP,    OP_NETPORT_EN)

NEXT_PTR_METHOD(busPort,           processor, opObjectBusPortNext);
NEXT_PTR_METHOD(FIFOPort,          processor, opObjectFIFOPortNext);
NEXT_PTR_METHOD(netPort,           processor, opObjectNetPortNext);

NEXT_PTR_METHOD(busPortConn,       processor, opObjectBusPortConnNext);
NEXT_PTR_METHOD(FIFOPortConn,      processor, opObjectFIFOPortConnNext);
NEXT_PTR_METHOD(netPortConn,       processor, opObjectNetPortConnNext);

GET_METHOD(processor, optProcessorP)

EXISTING_CONSTRUCTOR(processor, optProcessorP)

docNode *processor::docNodeNext(docNode *prev) {
    return docNode::classGet(opObjectDocNodeNext(handle(), prev->handle()));
}

exception *processor::exceptionNext(exception *prev) {
    return exception::classGet(opProcessorExceptionNext(handle(), prev->handle()));
}

exception *processor::exceptionCurrent(void) {
    return exception::classGet(opProcessorExceptionCurrent(handle()));
}

mode *processor::modeNext (mode *prev){
    return mode::classGet(opProcessorModeNext(handle(), prev->handle()));
}

mode *processor::modeCurrent(void) {
    return mode::classGet(opProcessorModeCurrent(handle()));
}

command *processor::commandNext(command *prev) {
    return command::classGet(opObjectCommandNext(handle(), prev->handle()));
}

vlnv *processor::DefaultSemihost(void) {
    return vlnv::classGet(opProcessorDefaultSemihost(handle()));
}

vlnv *processor::Helper(void) {
    return vlnv::classGet(opProcessorHelper(handle()));
}

vlnv *processor::DebugHelper(void) {
    return vlnv::classGet(opProcessorDebugHelper(handle()));
}

watchpoint *processor::accessWatchpointNew(Addr lo, Addr hi, bool ph, void *userData, optAddrWatchpointConditionFn notifierCB) {
    return watchpoint::classGet(opProcessorAccessWatchpointNew(handle(), lo, hi, ph, userData, notifierCB));
}

watchpoint *processor::readWatchpointNew(Addr lo, Addr hi, bool ph, void *userData, optAddrWatchpointConditionFn notifierCB) {
    return watchpoint::classGet(opProcessorReadWatchpointNew(handle(), lo, hi, ph, userData, notifierCB));
}

watchpoint *processor::writeWatchpointNew(Addr lo, Addr hi, bool ph, void *userData, optAddrWatchpointConditionFn notifierCB) {
    return watchpoint::classGet(opProcessorWriteWatchpointNew(handle(), lo, hi, ph, userData, notifierCB));
}

watchpoint *processor::modeWatchpointNew(void *userData, optProcWatchpointConditionFn notifierCB) {
    return watchpoint::classGet(opProcessorModeWatchpointNew(handle(),userData, notifierCB));
}

watchpoint *processor::exceptionWatchpointNew(void *userData, optProcWatchpointConditionFn notifierCB) {
    return watchpoint::classGet(opProcessorExceptionWatchpointNew(handle(),userData, notifierCB));
}

/////////////////////////////////////////// PERIPHERAL ///////////////////////////////////////////////


peripheral::peripheral(module &parent, std::string path, std::string name, params p) {
    h = opPeripheralNew(parent.handle(), path.c_str(), name.c_str(), 0, p.handle());
    classSet();
}

peripheral::peripheral(module &parent, std::string path, std::string name) {
    h = opPeripheralNew(parent.handle(), path.c_str(), name.c_str(), 0, 0);
    classSet();
}

bool peripheral::addPortMapCB(std::string portName, optPortMapFn cb, void *userData) {

    optBusPortConnP bpc = (optBusPortConnP)opVoidByName(handle(), portName.c_str(), OP_BUSPORTCONN_EN);
    if(bpc) {
        return opBusPortConnMapNotify(bpc, cb, userData);
    } else {
        return False;
    }
}

NEXT_PTR_METHOD(busPort,           peripheral, opObjectBusPortNext);
NEXT_PTR_METHOD(netPort,           peripheral, opObjectNetPortNext);
NEXT_PTR_METHOD(packetnetPort,     peripheral, opObjectPacketnetPortNext);

NEXT_PTR_METHOD(busPortConn,       peripheral, opObjectBusPortConnNext);
NEXT_PTR_METHOD(netPortConn,       peripheral, opObjectNetPortConnNext);
NEXT_PTR_METHOD(packetnetPortConn, peripheral, opObjectPacketnetPortConnNext);

BY_NAME_PTR_METHOD(busPort,    peripheral, optBusPortP,    OP_BUSPORT_EN)
BY_NAME_PTR_METHOD(netPort,    peripheral, optNetPortP,    OP_NETPORT_EN)

GET_METHOD(peripheral, optPeripheralP)

EXISTING_CONSTRUCTOR(peripheral, optPeripheralP)

docNode *peripheral::docNodeNext(docNode *prev) {
    return docNode::classGet(opObjectDocNodeNext(handle(), prev->handle()));
}

/////////////////////////////////////////// MEMORY ///////////////////////////////////////////////

memory::memory(module &parent, std::string name, Addr maxAddress, optPriv priv) {
    h = opMemoryNew(parent.handle(), name.c_str(), priv, maxAddress, 0, 0);
    classSet();
}

memory::memory(module &parent, std::string name, Addr maxAddress, optPriv priv, params p) {
    h = opMemoryNew(parent.handle(), name.c_str(), priv, maxAddress, 0, p.handle());
    classSet();
}

memory::memory(module &parent, std::string name, Addr maxAddress, void *native, optPriv priv) {
    h = opMemoryNativeNew(parent.handle(), name.c_str(), priv, maxAddress, native, 0, 0);
    classSet();
}

memory::memory(module &parent, std::string name, Addr maxAddress, void *native, optPriv priv, params p) {
    h = opMemoryNativeNew(parent.handle(), name.c_str(), priv, maxAddress, native, 0, p.handle());
    classSet();
}

busPortConn *memory::busPortConnNext(busPortConn *prev) {
    return busPortConn::classGet(opObjectBusPortConnNext(handle(), prev->handle()));
}

busPort *memory::busPortNext(busPort  *prev) {
    return busPort::classGet(opObjectBusPortNext(handle(), prev->handle()));
}


GET_METHOD(memory, optMemoryP)

EXISTING_CONSTRUCTOR(memory, optMemoryP)

/////////////////////////////////////////// MMC ///////////////////////////////////////////////


MMC::MMC(module &parent, std::string name, std::string path, params p) {
    h = opMMCNew(parent.handle(), name.c_str(), path.c_str(), 0, p.handle());
    classSet();
}

MMC::MMC(module &parent, std::string name, std::string path) {
    h = opMMCNew(parent.handle(), name.c_str(), path.c_str(), 0, 0);
    classSet();
}

busPortConn *MMC::busPortConnNext(busPortConn *prev) {
    return busPortConn::classGet(opObjectBusPortConnNext(handle(), prev->handle()));
}

busPort *MMC::busPortNext(busPort  *prev) {
    return busPort::classGet(opObjectBusPortNext(handle(), prev->handle()));
}

BY_NAME_PTR_METHOD(busPort,    MMC, optBusPortP,    OP_BUSPORT_EN)

GET_METHOD(MMC, optMMCP)

EXISTING_CONSTRUCTOR(MMC, optMMCP)

////////////////////////////////////////// INITIATOR //////////////////////////////////////////////


object *initiator::classGet(optObjectP existing) {
    if(existing) {
        object *r = (object*)opObjectClass(existing);
        if(r) {
            return r;
        }
        switch(opObjectType(existing)) {
            case OP_PROCESSOR_EN: {
                processor *p = new processor((optProcessorP)existing);
                p->allocated = true;
                return p;
                break;
            }
            case OP_PERIPHERAL_EN: {
                peripheral *p = new peripheral((optPeripheralP)existing);
                p->allocated = true;
                return p;
                break;
            }
            default:
                return 0;
                break;
        }
    } else {
        return 0;
    }
}

/////////////////////////////////////////// BUS SLAVE ///////////////////////////////////////////////

GET_METHOD(busSlave, optBusSlaveP)

EXISTING_CONSTRUCTOR(busSlave, optBusSlaveP)

OP_BUS_SLAVE_READ_FN(busSlave::busRead) {
    busSlave *classPtr = (busSlave*)userData;
    classPtr->read (initiator::classGet(initiator), addr, bytes, data, classPtr->_userData, VA, isFetch);
}

OP_BUS_SLAVE_WRITE_FN(busSlave::busWrite) {
    busSlave *classPtr = (busSlave*)userData;
    classPtr->write (initiator::classGet(initiator), addr, bytes, data, classPtr->_userData, VA);
}


busSlave::busSlave(
    bus        &bus,
    std::string name,
    processor  *proc,
    optPriv     privilege,
    Addr        addrLo,
    Addr        addrHi,
    void*       nativeMemory
) {
    _userData = 0;
    h = opBusSlaveNew (
       bus.handle(),
       name.c_str(),
       proc ? proc->handle() : 0,
       privilege,
       addrLo,
       addrHi,
       0,
       0,
       nativeMemory,
       this
   );
}

// mixture of user-allocated memory and callbacks
busSlave::busSlave (
    bus        &bus,
    std::string name,
    processor  *proc,
    optPriv     privilege,
    Addr        addrLo,
    Addr        addrHi,
    bool        rcb,
    bool        wcb,
    void*       userData,
    void*       nativeMemory
) {
     _userData = userData;

     h = opBusSlaveNew (
        bus.handle(),
        name.c_str(),
        proc ? proc->handle() : 0,
        privilege,
        addrLo,
        addrHi,
        (rcb ? busRead  : 0),
        (wcb ? busWrite : 0),
        nativeMemory,
        this
    );
}

////////////////////////////////////// BUS SLAVE WITH DMI //////////////////////////////////////


OP_BUS_SLAVE_READ_FN(busDMISlave::busReadNoDMI) {
    busDMISlave *classPtr = (busDMISlave*)userData;
    bool dmi;
    classPtr->read(initiator::classGet(initiator), addr, bytes, data, classPtr->iUserData, VA, isFetch, dmi);
}

OP_BUS_SLAVE_WRITE_FN(busDMISlave::busWriteNoDMI) {
    busDMISlave *classPtr = (busDMISlave*)userData;
    bool dmi;
    classPtr->write (initiator::classGet(initiator), addr, bytes, data, classPtr->iUserData, VA, dmi);
}

OP_BUS_SLAVE_READ_FN(busDMISlave::busReadTryDMI) {
    busDMISlave *classPtr = (busDMISlave*)userData;
    bool dmi;
    if(classPtr->read (initiator::classGet(initiator), addr, bytes, data, classPtr->iUserData, VA, isFetch, dmi) && dmi && classPtr->allowDMI) {
        classPtr->tryDMI(addr, bytes);
    }
}

OP_BUS_SLAVE_WRITE_FN(busDMISlave::busWriteTryDMI) {
    busDMISlave *classPtr = (busDMISlave*)userData;
    bool dmi;
    if(classPtr->write (initiator::classGet(initiator), addr, bytes, data, classPtr->iUserData, VA, dmi) && dmi && classPtr->allowDMI) {
        classPtr->tryDMI(addr, bytes);
    }
}

void busDMISlave::tryDMI(Addr addr, Uns32 bytes) {
    // Initialize the by-reference addr parameters in case
    // called routine does not set them when DMI is denied
    Addr       addrMin = addr;
    Addr       addrMax = addr + (bytes - 1);
    DMIaccess  rw;

    void *direct = getDMI(addr, bytes, addrMin, addrMax, rw, iUserData);
    if(direct) {
        // DMI confirmed
        //opPrintf("DMI %s 0x" FMT_64x " 0x" FMT_64x " %p\n", nextName().c_str(), addrMin, addrMax, direct);
        opBusSlaveNew (
            parentBus.handle(),
            0,
            0,
            priv,
            addrMin,
            addrMax,
            (rw == WR) ? busReadNoDMI  : 0,
            (rw == RD) ? busWriteNoDMI : 0,
            direct,
            this
        );
    } else {
        // DMI denied
        //opPrintf("DMI %s 0x" FMT_64x " 0x" FMT_64x "(denied)\n", nextName().c_str(), addrMin, addrMax);
        opBusSlaveNew (
            parentBus.handle(),
            0,
            0,
            priv,
            addrMin,
            addrMax,
            busReadNoDMI,
            busWriteNoDMI,
            0,
            this
       );
    }
}

busDMISlave::busDMISlave (
    bus        &bus,
    std::string name,
    processor  *proc,
    optPriv     privilege,
    Addr        addrLo,
    Addr        addrHi,
    Bool        enableDMI,
    void       *userData
)
: slaveName  (name)
, addrMin    (addrLo)
, addrMax    (addrHi)
, parentBus  (bus)
, priv       (privilege)
, allowDMI   (enableDMI)
, iUserData  (userData)
{
    opBusSlaveNew (
        bus.handle(),
        slaveName.c_str(),
        proc ? proc->handle() : 0,
        privilege,
        addrLo,
        addrHi,
        allowDMI ? busReadTryDMI  : busReadNoDMI,
        allowDMI ? busWriteTryDMI : busWriteNoDMI,
        0,
        this
    );
}

busDMISlave::busDMISlave (
    bus          &bus,
    std::string   name,
    processor    *proc,
    Bool          enableDMI
)
: slaveName  (name)
, addrMin    (0)
, addrMax    (opBusMaxAddress(bus.handle()))
, parentBus  (bus)
, priv       (OP_PRIV_RWX)
, allowDMI   (enableDMI)
 {
    opBusSlaveNew (
        bus.handle(),
        slaveName.c_str(),
        proc ? proc->handle() : 0,
        priv,
        addrMin,
        addrMax,
        allowDMI ? busReadTryDMI  : busReadNoDMI,
        allowDMI ? busWriteTryDMI : busWriteNoDMI,
        0,
        this
    );
}

busDMISlave::busDMISlave (
    bus       &bus,
    std::string     name,
    optPriv    privilege,
    Addr       addrLo,
    Addr       addrHi,
    Bool       enableDMI
)
: slaveName  (name)
, addrMin    (addrLo)
, addrMax    (addrHi)
, parentBus  (bus)
, priv       (privilege)
, allowDMI   (enableDMI)
{
    opBusSlaveNew (
        bus.handle(),
        slaveName.c_str(),
        0,
        privilege,
        addrLo,
        addrHi,
        allowDMI ? busReadTryDMI  : busReadNoDMI,
        allowDMI ? busWriteTryDMI : busWriteNoDMI,
        0,
        this
    );
}

busDMISlave::busDMISlave (
    bus        &bus,
    std::string name,
    Bool        enableDMI
)
: slaveName  (name)
, addrMin    (0)
, addrMax    (opBusMaxAddress(bus.handle()))
, parentBus  (bus)
, priv       (OP_PRIV_RWX)
, allowDMI   (enableDMI)
{
    opBusSlaveNew (
        bus.handle(),
        slaveName.c_str(),
        0,
        OP_PRIV_RWX,
        addrMin,
        addrMax,
        allowDMI ? busReadTryDMI  : busReadNoDMI,
        allowDMI ? busWriteTryDMI : busWriteNoDMI,
        0,
        this
    );
}

/////////////////////////////////////////// DECODE ///////////////////////////////////////////////

decode *decode::find(Addr addr) {
    decode *head = this;
    while(head) {
        if (addr >= head->lo && addr <= head->hi) {
            break;
        }
        head = head->link;
    }
    return head;
}

decode::decode(Addr loAddr, Addr hiAddr, decode  *prev)
: lo(loAddr)
, hi(hiAddr)
, link(prev)
{}

/////////////////////////////////////////// FIFO ///////////////////////////////////////////////


FIFO::FIFO(module &parent, std::string name) {
    h = opFIFONew(parent.handle(), name.c_str(), 0, 0, 0);
    classSet();
}

GET_METHOD(FIFO, optFIFOP)

EXISTING_CONSTRUCTOR(FIFO, optFIFOP)

/////////////////////////////////////////// PACKETNET ///////////////////////////////////////////////


packetnet::packetnet(module &parent, std::string name) {
    h = opPacketnetNew(parent.handle(), name.c_str(), 0, 0);
    classSet();
}

// Create a class wrapper only if required
GET_METHOD(packetnet, optPacketnetP)

EXISTING_CONSTRUCTOR(packetnet, optPacketnetP)

/////////////////////////////////////////// NET /////////////////////////////////////////

net::net(module &parent, std::string name) {
    h = opNetNew(parent.handle(), name.c_str(), 0, 0);
    classSet();
}

netWithCallback::netWithCallback(module &parent, std::string name)
    : net(parent, name)
{
    opNetWriteMonitorAdd(handle(), notifier, this);
}

// Create a class wrapper only if required
GET_METHOD(net, optNetP)

EXISTING_CONSTRUCTOR(net, optNetP)

///////////////////////////////////////// FIFO PORTCONN ///////////////////////////////////////////

//GET_METHOD(FIFOPortConn, optFIFOPortConnP)

//EXISTING_CONSTRUCTOR(FIFOPortConn, optFIFOPortConnP)

///////////////////////////////////////// NET PORTCONN ////////////////////////////////////////////

//GET_METHOD(netPortConn, optNetPortConnP)

//EXISTING_CONSTRUCTOR(netPortConn, optNetPortConnP)

/////////////////////////////////////// PACKETNET PORTCONN ////////////////////////////////////////

//GET_METHOD(packetnetPortConn, optPacketnetPortConnP)

//EXISTING_CONSTRUCTOR(packetnetPortConn, optPacketnetPortConnP)

/////////////////////////////////////////// SESSION ///////////////////////////////////////////////


OP_DEST_BUS_FN(session::oldBus) {
    DEST(bus,optBusP)
}

OP_DEST_FIFOFN(session::oldFIFO) {
    DEST(FIFO,optFIFOP)
}

OP_DEST_NET_FN(session::oldNet) {
    DEST(net,optNetP)
}

OP_DEST_PACKETNET_FN(session::oldPacketnet) {
    DEST(packetnet,optPacketnetP)
}

OP_DEST_BUS_PORT_CONN_FN(session::oldBusPortConn) {
    DEST(busPortConn,optBusPortConnP)
}

OP_DEST_FIFOPORT_CONN_FN(session::oldFIFOPortConn) {
    DEST(FIFOPortConn,optFIFOPortConnP)
}

OP_DEST_NET_PORT_CONN_FN(session::oldNetPortConn) {
    DEST(netPortConn,optNetPortConnP);
}

OP_DEST_PACKETNET_PORT_CONN_FN(session::oldPacketnetPortConn) {
    DEST(packetnetPortConn,optPacketnetPortConnP)
}

OP_DEST_BRIDGE_FN(session::oldBridge) {
    DEST(bridge,optBridgeP)
}

OP_DEST_MEMORY_FN(session::oldMemory) {
    DEST(memory,optMemoryP)
}

OP_DEST_MODULE_FN(session::oldModule) {
    DEST(module,optModuleP)
}

OP_DEST_PROCESSOR_FN(session::oldProcessor) {
    DEST(processor,optProcessorP)
}

OP_DEST_PERIPHERAL_FN(session::oldPeripheral) {
    DEST(peripheral,optPeripheralP)
}

void session::constructorNotifierInstaller(void) {
    optDestCallbacks cb;

    cb.bus               = oldBus;
    cb.FIFO              = oldFIFO;
    cb.net               = oldNet;
    cb.packetnet         = oldPacketnet;

    cb.busPortConn       = oldBusPortConn;
    cb.FIFOPortConn      = oldFIFOPortConn;
    cb.netPortConn       = oldNetPortConn;
    cb.packetnetPortConn = oldPacketnetPortConn;

    cb.bridge            = oldBridge;
    cb.memory            = oldMemory;
    cb.module            = oldModule;
    cb.processor         = oldProcessor;
    cb.peripheral        = oldPeripheral;
    cb.MMC               = 0;

    opSessionDestFnSet(&cb, 0);
}

session::session() {
    opSessionInit(OP_VERSION);
    constructorNotifierInstaller();
}

//////////////////////////////////////////// MESSAGES /////////////////////////////////////////////

void message::print(const char *severity, const char *ident, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    opVMessage(severity, ident, fmt, ap);
    va_end(ap);
}

void message::printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    opVPrintf(fmt, ap);
    va_end(ap);
}

void message::vprint(const char *severity, const char *ident, const char *fmt, va_list ap) {
    opVMessage(severity, ident, fmt, ap);
}

void message::vprintf(const char *fmt, va_list ap) {
    opVPrintf(fmt, ap);
}

void message::vabort(const char *ident, const char *fmt, va_list ap) {
    opVAbort(ident, fmt, ap);
}


const char *message::sprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const char *r = opSprintf(fmt, ap);
    va_end(ap);
    return r;
}

void message::disable(const char *ident) {
    opMessageDisable(ident);
}

void message::enable(const char *ident) {
    opMessageEnable(ident);
}

///////////////////////////////////////////// VLNV ////////////////////////////////////////////////

std::string vlnv::string(
    std::string   vendor,
    std::string   library,
    std::string   name,
    std::string   version,
    optModelType  type,
    optVLNVError  reportErrors
) {
    std::string ret(opVLNVString(
        0,
        vendor .c_str(),
        library.c_str(),
        name   .c_str(),
        version.c_str(),
        type,
        reportErrors
    ));
    return ret;
}

////////////////////////////////////// MODULER TRIGGER ///////////////////////////////////////////

moduleTrigger::moduleTrigger(module &parent, optTime relativetime)
    :parent(parent)
    ,period(relativetime)
{
    event = opModuleTriggerAdd(parent.handle(), relativetime, notifier, this);
}

moduleTrigger::~moduleTrigger() {
    if(event) {
        opModuleTriggerDelete(parent.handle(), event);
    }
}

void moduleTrigger::retrigger() {
    event = opModuleTriggerAdd(parent.handle(), period, notifier, this);
}

void moduleTrigger::retrigger(optTime interval) {
    event = opModuleTriggerAdd(parent.handle(), interval, notifier, this);
}

OP_TRIGGER_FN(moduleTrigger::notifier) {
    CPP_SUPRESS_UNUSED(mi);
    moduleTrigger *classPtr = (moduleTrigger*)userData;
    classPtr->event = 0;
    classPtr->triggered(now);
}

/////////////////////////////////////// WATCHPOINT ///////////////////////////////////////////////

bool watchpoint::triggered(processor *proc, Addr PA, Addr VA, Uns32 bytes, void *data) {
    CPP_SUPRESS_UNUSED(proc);
    CPP_SUPRESS_UNUSED(PA);
    CPP_SUPRESS_UNUSED(VA);
    CPP_SUPRESS_UNUSED(bytes);
    CPP_SUPRESS_UNUSED(data);
    return 0;
}

watchpoint::watchpoint(
    processor        &parent,
    optWatchpointType tp,
    Bool              physical,
    Addr              addrLo,
    Addr              addrHi
) {
    switch(tp) {
        case OP_WP_MEM_READ:
            wpHandle = opProcessorReadWatchpointNew  (parent.handle(), physical, addrLo, addrHi, this, addrNotifier);
            break;
        case OP_WP_MEM_WRITE:
            wpHandle = opProcessorWriteWatchpointNew (parent.handle(), physical, addrLo, addrHi, this, addrNotifier);
            break;
        case OP_WP_MEM_ACCESS:
            wpHandle = opProcessorAccessWatchpointNew(parent.handle(), physical, addrLo, addrHi, this, addrNotifier);
            break;
        case OP_WP_MODE:
            wpHandle = opProcessorModeWatchpointNew(parent.handle(), this, procNotifier);
            break;
        case OP_WP_EXCEPTION:
            wpHandle = opProcessorExceptionWatchpointNew(parent.handle(), this, procNotifier);
            break;
        default:
            cout << "Unrecognised watchpoint type:" << tp << endl;
            break;
    }
}

watchpoint::watchpoint(
    bus              &parent,
    optWatchpointType tp,
    Addr              addrLo,
    Addr              addrHi
) {
    switch(tp) {
        case OP_WP_MEM_READ:
            wpHandle = opBusReadWatchpointNew  (parent.handle(), addrLo, addrHi, this, addrNotifier);
            break;
        case OP_WP_MEM_WRITE:
            wpHandle = opBusWriteWatchpointNew (parent.handle(), addrLo, addrHi, this, addrNotifier);
            break;
        case OP_WP_MEM_ACCESS:
            wpHandle = opBusAccessWatchpointNew(parent.handle(), addrLo, addrHi, this, addrNotifier);
            break;
        default:
            cout << "Unrecognised watchpoint type:" << tp << endl;
            break;
    }
}

watchpoint::watchpoint(
    memory           &parent,
    optWatchpointType tp,
    Addr              addrLo,
    Addr              addrHi
) {
    switch(tp) {
        case OP_WP_MEM_READ:
            wpHandle = opMemoryReadWatchpointNew  (parent.handle(), addrLo, addrHi, this, addrNotifier);
            break;
        case OP_WP_MEM_WRITE:
            wpHandle = opMemoryWriteWatchpointNew (parent.handle(), addrLo, addrHi, this, addrNotifier);
            break;
        case OP_WP_MEM_ACCESS:
            wpHandle = opMemoryAccessWatchpointNew(parent.handle(), addrLo, addrHi, this, addrNotifier);
            break;
        default:
            cout << "Unrecognised watchpoint type:" << tp << endl;
            break;
    }
}

watchpoint::watchpoint(
    processor           &parent,
    optRegWatchpointMode mode,
    reg                 &r
) {
    wpHandle = opProcessorRegWatchpointNew  (parent.handle(), r.handle(), mode, this, procNotifier);
}

// required beacuse watxhpoint is an argument to OP_ADDR_WATCHPOINT_CONDITION_FN
typedef watchpoint watchpointType;

// Note: not using macro OP_ADDR_WATCHPOINT_CONDITION_FN because of name clash
Bool watchpoint::addrNotifier (
    optProcessorP  proc,
    optWatchpointP watchpoint,
    Addr           PA,
    Addr           VA,
    Uns32          bytes,
    void*          userData,
    void*          value
) {
    CPP_SUPRESS_UNUSED(watchpoint);
    watchpointType *classPtr = (watchpointType*)userData;
    processor *p = proc ? (processor*)opObjectClass(proc) : NULL;
    return classPtr->triggered(p, PA, VA, bytes, value);
}

OP_PROC_WATCHPOINT_CONDITION_FN(watchpoint::procNotifier) {
    CPP_SUPRESS_UNUSED (processor);
    CPP_SUPRESS_UNUSED (watchpoint);

    watchpointType *classPtr =  (watchpointType*)userData;
    return classPtr->triggered();
}

///////////////////////////////////////////// MONITOR//////////////////////////////////////////////

// Note : not using the callback macro OP_MONITOR_FN because of a name clash

void monitor::notifier (
    optProcessorP proc,
    Addr          addr,
    Uns32         bytes,
    const void*   data,
    void*         userData,
    Addr          VA
) {

    monitor *classPtr = (monitor *)userData;
    processor *p = proc ? (processor*)opObjectClass(proc) : 0;
    classPtr->triggered(p, addr, VA, bytes, data);
}

void monitor::triggered(processor *proc, Addr PA, Addr VA, Uns32 bytes, const void *data) {
    CPP_SUPRESS_UNUSED(proc);
    CPP_SUPRESS_UNUSED(PA);
    CPP_SUPRESS_UNUSED(VA);
    CPP_SUPRESS_UNUSED(bytes);
    CPP_SUPRESS_UNUSED(data);
}

monitor::monitor(processor &p, bool write, Addr lo, Addr hi)
    : write(write)
    , hi(hi)
    , lo(lo)
    , proc(&p)
    , bs(0)
    , mem(0)
{
    if(write) {
        opProcessorWriteMonitorAdd(proc->handle(), lo, hi, notifier, this);
    } else {
        opProcessorReadMonitorAdd (proc->handle(), lo, hi, notifier, this);
    }
}

monitor::monitor(bus &b, processor &p, bool write, Addr lo, Addr hi)
    : write(write)
    , hi(hi)
    , lo(lo)
    , proc(&p)
    , bs(&b)
    , mem(0)
{
    if(write) {
        opBusWriteMonitorAdd(bs->handle(), proc->handle(), lo, hi, notifier, this);
    } else {
        opBusReadMonitorAdd (bs->handle(), proc->handle(), lo, hi, notifier, this);
    }
}

monitor::monitor(bus &b, bool write, Addr lo, Addr hi)
    : write(write)
    , hi(hi)
    , lo(lo)
    , proc(0)
    , bs(&b)
    , mem(0)
{
    if(write) {
        opBusWriteMonitorAdd(bs->handle(), 0, lo, hi, notifier, this);
    } else {
        opBusReadMonitorAdd (bs->handle(), 0, lo, hi, notifier, this);
    }
}

monitor::monitor(memory &m, processor &p, bool write, Addr lo, Addr hi)
    : write(write)
    , hi(hi)
    , lo(lo)
    , proc(&p)
    , bs(0)
    , mem(&m)
{
    if(write) {
        opMemoryWriteMonitorAdd(mem->handle(),  proc->handle(), lo, hi, notifier, this);
    } else {
        opMemoryReadMonitorAdd(mem->handle(),  proc->handle(), lo, hi, notifier, this);
    }
}

monitor::monitor(memory &m, bool write, Addr lo, Addr hi)
    : write(write)
    , hi(hi)
    , lo(lo)
    , proc(0)
    , bs(0)
    , mem(&m)
{
    if(write) {
        opMemoryWriteMonitorAdd(mem->handle(),  0, lo, hi, notifier, this);
    } else {
        opMemoryReadMonitorAdd(mem->handle(),  0, lo, hi, notifier, this);
    }
}

monitor::~monitor() {
    optProcessorP p = proc ? proc->handle() : 0;
    if (bs) {
        if(write) {
            opBusWriteMonitorDelete(bs->handle(), p, lo, hi, notifier, this);
        } else {
            opBusReadMonitorDelete (bs->handle(), p, lo, hi, notifier, this);
        }
    } else if (mem) {
        if(write) {
            opMemoryWriteMonitorDelete(mem->handle(), p, lo, hi, notifier, this);
        } else {
            opMemoryReadMonitorDelete (mem->handle(), p, lo, hi, notifier, this);
        }
    } else {
        if(write) {
            opProcessorWriteMonitorDelete(p, lo, hi, notifier, this);
        } else {
            opProcessorReadMonitorDelete (p, lo, hi, notifier, this);
        }
    }
}

/////////////////////////////////////////// SHARED DATA ////////////////////////////////////////////


sharedData::sharedData(module &root, const char* version, const char* key, Bool add) {
    r = &root;
    if(add) {
        h = opSharedDataFindAdd(root.handle(), version, key, this);
    } else {
        h = opSharedDataFind(root.handle(), version, key);
    }
}

sharedData::~sharedData() {
    opSharedDataDelete(r->handle(), h);
}

Int32 sharedData::write(Int32 id, void *data) {
    return opSharedDataListenersWrite(h, id, data);
}

//////////////////////////////////// SHARED DATA TRIGGER //////////////////////////////////////////

OP_SHARED_DATA_LISTENER_FN(sharedDataTrigger::notifier) {

    CPP_SUPRESS_UNUSED(ret       );
    CPP_SUPRESS_UNUSED(id        );

    sharedDataTrigger *classPtr = (sharedDataTrigger *)userObject;
    classPtr->triggered(id, userData);
}

sharedDataTrigger::sharedDataTrigger(sharedData &data)
    : data(data)
{
    module *m = data.root();
    opSharedDataListenerRegister(m->handle(), data.handle(), notifier, this);
}

sharedDataTrigger::~sharedDataTrigger() {
    module *m = data.root();
    opSharedDataListenerUnregister(m->handle(), data.handle(), notifier, this);
}

} // end namespace op



