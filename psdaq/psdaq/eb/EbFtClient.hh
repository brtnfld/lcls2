#ifndef Pds_Eb_EbFtClient_hh
#define Pds_Eb_EbFtClient_hh

#include "EbFtBase.hh"

#include <stdint.h>
#include <cstddef>
#include <string>
#include <vector>

namespace Pds {
  namespace Fabrics {

    class Endpoint;
    class MemoryRegion;
    class RemoteAddress;

  };

#define StringList std::vector<std::string>
#define EpList     std::vector<Fabrics::Endpoint*>
#define MrList     std::vector<Fabrics::MemoryRegion*>
#define RaList     std::vector<Fabrics::RemoteAddress>

  namespace Eb {

    class EbFtClient : public EbFtBase
    {
    public:
      EbFtClient(StringList& peers,
                 StringList& port,
                 size_t      rmtSize);
      virtual ~EbFtClient();
    public:
      int connect(unsigned id, unsigned tmo);
    public:
      virtual int shutdown();
    private:
      int _connect(unsigned                myId,
                   std::string&            peer,
                   std::string&            port,
                   unsigned                tmo,
                   char*                   pool,
                   Fabrics::Endpoint*&     ep,
                   Fabrics::MemoryRegion*& mr,
                   unsigned&               id);
    private:
      StringList& _peers;               // List of peers
      StringList& _port;                // The port to listen on
      size_t      _lclSize;             // Local  memory region size
      size_t      _rmtSize;             // Remote memory region size
      char*       _base;                // The local memory region
    };
  };
};

#endif