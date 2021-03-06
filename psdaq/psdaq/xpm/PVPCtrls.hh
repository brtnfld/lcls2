#ifndef Xpm_PVPCtrls_hh
#define Xpm_PVPCtrls_hh

#include <string>
#include <vector>

namespace Pds_Epics { class PVBase; }

namespace Pds {

  class Semaphore;

  namespace Xpm {

    class Module;
    class PVPStats;

    class PVPCtrls
    {
    public:
      PVPCtrls(Module&,
               Semaphore&,
               PVPStats&,
               unsigned shelf,
               unsigned partition);
      ~PVPCtrls();
    public:
      void allocate(const std::string& title);
      void enable(unsigned shelf);
      void update();
      bool enabled() const;
      void setPartition();
    public:
      Module& module();
      Semaphore& sem();
    public:
      void l0Select  (unsigned v);
      void fixedRate (unsigned v);
      void acRate    (unsigned v);
      void acTimeslot(unsigned v);
      void seqIdx    (unsigned v);
      void seqBit    (unsigned v);
      void dstSelect (unsigned v);
      void dstMask   (unsigned v);
      void msgHeader (unsigned v);
      void msgPayload(unsigned v);
      void configKey (unsigned v);

      void setL0Select ();
      void setDstSelect();
      void msgInsert   ();
      void msg_config  ();
      void msg_enable  ();
      void msg_disable ();
      void msg_clear   ();
      void dump() const;
    public:
      enum { FixedRate, ACRate, Sequence };
    private:
      std::vector<Pds_Epics::PVBase*> _pv;
      Module&    _m;
      Semaphore& _sem;
      PVPStats&  _stats;
      unsigned _shelf;
      unsigned _partition;
      bool     _enabled;
      unsigned _l0Select;
      unsigned _fixedRate;
      unsigned _acRate;
      unsigned _acTimeslot;
      unsigned _seqIdx;
      unsigned _seqBit;
      unsigned _dstSelect;
      unsigned _dstMask;
      unsigned _msgHdr;
      unsigned _msgPayload;
      unsigned _cfgKey;
    };
  };
};

#endif
