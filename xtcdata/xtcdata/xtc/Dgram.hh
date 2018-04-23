#ifndef XtcData_Dgram_hh
#define XtcData_Dgram_hh

#include "Sequence.hh"
#include "Xtc.hh"
#include <stdint.h>

#pragma pack(push,4)

namespace XtcData
{

class Transition {
public:
    Sequence seq;
    unsigned evtCounter:24;
    unsigned version:8;
    uint32_t env[3];
};

class L1Transition : public Transition {
public:
    uint16_t trigLines()     const { return (env[0]>>16)&0xffff; }
    uint16_t readoutGroups() const { return (env[0])&0xffff; }
};

class Dgram : public Transition {
public:
    Xtc xtc;
};

}

#pragma pack(pop)

#endif
