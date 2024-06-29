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

#include "tlm/tlmProcessor.hpp"

namespace cw {

/*
 * without params
 */
tlmProcessor::tlmProcessor(tlmModule &parent, string path, sc_module_name name)
    : sc_module       (name)
    , processor       (parent, path, sc_object::name(), setSystemC())
    , traceQuantaOn   (false)
    , traceBusesOn    (false)
    , traceBusErrorsOn(false)
    , traceSignalsOn  (false)
    , dmiOn           (true)
    , localQuantum    (1, SC_MS)
{
    setTracing();
    startThreads();
}

/*
 * with params
 */
tlmProcessor::tlmProcessor(tlmModule &parent, string path, sc_module_name name, params p)
    : sc_module       (name)
    , processor       (parent, path, sc_object::name(), setSystemC(p))
    , traceQuantaOn   (false)
    , traceBusesOn    (false)
    , traceBusErrorsOn(false)
    , traceSignalsOn  (false)
    , dmiOn           (true)
    , localQuantum    (1, SC_MS)
{
    setTracing();
    startThreads();
}

/*
 * The 'finished' variable used to be static (global) so that one processor
 * finishing would finish all. This is not correct in a multi-platform OP
 * situation. I think that the controls in the PC layer will handle this
 * anyway, so now it's per thread.
 */
void tlmProcessor::processorThread(processor *p) {

    optTime endOfQuantum = 0;
    bool    finished     = false;

    wait(sc_core::SC_ZERO_TIME);  // initial wait

    while(!finished) {
        sc_time quantum  = localQuantum;
        optTime seconds  = quantum.to_seconds();
        endOfQuantum += seconds;
        Uns64   instToDo = p->clocksUntilTime(endOfQuantum);

        if (traceQuantaOn) {
            opPrintf("TLM: %s running " FMT_64d " instructions\n", p->hierCName(), instToDo);
        }
        optStopReason r = p->simulate(instToDo);
        switch(r) {
            case OP_SR_HALT:
            case OP_SR_SCHED:
            case OP_SR_YIELD:
            case OP_SR_FREEZE:
                // restart on a halt
                break;

            case OP_SR_FINISH:
                finished = 1;
                // fall through
            default:
                return;
        }
        wait(quantum);
    }
}

/*
 * Need pass a static function to sc_spawn, the thread function is
 * a true method.
 */
void tlmProcessor::threadEntry(tlmProcessor *t, processor *p) {
    t->processorThread(p);
}

/*
 * recurse through a multicore, finding the leaf processors
 */
void tlmProcessor::explore(tlmProcessor *t, processor *p) {

    if(p->isLeaf()) {
        const char *name = p->hierCName();
        char sname[strlen(name)+1];
        const char *s;
        char  *d;

        // make a legal SystemC name without dots in it.
        for(s=name, d=sname; *s; s++, d++) {
            if(*s == '.') {
                *d = '_';
            } else {
                *d = *s;
            }
        }
        *d =0;

        sc_spawn_options opts;
        opts.set_stack_size(STACK_SIZE);

        sc_spawn(sc_bind(&cw::tlmProcessor::threadEntry, t, p), sname, &opts);
    } else {
        processor *c;
        for(c = p->child(); c ; c = c->siblingNext()) {
            explore(t, c);
        }
    }
}

void tlmProcessor::startThreads() {

    explore(this, this);
}

}
