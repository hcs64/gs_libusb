#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

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
static const int MAX_SPIN = 10000;

static void WriteRAMStart(gscomms * g, unsigned long address, unsigned long length);
static void WriteRAMByte(gscomms * g, unsigned char b);
static void WriteRAMFinish(gscomms * g, unsigned char checksum);
// 

static const struct timespec hundredms = {
  .tv_sec = 0,
  .tv_nsec = 100*1000*1000
};

void set_mode(gscomms * g, int mode) {
  switch (mode) {
    case GSCOMMS_MODE_CAREFUL:
    case GSCOMMS_MODE_STANDARD:
    case GSCOMMS_MODE_FAST:
    case GSCOMMS_MODE_BULK:
      break;
    default:
      fprintf(stderr, "mode #%d unsupported\n", mode);
      exit(-1);
      break;
  }

  g->mode = mode;
}

void set_mos_mode(gscomms * g, int mos_mode) {
  // a chance for any ongoing transfers to finish
  // TODO: this should really be using the 7705's status
  //nanosleep(&hundredms, NULL);
  sleep(1);

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
    fprintf(stderr, "async transfer failed: %s\n", libusb_error_name(transfer->status));
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

void do_bulk_write_async_cb(struct libusb_transfer * transfer) {
  gscomms * g = transfer->user_data;

  g->writes_pending--;

  if (g->writes_pending < 0) {
    fprintf(stderr, "bulk writes pending underflow!\n");
    exit(-1);
  }
  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    fprintf(stderr, "async bulk transfer failed: %s (%d)\n", libusb_error_name(transfer->status), transfer->status);
    exit(-1);
  }
}

void do_bulk_write_async(gscomms * g, const uint8_t * data, int length) {

  const int max_len = 32;
  const int transfers_per_byte = 2;
  const int bytes_per_buf = max_len/transfers_per_byte;

  while (length > 0) {
    struct libusb_transfer * transfer = libusb_alloc_transfer(0);

    int todo = bytes_per_buf;
    if (todo > length) {
      todo = length;
    }

    unsigned char * buf = malloc(todo * transfers_per_byte);

    for (int i = 0, j = 0; i < todo; i++, j += transfers_per_byte) {
      buf[j+0] = 0x10 | (data[i] >> 4);
      buf[j+1] = 0x00 | (data[i] & 0xf);
    }

    //printf("transfer call start\n");
    libusb_fill_bulk_transfer(
        transfer,
        g->dev,
        ENDPOINT_MOS_BULK_WRITE,
        buf,
        todo * transfers_per_byte,
        do_bulk_write_async_cb,
        g,
        TIMEOUT);
    //printf("transfer call finished\n");

    transfer->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
    transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
    transfer->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;

    int rc = libusb_submit_transfer(transfer);
    if (rc != 0) {
      fprintf(stderr, "submit async bulk write failed: %s\n", libusb_error_name(rc));
      do_clear(g);
      set_mos_mode(g, MOS_SPP_MODE);
      exit(-1);
    }

    g->writes_pending ++;

    length -= todo;
    data += todo;

  }

}

void clear_bulk_write_async(gscomms * g) {
    unsigned char * buf = malloc(1);
    buf[0] = 0;

    struct libusb_transfer * transfer = libusb_alloc_transfer(0);

    libusb_fill_bulk_transfer(
        transfer,
        g->dev,
        ENDPOINT_MOS_BULK_WRITE,
        buf,
        1,
        do_bulk_write_async_cb,
        g,
        TIMEOUT);
    //printf("transfer call finished\n");

    transfer->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
    transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
    transfer->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;

    int rc = libusb_submit_transfer(transfer);
    if (rc != 0) {
      fprintf(stderr, "submit async bulk clear failed: %s\n", libusb_error_name(rc));
      do_clear(g);
      set_mos_mode(g, MOS_SPP_MODE);
      exit(-1);
    }

    g->writes_pending ++;
}

unsigned char ReadWriteNibble(gscomms * g, unsigned char x) {
  int spin[2] = {0,0};
  do_write(g, x & 0xf, 1);

  //printf("w:%x\n", x&0xf);

  while ((do_read(g)&0x10) == 0) {
    spin[0]++;
    if (spin[0] > MAX_SPIN) {
      printf("spin[0] is stuck on %x\n", do_read(g));
      break;
    }
  }

  unsigned char retval = do_read(g)&0xf;

  do_clear(g);

  while ((do_read(g)&0x10) == 0x10) {
    spin[1]++;
    if (spin[1] > MAX_SPIN) {
      printf("spin[1] is stuck on %x\n", do_read(g));
      break;
    }
  }

  //printf("r:%x\n", retval);

  return retval;

}


void WriteNibble(gscomms * g, unsigned char x) {
  do_write(g, x & 0xf, 1);
  do_clear(g);
}


unsigned char ReadWriteByte(gscomms * g, unsigned char b) {
  unsigned char result = (ReadWriteNibble(g, b>>4)<<4)|ReadWriteNibble(g, b);
  //printf("W:%02x R:%02x\n", b, result);
  return result;
}

void WriteByte(gscomms * g, unsigned char b) {
  WriteNibble(g, b>>4);
  WriteNibble(g, b);
}

void WriteByteFast(gscomms * g, unsigned char b) {
  do_fast_write_async(g, b);
}

unsigned char ReadByte(gscomms * g) {
  return ReadWriteByte(g, 0);
}

unsigned long ReadWrite32(gscomms * g, unsigned long v) {
  unsigned long result = (((unsigned long)ReadWriteByte(g, v>>24))<<24) |
    (((unsigned long)ReadWriteByte(g, v>>16))<<16) |
    (((unsigned long)ReadWriteByte(g, v>> 8))<< 8) |
    ReadWriteByte(g, v);
  //printf("W:%08lx R:%08lx\n",v, result);
  return result;
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

  printf("\n");

  return 1; //Handshake(g, quiet);
}

int Handshake(gscomms * g, int quiet) {
  if (!quiet) printf("Handshake...\n");

  unsigned char hs0 = 0 , hs1 = 0;
#if 0
  int retries = 8;
  while ((hs0 != 'g' || hs1 != 't') && retries > 0) {
    while (hs0 != 'g' && retries > 0) {
      hs0 = ReadWriteByte(g, 'G');
      retries --;
    }
    hs1 = ReadWriteByte(g, 'T');
    retries --;
  }
#else
  hs0 = ReadWriteByte(g, 'G');
  hs1 = ReadWriteByte(g, 'T');
#endif

  if (!(hs0 == 'g' && hs1 == 't')) {
    fprintf(stderr, "Handshake Failed (%02x %02x)\n", hs0, hs1);
    return 0;
  }

  if (!quiet) printf("OK\n");
  return 1;
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

int ReadRAM(gscomms * g, unsigned char *buf, unsigned long address, unsigned long length) {
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

  return 0;
}

static inline void status_report(gscomms * g, time_t * status_report_time, unsigned long i, unsigned long length) {
  if (!status_report_time || time(NULL) >= *status_report_time) {
    if (g->writes_pending > 0) {
      printf("%lu %2lu%% %d\n", i, i*100/length, g->writes_pending);
    } else {
      printf("%lu %2lu%%\n", i, i*100/length);
    }

    if (status_report_time) {
      *status_report_time = time(NULL)+1;
    }
  }
}


int WriteRAM(gscomms * g, const unsigned char *buf, unsigned long address, unsigned long length) {
  WriteRAMStart(g, address, length);

  printf("Uploading %lu to %lx\n", length, address);

  time_t status_report_time = 0;
  unsigned char checksum = 0;

  if (g->mode == GSCOMMS_MODE_BULK) {
    const int maxlen = 256;
    int todo;
    for (unsigned long i = 0; i < length; i += todo) {
      status_report(g, &status_report_time, i, length);

      todo = maxlen;
      if (i + todo > length) {
        todo = length - i;
      }

      for (int j = 0; j < todo; j++) {
        checksum = (checksum + buf[i+j]) & 0xff;
      }

      do_bulk_write_async(g, &buf[i], todo);

      HandleEvents(g, 0, 512);
    }
  } else {
    for (unsigned long i = 0; i < length; i++) {
      status_report(g, &status_report_time, i, length);

      checksum = (checksum + buf[i]) & 0xff;

      WriteRAMByte(g, buf[i]);


      HandleEvents(g, 0, 256);
    }
  }

  WriteRAMFinish(g, checksum);

  status_report(g, NULL, length, length);
  printf("\n");

  return 0;
}

int WriteRAMfromFile(gscomms * g, FILE * infile, unsigned long address, unsigned long length) {
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
    return 1;
  }

  unsigned char checksum = 0;

  WriteRAMStart(g, address, length);

  time_t status_report_time = 0;
  printf("Uploading %lu to %lx\n", length, address);

  if (g->mode == GSCOMMS_MODE_BULK) {
    const int maxlen = 256;
    int todo;
    unsigned char buf[maxlen];
    for (unsigned long i = 0; i < length; i += todo) {
      status_report(g, &status_report_time, i, length);

      todo = maxlen;
      if (todo + i > length) {
        todo = length - i;
      }

      int got = fread(buf, 1, todo, infile);

      if (got != todo) {
        fprintf(stderr, "hit EOF before writing all bytes\n");
        exit(-1);
      }

      for (int j = 0; j < got; j++) {
        checksum = (checksum + buf[j]) & 0xff;
      }

      while (g->writes_pending > 1024) {
        struct timeval tv = { .tv_sec = 0, .tv_usec = 100*1000 };
        libusb_handle_events_timeout(g->ctx, &tv);
      }

      do_bulk_write_async(g, buf, todo);
    }

    while (g->writes_pending > 0) {
      struct timeval tv = { .tv_sec = 0, .tv_usec = 100*1000 };
      libusb_handle_events_timeout(g->ctx, &tv);
    }

  } else {

    for (unsigned long i = 0; i < length; i++) {
      status_report(g, &status_report_time, i, length);

      int c = fgetc(infile);

      if (c == EOF) {
        fprintf(stderr, "hit EOF before writing all bytes\n");
        exit(-1);
      }

      checksum = (checksum + (unsigned char)c) & 0xff;

      WriteRAMByte(g, (unsigned char)c);

      HandleEvents(g, 0, 256);

    }
  }


  WriteRAMFinish(g, checksum);

  status_report(g, NULL, length, length);
  printf("\n");

  return 0;
}

static void WriteRAMStart(gscomms * g, unsigned long address, unsigned long length) {
  Handshake(g, 1);

  ReadWriteByte(g, 2);
  ReadWrite32(g, address);

  if (g->mode == GSCOMMS_MODE_BULK) {
    // bulk mode may throw up flags before we detect clear
    ReadWriteByte(g, length >> 24);
    ReadWriteByte(g, length >> 16);
    ReadWriteByte(g, length >> 8);
    ReadWriteNibble(g, length >> 4);
    WriteNibble(g, length);
    //Write32(g, length);
  } else {
    ReadWrite32(g, length);
  }

  if (g->writes_pending != 0) {
    fprintf(stderr, "Error: %d writes pending when starting a new write\n", g->writes_pending);
    exit(-1);
  }

  if (g->mode == GSCOMMS_MODE_BULK) {
    set_mos_mode(g, MOS_FIFO_MODE);
  }
}

void HandleEvents(gscomms * g, long timeout_ms, int max_pending_writes) {
  if (g->writes_pending > max_pending_writes) {
    struct timeval tv = {
      .tv_sec = 0,
      .tv_usec = timeout_ms*1000
    };
    libusb_handle_events_timeout(g->ctx, &tv);
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
      case GSCOMMS_MODE_BULK:
        do_bulk_write_async(g, &b, 1);
        break;
    }
}

static void WriteRAMFinish(gscomms * g, unsigned char checksum) {
  if (g->writes_pending > 0) {
    printf("waiting on %d writes\n", g->writes_pending);
  }

  if (g->mode == GSCOMMS_MODE_BULK) {
    // make sure the last output is 0
    clear_bulk_write_async(g);
  }

  while (g->writes_pending > 0) {
    nanosleep(&hundredms, NULL);
    HandleEvents(g, TIMEOUT, 0);
  }

  if (g->mode == GSCOMMS_MODE_BULK) {
    set_mos_mode(g, MOS_SPP_MODE);
  }

  unsigned char remote_checksum = EndTransaction(g, 0);
  if (checksum != remote_checksum) {
    printf("Checksum mismatch %02x != %02x\n", checksum, remote_checksum);
  } else {
    printf("Checksum %02x OK\n", checksum);
  }

}

void Disconnect(gscomms * g) {
  for (int i = 0; i < 16; i++) {
    if (Handshake(g, 1)) {
      unsigned char resp = ReadWriteByte(g, 'd');
      printf("OK, Disconnect responded %02x\n", resp);
      return;
    }
    printf("retry disconnect handshake\n");
  }
  fprintf(stderr, "Disconnect failed\n");
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
  set_mos_mode(g, MOS_SPP_MODE);

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
