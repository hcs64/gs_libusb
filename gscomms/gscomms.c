#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#include "gscomms.h"
//

static const uint16_t VENDOR_ID = 0x9710;
static const uint16_t PRODUCT_ID = 0x7705;

static const uint8_t REQTYPE_READ =
 LIBUSB_ENDPOINT_IN |
 LIBUSB_TRANSFER_TYPE_CONTROL |
 LIBUSB_REQUEST_TYPE_VENDOR;

static const uint8_t REQTYPE_WRITE =
 LIBUSB_ENDPOINT_OUT |
 LIBUSB_TRANSFER_TYPE_CONTROL |
 LIBUSB_REQUEST_TYPE_VENDOR;

static const uint8_t ENDPOINT_MOS_BULK_READ = 0x81;
static const uint8_t ENDPOINT_MOS_BULK_WRITE = 0x02;

static const uint8_t REQ_MOS_WRITE = 0x0E;
static const uint8_t REQ_MOS_READ  = 0x0D;

// value field, only one port on the 7705
enum { MOS_PP_PORT = 0 };
static const uint16_t MOS_PORT_BASE = (MOS_PP_PORT+1)<<8;

// index field, simulated IBM PC interface
static const uint16_t MOS_PP_DATA_REG   = 0x0000;
static const uint16_t MOS_PP_STATUS_REG = 0x0001;
static const uint16_t MOS_PP_CONTROL_REG= 0x0002;
static const uint16_t MOS_PP_DEBUG_REG  = 0x0004;
static const uint16_t MOS_PP_EXTENDED_CONTROL_REG = 0x000A;

static const uint8_t MOS_SPP_MODE    = 0 << 5;
static const uint8_t MOS_NIBBLE_MODE = 1 << 5; // default on reset
static const uint8_t MOS_FIFO_MODE   = 2 << 5;

static const int TIMEOUT = 10*1000; // ms

static void WriteRAMStart(gscomms * g, unsigned long address, unsigned long length);
static void WriteRAMByte(gscomms * g, unsigned char b);
static void WriteRAMFinish(gscomms * g);
// 

void set_mode(gscomms * g, int mode) {
  uint8_t mos_mode;

  switch (mode) {
    case GSCOMMS_MODE_CAREFUL:
    case GSCOMMS_MODE_STANDARD:
    case GSCOMMS_MODE_FAST:
      mos_mode = MOS_SPP_MODE;
      break;
    case GSCOMMS_MODE_BULK:
      fprintf(stderr, "bulk mode unsupported\n");
      exit(-1);
      break;
    default:
      fprintf(stderr, "mode #%d unsupported\n", mode);
      exit(-1);
      break;
  }

  g->mode = mode;

  int rc = libusb_control_transfer(
      g->dev, 
      REQTYPE_WRITE,
      REQ_MOS_WRITE,
      MOS_PORT_BASE | mos_mode,
      MOS_PP_EXTENDED_CONTROL_REG,
      NULL,
      0,
      TIMEOUT
      );

  if (rc != 0) {
    fprintf(stderr, "mode set failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }
}

#if 0
void get_clock(gscomms * g) {
  unsigned char data;
  int rc = libusb_control_transfer(
      g->dev,
      REQTYPE_READ,
      REQ_MOS_READ,
      MOS_PORT_BASE,
      MOS_PP_DEBUG_REG,
      &data,
      1,
      TIMEOUT);

  if (rc != 1) {
    fprintf(stderr, "clock read failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }

  printf("PPREG %02x\n", data);
}
#endif

uint8_t do_raw_read(gscomms * g) {
  unsigned char data;

  int rc = libusb_control_transfer(
      g->dev,
      REQTYPE_READ,
      REQ_MOS_READ,
      MOS_PORT_BASE,
      MOS_PP_STATUS_REG,
      &data,
      1,
      TIMEOUT);

  if (rc != 1) {
    fprintf(stderr, "read failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }

  return data;
}

uint8_t do_read(gscomms * g) {
  unsigned char data;

  data = do_raw_read(g);

  if (data & 0x08) {
    return ((data^0x80)>>4)|0x10;
  }

  return 0;
}

void do_clear(gscomms * g) {
  do_write(g, 0, 0);
}

void do_write(gscomms * g, uint8_t data,int flagged) {

  uint8_t flagged_data = (flagged ? 0x10 : 0) | (data & 0xf);
  //printf("%02x\n", flagged_data);

  int rc = libusb_control_transfer(
      g->dev,
      REQTYPE_WRITE,
      REQ_MOS_WRITE,
      MOS_PORT_BASE | flagged_data,
      MOS_PP_DATA_REG,
      NULL,
      0,
      TIMEOUT);

  if (rc != 0) {
    fprintf(stderr, "write failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }
}

void do_write_async_cb(struct libusb_transfer * transfer) {
  gscomms * g = transfer->user_data;
  g->writes_pending --;
  if (g->writes_pending < 0) {
    fprintf(stderr, "writes pending underflow!\n");
    exit(-1);
  }
  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    fprintf(stderr, "async transfer failed: %d\n", transfer->status);
    exit(-1);
  }
}

void do_write_async(gscomms * g, uint8_t data, int flagged) {
  uint8_t flagged_data = (flagged ? 0x10 : 0) | (data & 0xf);
  //printf("%02x\n", flagged_data);

  struct libusb_transfer * transfer = libusb_alloc_transfer(0);

  unsigned char *setup_buffer = malloc(LIBUSB_CONTROL_SETUP_SIZE);

  libusb_fill_control_setup(
      setup_buffer,
      REQTYPE_WRITE,
      REQ_MOS_WRITE,
      MOS_PORT_BASE | flagged_data,
      MOS_PP_DATA_REG,
      0);

  libusb_fill_control_transfer(
      transfer,
      g->dev,
      setup_buffer,
      do_write_async_cb,
      g,
      TIMEOUT);

  transfer->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
  transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
  transfer->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;

  int rc = libusb_submit_transfer(transfer);
  if (rc != 0) {
    fprintf(stderr, "submit async write failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }

  g->writes_pending ++;
}

void do_clear_async(gscomms * g) {
  do_write_async(g, 0, 0);
}

void do_fast_write(gscomms * g, uint8_t data) {
  //printf("%02x\n", data);
  do_write(g, data >> 4, 1);
  do_write(g, data, 0);
}

void do_fast_write_async(gscomms * g, uint8_t data) {
  //printf("%02x\n", data);
  do_write_async(g, data >> 4, 1);
  do_write_async(g, data, 0);
}

unsigned char ReadWriteNibble(gscomms * g, unsigned char x) {
  int spin[2] = {0,0};
  do_write(g, x & 0xf, 1);

  while ((do_read(g)&0x10) == 0) spin[0]++;

  unsigned char retval = do_read(g)&0xf;

  do_clear(g);

  while ((do_read(g)&0x10) == 0x10) spin[1]++;

  return retval;

}

void WriteNibble(gscomms * g, unsigned char x) {
  if (g->async) {
    do_write_async(g, x & 0xf, 1);
    do_clear_async(g);
  } else {
    do_write(g, x & 0xf, 1);
    do_clear(g);
  }
}


unsigned char ReadWriteByte(gscomms * g, unsigned char b) {
  return (ReadWriteNibble(g, b>>4)<<4)|ReadWriteNibble(g, b);
}

void WriteByte(gscomms * g, unsigned char b) {
  WriteNibble(g, b>>4);
  WriteNibble(g, b);
}
void WriteByteFast(gscomms * g, unsigned char b) {
  if (g->async) {
      do_fast_write_async(g, b);
  } else {
      do_fast_write(g, b);
  }
}

unsigned char ReadByte(gscomms * g) {
  return ReadWriteByte(g, 0);
}

unsigned long ReadWrite32(gscomms * g, unsigned long v) {
  return (((unsigned long)ReadWriteByte(g, v>>24))<<24) |
    (((unsigned long)ReadWriteByte(g, v>>16))<<16) |
    (((unsigned long)ReadWriteByte(g, v>> 8))<< 8) |
    ReadWriteByte(g, v);
}

void Write32(gscomms * g, unsigned long v) {
  WriteByte(g, v>>24);
  WriteByte(g, v>>16);
  WriteByte(g, v>> 8);
  WriteByte(g, v);
}

int InitGSCommsNoisy(gscomms * g, int retries, int noisy) {
  int quiet = !noisy;
  if (!quiet) printf("Sync...\n");

  int got_it = 0;

  for (int i = 0; i < retries; i++) {
    static const struct timespec hundredms = {
      .tv_sec = 0,
      .tv_nsec = 100*1000*1000
    };

    do_write(g, 3, 1);

    nanosleep(&hundredms, NULL);
    unsigned char result = do_read(g)&0xf;
    do_clear(g);
    nanosleep(&hundredms, NULL);

    do_write(g, 3, 1);
    nanosleep(&hundredms, NULL);
    result = (result << 4) | (do_read(g)&0xf);
    do_clear(g);
    nanosleep(&hundredms, NULL);


    if (result == 'g') {
      got_it = 1;
      break;
    }

    if (!quiet) {
      printf("Sync got %02x != %02x, retrying\n", result, 'g');
    }
    // try to get back in sync
    do_write(g, 3, 1);
    nanosleep(&hundredms, NULL);
    do_clear(g);
    nanosleep(&hundredms, NULL);
  }

  if (!got_it) {
    printf("Aborting sync\n");
    do_clear(g);
    return 0;
  }

  return 1; //Handshake(g, quiet);
}

int Handshake(gscomms * g, int quiet) {
  if (!quiet) printf("Handshake...\n");

  if (!(ReadWriteByte(g, 'G') == 'g' && 
        ReadWriteByte(g, 'T') == 't')) {

    fprintf(stderr, "Handshake Failed\n");
    return 0;
  }

  if (!quiet) printf("OK\n");
  return 1;
}

void WriteHandshake(gscomms * g) {
  WriteByte(g, 'G');
  WriteByte(g, 'T');
}

int InitGSComms(gscomms * g, int retries) {
  return InitGSCommsNoisy(g, retries, 0);
}

char * GetGSVersion(gscomms * g) {
  Handshake(g, 1);

  ReadWriteByte(g, 'f');
  ReadByte(g);
  ReadByte(g);
  ReadByte(g);

  unsigned char length = ReadByte(g);
  char * vs = (char*)malloc(length+1);
  for (unsigned char i = 0; i < length; i++) {
    vs[i] = ReadByte(g);
  }
  vs[length] = '\0';

  return vs;
}

unsigned char EndTransaction(gscomms * g, unsigned char checksum) {
  ReadWrite32(g, 0);
  ReadWrite32(g, 0);
  return ReadWriteByte(g, checksum);
}

void ReadRAM(gscomms * g, unsigned char *buf, unsigned long address, unsigned long length) {
  Handshake(g, 1);

  ReadWriteByte(g, 1);
  ReadWrite32(g, address);
  ReadWrite32(g, length);

  for (unsigned long i = 0; i < length; i++) {
    if (buf) {
      buf[i] = ReadByte(g);
    } else {
      ReadByte(g);
    }
  }

  EndTransaction(g, 0);
}

static inline void status_report(gscomms * g, time_t * status_report_time, unsigned long i, unsigned long length) {
  if (!status_report_time || time(NULL) >= *status_report_time) {
    if (g->async) {
      printf("%lu %2lu%% %d\n", i, i*100/length, g->writes_pending);
    } else {
      printf("%lu %2lu%%\n", i, i*100/length);
    }

    if (status_report_time) {
      *status_report_time = time(NULL)+2;
    }
  }
}


void WriteRAM(gscomms * g, const unsigned char *buf, unsigned long address, unsigned long length) {
  WriteRAMStart(g, address, length);

  printf("Uploading to %x\n", (int)address);

  time_t status_report_time = 0;

  for (unsigned long i = 0; i < length; i++) {
    WriteRAMByte(g, buf[i]);

    HandleEvents(g, 0, 256);

    status_report(g, &status_report_time, i, length);
  }

  WriteRAMFinish(g);
}

void WriteRAMfromFile(gscomms * g, FILE * infile, unsigned long address, unsigned long length) {
  if (length == (unsigned long)-1) {
    fseek(infile, 0, SEEK_END);
    length = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    if (length == -1) {
      perror("failed getting file length");
      exit(-1);
    }
  }

  if (length < 1) {
    return;
  }


  WriteRAMStart(g, address, length);

  time_t status_report_time = 0;
  printf("Uploading to %x\n", (int)address);

  for (unsigned long i = 0; i < length; i++) {
    int c = fgetc(infile);

    if (c == EOF) {
      fprintf(stderr, "hit EOF before writing all bytes\n");
      exit(-1);
    }

    WriteRAMByte(g, (unsigned char)c);

    HandleEvents(g, 0, 256);

    status_report(g, &status_report_time, i, length);
  }

  WriteRAMFinish(g);

  status_report(g, NULL, length, length);
}

static void WriteRAMStart(gscomms * g, unsigned long address, unsigned long length) {
  Handshake(g, 1);

  ReadWriteByte(g, 2);
  ReadWrite32(g, address);
  ReadWrite32(g, length);

  if (g->async ) {
    if (g->writes_pending != 0) {
      fprintf(stderr, "%d writes pending when starting a new write\n", g->writes_pending);
      exit(-1);
    }

    //HandleEvents(g, 0, 256);
  }
}

void HandleEvents(gscomms * g, long timeout_ms, int max_pending_writes) {
  if (g->async) {
    if (g->writes_pending > max_pending_writes) {
      struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = timeout_ms*1000
      };
      libusb_handle_events_timeout(g->ctx, &tv);
    }
  }
}

static void WriteRAMByte(gscomms * g, unsigned char b) {
    switch (g->mode) {
      case GSCOMMS_MODE_CAREFUL:
        ReadWriteByte(g, b);
        break;
      case GSCOMMS_MODE_STANDARD:
        WriteByte(g, b);
        break;
      case GSCOMMS_MODE_FAST:
        WriteByteFast(g, b);
        break;
    }
}

static void WriteRAMFinish(gscomms * g) {
  if (g->async) {
    while (g->writes_pending > 0) {
      static const struct timespec hundredms = {
        .tv_sec = 0,
        .tv_nsec = 100*1000*1000
      };

      HandleEvents(g, TIMEOUT, 0);

      if (g->writes_pending > 0) {
        printf("waiting on %d writes\n", g->writes_pending);
      }
    }
  }

  EndTransaction(g, 0);
}

void Disconnect(gscomms * g) {
  Handshake(g, 1);
  ReadWriteByte(g, 'd');
}

///

gscomms * setup_gscomms() {
  int rc;

  gscomms * g = malloc(sizeof(gscomms));

  rc = libusb_init(&g->ctx);
  if (rc != 0) {
    fprintf(stderr, "libusb_init failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }

  g->dev = libusb_open_device_with_vid_pid(g->ctx, VENDOR_ID, PRODUCT_ID);

  if (!g->dev) {
    fprintf(stderr, "failed to locate/open device\n");
    exit(-1);
  }

  rc = libusb_claim_interface(g->dev, 0);
  if (rc != 0) {
    fprintf(stderr, "claim failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }


  const char * speed_string;

  rc = libusb_get_device_speed(libusb_get_device(g->dev));
  switch (rc) {
    case LIBUSB_SPEED_LOW:
      speed_string = "Low (1.5MBit/s)";
      break;
    case LIBUSB_SPEED_FULL:
      speed_string = "Full (12MBit/s)";
      break;
    case LIBUSB_SPEED_HIGH:
      speed_string = "High (480MBit/s)";
      break;
    case LIBUSB_SPEED_SUPER:
      speed_string = "Super (5000MBit/s)";
      break;
    default:
      speed_string = "unknown";
      break;
  }
  printf("Device speed: %s\n", speed_string);

  rc = libusb_get_max_packet_size(libusb_get_device(g->dev), ENDPOINT_MOS_BULK_WRITE);
  printf("Max bulk write packet: %d bytes\n", rc);

  set_mode(g, GSCOMMS_MODE_STANDARD);

  g->async = 1;
  g->writes_pending = 0;

  return g;
}

///

void cleanup_gscomms(gscomms * g) {
  int rc;

  rc = libusb_release_interface(g->dev, 0);
  if (rc != 0) {
    fprintf(stderr, "release failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }

  libusb_exit(g->ctx);
}
