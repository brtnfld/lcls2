#ifndef Pds_Fabrics_Endpoint_hh
#define Pds_Fabrics_Endpoint_hh

#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_rma.h>

#include <vector>
#include <poll.h>
#include <sys/uio.h>

namespace Pds {
  namespace Fabrics {
    enum State { EP_CLOSED, EP_INIT, EP_UP, EP_LISTEN, EP_CONNECTED };

    class RemoteAddress {
    public:
      RemoteAddress();
      RemoteAddress(uint64_t rkey, uint64_t addr, size_t extent);
      const struct fi_rma_iov* rma_iov() const;
    public:
      uint64_t addr;
      size_t extent;
      uint64_t rkey;
    };

    class MemoryRegion {
    public:
      MemoryRegion(struct fid_mr* mr, void* start, size_t len);
      ~MemoryRegion();
      uint64_t rkey() const;
      uint64_t addr() const;
      void* desc() const;
      void* start() const;
      size_t length() const;
      struct fid_mr* fid() const;
      bool contains(void* start, size_t len) const;
    private:
      struct fid_mr* _mr;
      void*          _start;
      size_t         _len;
    };

    class LocalAddress {
    public:
      LocalAddress();
      LocalAddress(void* buf, size_t len, MemoryRegion* mr=NULL);
      void* buf() const;
      size_t len() const;
      MemoryRegion* mr() const;
      const struct iovec* iovec() const;
    public:
      void*         _buf;
      size_t        _len;
      MemoryRegion* _mr;
    };

    class LocalIOVec {
    public:
      LocalIOVec(size_t count=1);
      LocalIOVec(LocalAddress* local_addrs, size_t count);
      LocalIOVec(const std::vector<LocalAddress*>& local_addrs);
      ~LocalIOVec();
      void reset();
      bool check_mr();
      size_t count() const;
      const struct iovec* iovecs() const;
      void** desc() const;
      void* allocate();
      bool add_iovec(LocalAddress* local_addr, size_t count=1);
      bool add_iovec(std::vector<LocalAddress*>& local_addr);
      bool add_iovec(void* buf, size_t len, MemoryRegion* mr=NULL);
      bool set_count(size_t count);
      bool set_iovec(unsigned index, LocalAddress* local_addr);
      bool set_iovec(unsigned index, void* buf, size_t len, MemoryRegion* mr=NULL);
      bool set_iovec_mr(unsigned index, MemoryRegion* mr);
    private:
      void check_size(size_t count);
      void verify();
    private:
      bool          _mr_set;
      size_t        _count;
      size_t        _max;
      struct iovec* _iovs;
      void**        _mr_desc;
    };

    class RemoteIOVec {
    public:
      RemoteIOVec(size_t count=1);
      RemoteIOVec(RemoteAddress* remote_addrs, size_t count);
      RemoteIOVec(const std::vector<RemoteAddress*>& remote_addrs);
      ~RemoteIOVec();
      size_t count() const;
      const struct fi_rma_iov* iovecs() const;
      bool add_iovec(RemoteAddress* remote_addr, size_t count=1);
      bool add_iovec(std::vector<RemoteAddress*>& remote_addr);
      bool add_iovec(unsigned index, uint64_t rkey, uint64_t addr, size_t extent);
      bool set_iovec(unsigned index, RemoteAddress* remote_addr);
      bool set_iovec(unsigned index, uint64_t rkey, uint64_t addr, size_t extent);
    private:
      void check_size(size_t count);
    private:
      size_t              _count;
      size_t              _max;
      struct fi_rma_iov*  _rma_iovs;
    };

    class RmaMessage {
    public:
      RmaMessage();
      RmaMessage(LocalIOVec* loc_iov, RemoteIOVec* rem_iov, void* context, uint64_t data=0);
      ~RmaMessage();
      LocalIOVec* loc_iov() const;
      RemoteIOVec* rem_iov() const;
      void loc_iov(LocalIOVec* loc_iov);
      void rem_iov(RemoteIOVec* rem_iov);
      const struct iovec* msg_iov() const;
      size_t iov_count() const;
      void** desc() const;
      const struct fi_rma_iov* rma_iov() const;
      size_t rma_iov_count() const;
      void* context() const;
      void context(void* context);
      uint64_t data() const;
      void data(uint64_t data);
      const struct fi_msg_rma* msg() const;
    private:
      LocalIOVec*         _loc_iov;
      RemoteIOVec*        _rem_iov;
      struct fi_msg_rma*  _msg;
    };

    class ErrorHandler {
    public:
      ErrorHandler();
      virtual ~ErrorHandler();
      int error_num() const;
      const char* error() const;
      void clear_error();
    protected:
      void set_custom_error(const char* fmt, ...);
      void set_error(const char* error_desc);
    protected:
      int   _errno;
      char* _error;
    };

    class Fabric : public ErrorHandler {
    public:
      Fabric(const char* node, const char* service, uint64_t flags=0);
      ~Fabric();
      MemoryRegion* register_memory(void* start, size_t len);
      MemoryRegion* register_memory(LocalAddress* laddr);
      MemoryRegion* lookup_memory(void* start, size_t len) const;
      MemoryRegion* lookup_memory(LocalAddress* laddr) const;
      bool lookup_memory_iovec(LocalIOVec* iov) const;
      bool up() const;
      bool has_rma_event_support() const;
      const char* name() const;
      const char* provider() const;
      uint32_t version() const;
      struct fi_info* info() const;
      struct fid_fabric* fabric() const;
      struct fid_domain* domain() const;
    private:
      bool initialize(const char* node, const char* service, uint64_t flags);
      void shutdown();
    private:
      bool                        _up;
      struct fi_info*             _hints;
      struct fi_info*             _info;
      struct fid_fabric*          _fabric;
      struct fid_domain*          _domain;
      std::vector<MemoryRegion*>  _mem_regions;
    };

    class EndpointBase : public ErrorHandler {
    protected:
      EndpointBase(const char* addr, const char* port, uint64_t flags=0);
      EndpointBase(Fabric* fabric);
      virtual ~EndpointBase();
    public:
      State state() const;
      Fabric* fabric() const;
      struct fid_eq* eq() const;
      struct fid_cq* cq() const;
      virtual void shutdown();
      bool event(uint32_t* event, void* entry, bool* cm_entry);
      bool event_wait(uint32_t* event, void* entry, bool* cm_entry, int timeout=-1);
      bool event_error(struct fi_eq_err_entry *entry);
    protected:
      bool handle_event(ssize_t event_ret, bool* cm_entry, const char* cmd);
      bool initialize();
    protected:
      State                       _state;
      const bool                  _fab_owner;
      Fabric*                     _fabric;
      struct fid_eq*              _eq;
      struct fid_cq*              _cq;
    };

    class Endpoint : public EndpointBase {
    public:
      Endpoint(const char* addr, const char* port, uint64_t flags=0);
      Endpoint(Fabric* fabric);
      ~Endpoint();
    public:
      void shutdown();
      bool connect();
      bool accept(struct fi_info* remote_info);
      bool comp(struct fi_cq_data_entry* comp, int* comp_num, ssize_t max_count);
      bool comp_wait(struct fi_cq_data_entry* comp, int* comp_num, ssize_t max_count, int timeout=-1);
      bool comp_error(struct fi_cq_err_entry* comp_err);
      /* Asynchronous calls (raw buffer) */
      bool recv_comp_data();
      bool send(void* buf, size_t len, void* context, const MemoryRegion* mr=NULL);
      bool recv(void* buf, size_t len, void* context, const MemoryRegion* mr=NULL);
      bool read(void* buf, size_t len, const RemoteAddress* raddr, void* context, const MemoryRegion* mr=NULL);
      bool write(void* buf, size_t len, const RemoteAddress* raddr, void* context, const MemoryRegion* mr=NULL);
      bool write_data(void* buf, size_t len, const RemoteAddress* raddr, void* context, uint64_t data, const MemoryRegion* mr=NULL);
      /* Asynchronous calls (LocalAddress wrapper) */
      bool send(LocalAddress* laddr, void* context);
      bool recv(LocalAddress* laddr, void* context);
      bool read(LocalAddress* laddr, const RemoteAddress* raddr, void* context);
      bool write(LocalAddress* laddr, const RemoteAddress* raddr, void* context);
      bool write_data(LocalAddress* laddr, const RemoteAddress* raddr, void* context, uint64_t data);
      /* Vectored Asynchronous calls */
      bool sendv(LocalIOVec* iov, void* context);
      bool recvv(LocalIOVec* iov, void* context);
      bool readv(LocalIOVec* iov, const RemoteAddress* raddr, void* context);
      bool writev(LocalIOVec* iov, const RemoteAddress* raddr, void* context);
      bool readmsg(RmaMessage* msg, uint64_t flags);
      bool writemsg(RmaMessage* msg, uint64_t flags);
      /* Synchronous calls (raw buffer) */
      bool recv_comp_data_sync(uint64_t* data=NULL);
      bool send_sync(void* buf, size_t len, const MemoryRegion* mr=NULL);
      bool recv_sync(void* buf, size_t len, const MemoryRegion* mr=NULL);
      bool read_sync(void* buf, size_t len, const RemoteAddress* raddr, const MemoryRegion* mr=NULL);
      bool write_sync(void* buf, size_t len, const RemoteAddress* raddr, const MemoryRegion* mr=NULL);
      bool write_data_sync(void* buf, size_t len, const RemoteAddress* raddr, uint64_t data, const MemoryRegion* mr=NULL);
      /* Synchronous calls (LocalAddress wrapper) */
      bool send_sync(LocalAddress* laddr);
      bool recv_sync(LocalAddress* laddr);
      bool read_sync(LocalAddress* laddr, const RemoteAddress* raddr);
      bool write_sync(LocalAddress* laddr, const RemoteAddress* raddr);
      bool write_data_sync(LocalAddress* laddr, const RemoteAddress* raddr, uint64_t data);
      /* Vectored Synchronous calls */
      bool sendv_sync(LocalIOVec* iov);
      bool recvv_sync(LocalIOVec* iov);
      bool readv_sync(LocalIOVec* iov, const RemoteAddress* raddr);
      bool writev_sync(LocalIOVec* iov, const RemoteAddress* raddr);
      bool readmsg_sync(RmaMessage* msg, uint64_t flags=0);
      bool writemsg_sync(RmaMessage* msg, uint64_t flags=0);
    private:
      bool post_comp_data_recv(void* context=NULL);
      bool handle_comp(ssize_t comp_ret, struct fi_cq_data_entry* comp, int* comp_num, const char* cmd);
      bool check_completion(int context, unsigned flags, uint64_t* data=0);
      bool check_completion_noctx(unsigned flags, uint64_t* data=0);
      bool check_connection_state();
    private:
      uint64_t        _counter;
      struct fid_ep*  _ep;
    };

    class PassiveEndpoint : public EndpointBase {
    public:
      PassiveEndpoint(const char* addr, const char* port, uint64_t flags=0);
      ~PassiveEndpoint();
    public:
      void shutdown();
      bool listen();
      Endpoint* accept();
      bool reject();
      bool close(Endpoint* endpoint);
    private:
      int                     _flags;
      struct fid_pep*         _pep;
      std::vector<Endpoint*>  _endpoints;
    };

    class CompletionPoller : public ErrorHandler {
    public:
      CompletionPoller(Fabric* fabric, nfds_t size_hint=1);
      ~CompletionPoller();
      bool up() const;
      bool add(Endpoint* endp);
      bool del(Endpoint* endp);
      bool poll(int timeout=-1);
      void shutdown();
    private:
      bool initialize();
      void check_size();
    private:
      bool          _up;
      Fabric*       _fabric;
      nfds_t        _nfd;
      nfds_t        _nfd_max;
      pollfd*       _pfd;
      struct fid**  _pfid;
      Endpoint**    _endps;
    };
  }
}

#endif