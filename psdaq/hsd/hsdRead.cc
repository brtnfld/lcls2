
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <new>

FILE*               writeFile           = 0;

void sigHandler( int signal ) {
  psignal( signal, "Signal received by pgpWidget");
  if (writeFile) fclose(writeFile);
  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}


#include "pgp/pgpcardG3/include/PgpCardMod.h"

using namespace std;

void printUsage(char* name) {
  printf( "Usage: %s [-h]  -P <deviceName> [options]\n"
      "    -h        Show usage\n"
      "    -P        Set pgpcard device name  (REQUIRED)\n"
      "    -L <num>  Number of lanes\n"
      "    -c        number of times to read\n"
      "    -o        Print out up to maxPrint words when reading data\n"
      "    -f <file> Record to file\n"
      "    -d <nsec> Delay given number of nanoseconds per event\n"
      "    -D        Set debug value           [Default: 0]\n"
      "                bit 00          print out progress\n"
      "    -N        Exit after N events\n"
      "    -r        Report rate\n"
      "    -v        Validate each event\n",
      name
  );
}

void* countThread(void*);

static int      count = 0;
static int64_t  bytes = 0;
static unsigned lanes = 0;
static unsigned buffs = 0;

int main (int argc, char **argv) {
  PgpCardRx     pgpCardRx;
  PgpCardTx     pgpCardTx;
  int           x;
  int           ret, fd;
  int           numb;
  bool          print = false;
  uint          maxSize;
  uint          *data;
  char          err[128];
  char          pgpcard[128]              = "";
  int                 maxPrint            = 1024;
  bool                cardGiven           = false;
  unsigned            debug               = 0;
  unsigned            nevents             = unsigned(-1);
  unsigned            delay               = 0;
  bool                lvalidate           = false;
  unsigned            payloadBuffers      = 0;
  bool                reportRate          = false;
  unsigned            nlanes              = 0;
  ::signal( SIGINT, sigHandler );

  //  char*               endptr;
  extern char*        optarg;
  int c;
  while( ( c = getopt( argc, argv, "hP:L:d:D:c:f:N:o:rvV:" ) ) != EOF ) {
    switch(c) {
    case 'P':
      strcpy(pgpcard, optarg);
      cardGiven = true;
      break;
    case 'L':
      { unsigned nl = strtoul(optarg,NULL,0);
        if (nl > 0 && nl <= 8)
          nlanes = nl-1;
        printf("Asking for %d lanes; granting %d\n",nl,nlanes+1);
      }
      break;
    case 'N':
      nevents = strtoul(optarg,NULL, 0);
      break;
    case 'D':
      debug = strtoul(optarg, NULL, 0);
      if (debug & 1) print = true;
      break;
    case 'd':
      delay = strtoul(optarg, NULL, 0);
      break;
    case 'c':
      numb = strtoul(optarg  ,NULL,0);
      break;
    case 'f':
      if (!(writeFile = fopen(optarg,"w"))) {
        perror("Opening save file");
        return -1;
      }
      break;
    case 'o':
      maxPrint = strtoul(optarg, NULL, 0);
      print = true;
      break;
    case 'r':
      reportRate = true;
      break;
    case 'v':
      lvalidate = true;
      break;
    case 'V':
      payloadBuffers = strtoul(optarg, NULL, 0);
      break;
    case 'h':
      printUsage(argv[0]);
      return 0;
      break;
    default:
      printf("Error: Option could not be parsed, or is not supported yet!\n");
      printUsage(argv[0]);
      return 0;
      break;
    }
  }

  if (!cardGiven) {
    printf("PGP card must be specified !!\n");
    printUsage(argv[0]);
    return 1;
  }
  fd = open( pgpcard,  O_RDWR );
  if (fd < 0) {
    sprintf(err, "%s opening %s failed", argv[0], pgpcard);
    perror(err);
    return 1;
  }

  pgpCardTx.cmd = IOCTL_Set_Debug;
  pgpCardTx.model = sizeof(&pgpCardTx);
  pgpCardTx.size = sizeof(PgpCardTx);
  pgpCardTx.data = reinterpret_cast<__u32*>(debug);

  ret = write(fd,&pgpCardTx,sizeof(PgpCardTx));

  if (nlanes > 0) {
    pgpCardTx.cmd = IOCTL_Add_More_Ports;
    pgpCardTx.data = reinterpret_cast<__u32*>(nlanes);
    ret = write(fd,&pgpCardTx,sizeof(PgpCardTx));
  }

  // Allocate a buffer
  maxSize = 1024*1024*2;
  data = (uint *)malloc(sizeof(uint)*maxSize);

  pgpCardRx.maxSize = maxSize;
  pgpCardRx.data    = reinterpret_cast<__u32*>(data);
  pgpCardRx.model   = sizeof(data);

  pthread_attr_t tattr;
  pthread_attr_init(&tattr);
  pthread_t thr;
  if (reportRate) {
    if (pthread_create(&thr, &tattr, &countThread, 0))
      perror("Error creating RDMA status thread");
  }

  unsigned nextCount=0, nextPword=0;
  uint64_t ppulseId =0, dpulseId =0;

  // DMA Read
  do {
    ret = read(fd,&pgpCardRx,sizeof(PgpCardRx));

    if (nevents-- == 0)
      break;

    if ( ret != 0 ) {
      if (print) {

        cout << "Ret=" << dec << ret;
        cout << ", pgpLane=" << dec << pgpCardRx.pgpLane;
        cout << ", pgpVc=" << dec << pgpCardRx.pgpVc;
        cout << ", EOFE=" << dec << pgpCardRx.eofe;
        cout << ", FifoErr=" << dec << pgpCardRx.fifoErr;
        cout << ", LengthErr=" << dec << pgpCardRx.lengthErr;
        cout << endl << "   ";

        for (x=0; x<ret && x<maxPrint; x++) {
          cout << " 0x" << setw(8) << setfill('0') << hex << data[x];
          if ( ((x+1)%10) == 0 ) cout << endl << "   ";
        }
        cout << endl;

        if (count >= numb)
          print = false;
      }
      if (ret>0)
        bytes += ret*sizeof(uint);

      if (lvalidate) {
        //  Check that pulseId increments by a constant
        uint64_t pulseId = (uint64_t(data[1])<<32) | data[0];
        if (ppulseId) {
          if (dpulseId > 100 && pulseId != (ppulseId+dpulseId))
            printf("\tPulseId = %016llx [%016llx, %016llx]\n", 
                   (unsigned long long)pulseId, 
                   (unsigned long long)(pulseId+dpulseId), 
                   (unsigned long long)(pulseId-ppulseId));
          dpulseId = pulseId - ppulseId;
        }
        ppulseId = pulseId;
        //  Check that analysis count increments by one
        if (nextCount && data[5] != nextCount)
          printf("\tanalysisCount = %08x [%08x]\n", data[5], nextCount);
        nextCount = data[5]+1;
        //  Check that the first payload word increments by one
        if (payloadBuffers) {
          if (nextPword && data[8] != nextPword)
            printf("\tpayloadWord = %08x [%08x]\n", data[8], nextPword);
          nextPword = (data[8]+1)%payloadBuffers;
        }
      }

      lanes |= 1<<pgpCardRx.pgpLane;

      { unsigned buff = data[9]>>16;
        buffs |= (1<<buff); }

      ++count;

      if (writeFile) {
        data[6] |= (pgpCardRx.pgpLane<<20);  // write the lane into the event header
        fwrite(pgpCardRx.data,sizeof(unsigned),ret,writeFile);
      }

      if (delay) {
        timespec tv = { .tv_sec=0, .tv_nsec=delay };
        while( nanosleep(&tv, &tv) )
          ;
      }
    }
    else {
      printf("ret == 0\n");
    }
  } while ( ret > 0 );
  count = -1;
  if (ret < 0) {
    sprintf(err, "%s reading %s failed ", argv[0], pgpcard);
    perror(err);
    return 1;
  }
  if (reportRate)
    pthread_join(thr,NULL);
  free(data);
  //  sleep(5);
  //  close(fd);
  return 0;
}

void* countThread(void* args)
{
  timespec tv;
  clock_gettime(CLOCK_REALTIME,&tv);
  unsigned ocount = count;
  int64_t  obytes = bytes;
  while(1) {
    usleep(1000000);
    timespec otv = tv;
    clock_gettime(CLOCK_REALTIME,&tv);
    unsigned ncount = count;
    int64_t  nbytes = bytes;

    double dt     = double( tv.tv_sec - otv.tv_sec) + 1.e-9*(double(tv.tv_nsec)-double(otv.tv_nsec));
    double rate   = double(ncount-ocount)/dt;
    double dbytes = double(nbytes-obytes)/dt;
    unsigned dbsc = 0, rsc=0;
    
    if (count < 0) break;

    static const char scchar[] = { ' ', 'k', 'M' };
    if (rate > 1.e6) {
      rsc     = 2;
      rate   *= 1.e-6;
    }
    else if (rate > 1.e3) {
      rsc     = 1;
      rate   *= 1.e-3;
    }

    if (dbytes > 1.e6) {
      dbsc    = 2;
      dbytes *= 1.e-6;
    }
    else if (dbytes > 1.e3) {
      dbsc    = 1;
      dbytes *= 1.e-3;
    }
    
    printf("Rate %7.2f %cHz [%u]:  Size %7.2f %cBps [%lld B]  lanes %02x  buffs %04x\n", 
           rate  , scchar[rsc ], ncount, 
           dbytes, scchar[dbsc], (long long)nbytes, lanes, buffs);
    lanes = 0;
    buffs = 0;

    ocount = ncount;
    obytes = nbytes;
  }
  return 0;
}
