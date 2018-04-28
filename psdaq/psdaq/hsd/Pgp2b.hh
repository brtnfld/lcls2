#ifndef HSD_Pgp2b_hh
#define HSD_Pgp2b_hh

#include "psdaq/hsd/Pgp.hh"

namespace Pds {
  namespace Mmhw { class Pgp2bAxi; }
  namespace HSD {
    class Pgp2b : public Pgp {
    public:
      Pgp2b(Mmhw::Pgp2bAxi&);
    public:
      virtual bool   localLinkReady () const;
      virtual bool   remoteLinkReady() const;
      virtual double   txClkFreqMHz () const;
      virtual double   rxClkFreqMHz () const;
      virtual unsigned txCount      () const;
      virtual unsigned txErrCount   () const;
      virtual unsigned rxOpCodeCount() const;
      virtual unsigned rxOpCodeLast () const;
    private:
      Mmhw::Pgp2bAxi& _axi;
    };
  };
};

#endif
