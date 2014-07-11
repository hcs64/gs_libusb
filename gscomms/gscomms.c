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

static const int TIMEOUT = 1000; // ms

static void WriteRAMStart(libusb_context * ctx, libusb_device_handle * dev, unsigned long address, unsigned long length);
static void WriteRAMByte(libusb_context * ctx, libusb_device_handle * dev, unsigned char b);
static void WriteRAMFinish(libusb_context *ctx, libusb_device_handle * dev);
// 

void set_mode(libusb_device_handle * dev, uint8_t mode) {
  int rc = libusb_control_transfer(
      dev, 
      REQTYPE_WRITE,
      REQ_MOS_WRITE,
      MOS_PORT_BASE | mode,
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
void get_clock(libusb_device_handle * dev) {
  unsigned char data;
  int rc = libusb_control_transfer(
      dev,
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

uint8_t do_read(libusb_device_handle * dev) {
  unsigned char data;

  int rc = libusb_control_transfer(
      dev,
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

  if (data & 0x08) {
    return ((data^0x80)>>4)|0x10;
  }

  return 0;
}

void do_clear(libusb_device_handle * dev) {
  do_write(dev, 0, 0);
}

void do_write(libusb_device_handle * dev, uint8_t data,int flagged) {
  uint8_t flagged_data = (flagged ? 0x10 : 0) | (data & 0xf);

  int rc = libusb_control_transfer(
      dev,
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

int writes_pending = 0;

void do_write_async_cb(struct libusb_transfer * transfer) {
  writes_pending --;
  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    fprintf(stderr, "async transfer failed: %d\n", transfer->status);
    exit(-1);
  }
}

void do_write_async(libusb_device_handle * dev, uint8_t data, int flagged) {
  uint8_t flagged_data = (flagged ? 0x10 : 0) | (data & 0xf);

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
      dev,
      setup_buffer,
      do_write_async_cb,
      NULL,
      TIMEOUT);

  transfer->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
  transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
  transfer->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;

  int rc = libusb_submit_transfer(transfer);
  if (rc != 0) {
    fprintf(stderr, "submit async write failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }
  writes_pending ++;
}

void do_clear_async(libusb_device_handle * dev) {
  do_write_async(dev, 0, 0);
}

// trying to coerce the device into doing a bulk mode transfer
void do_bulk_write(libusb_device_handle *dev, const uint8_t * data, int length) {

  // set into the correct mode
  set_mode(dev, MOS_FIFO_MODE);

  const int max_len = 16;
  uint8_t buf[max_len];
  const int bytes_per_buf = 16/2;

  while (length > 0) {
    int todo = bytes_per_buf;
    if (todo > length) {
      todo = length;
    }

    for (int i = 0, j = 0; i < todo; i++, j+=2) {
      buf[j+0] = 0x10 | (data[i] >> 4);
      buf[j+1] = 0 | (data[i] & 0xf);
    }

    int transferred;

    printf("transfer call start\n");
    int rc = libusb_bulk_transfer(
        dev,
        ENDPOINT_MOS_BULK_WRITE,
        buf,
        todo * 2,
        &transferred,
        10*1000);
    printf("transfer call finished\n");

    if (rc != 0) {
      fprintf(stderr, "bulk write failed: %s\n", libusb_error_name(rc));
      exit(-1);
    }
    if (transferred != todo * 2) {
      fprintf(stderr, "short bulk write %d != %d\n", todo*2, transferred);
      exit(-1);
    }

    length -= transferred/2;
    data += transferred/2;
  }

  set_mode(dev, MOS_SPP_MODE);

}

void do_sim_bulk_write(libusb_device_handle *dev, const uint8_t * data, int length) {
#if 0
  const int max_len = 16;
  uint8_t buf[max_len];
  const int bytes_per_buf = 16/2;
#endif

  while (length > 0) {
#if 1
    ReadWriteNibble(dev, *data>>4);
    ReadWriteNibble(dev, *data);

    length--;
    data++;
#else
    int rc;
    int todo = 1; //bytes_per_buf;
    if (todo > length) {
      todo = length;
    }

    for (int i = 0; i < todo; i++) {

      rc = libusb_control_transfer(
        dev,
        REQTYPE_WRITE,
        REQ_MOS_WRITE,
        MOS_PORT_BASE | 0x10 | (data[i] >> 4),
        MOS_PP_DATA_REG,
        NULL,
        0,
        TIMEOUT);

      if (rc != 0) {
        fprintf(stderr, "write failed: %s\n", libusb_error_name(rc));
        exit(-1);
      }

      rc = libusb_control_transfer(
        dev,
        REQTYPE_WRITE,
        REQ_MOS_WRITE,
        MOS_PORT_BASE | (data[i] & 0xf),
        MOS_PP_DATA_REG,
        NULL,
        0,
        TIMEOUT);

      if (rc != 0) {
        fprintf(stderr, "write failed: %s\n", libusb_error_name(rc));
        exit(-1);
      }
    }

    length -= todo;
    data += todo;
#endif
  }
}

unsigned char ReadWriteNibble(libusb_device_handle * dev, unsigned char x) {
  int spin[2] = {0,0};
  do_write(dev, x & 0xf, 1);

  while ((do_read(dev)&0x10) == 0) spin[0]++;

  unsigned char retval = do_read(dev)&0xf;

  do_clear(dev);

  while ((do_read(dev)&0x10) == 0x10) spin[1]++;

  return retval;

}

void WriteNibble(libusb_device_handle * dev, unsigned char x) {
  //int spin[2] = {0,0};
  do_write_async(dev, x & 0xf, 1);

  //while ((do_read(dev)&0x10) == 0) spin[0]++;

  do_clear_async(dev);

  //while ((do_read(dev)&0x10) == 0x10) spin[1]++;
}


unsigned char ReadWriteByte(libusb_device_handle * dev, unsigned char b) {
  return (ReadWriteNibble(dev, b>>4)<<4)|ReadWriteNibble(dev, b);
}

void WriteByte(libusb_device_handle * dev, unsigned char b) {
  WriteNibble(dev, b>>4);
  WriteNibble(dev, b);
}

unsigned char ReadByte(libusb_device_handle * dev) {
  return ReadWriteByte(dev, 0);
}

unsigned long ReadWrite32(libusb_device_handle * dev, unsigned long v) {
  return (((unsigned long)ReadWriteByte(dev, v>>24))<<24) |
    (((unsigned long)ReadWriteByte(dev, v>>16))<<16) |
    (((unsigned long)ReadWriteByte(dev, v>> 8))<< 8) |
    ReadWriteByte(dev, v);
}

int InitGSCommsNoisy(libusb_device_handle * dev, int retries, int noisy) {
  int quiet = !noisy;
  if (!quiet) printf("Sync...\n");

  int got_it = 0;

  for (int i = 0; i < retries; i++) {
    static const struct timespec tenms = {
      .tv_sec = 0,
      .tv_nsec = 10*1000*1000
    };

    do_write(dev, 3, 1);

    nanosleep(&tenms, NULL);
    unsigned char result = do_read(dev)&0xf;
    do_clear(dev);
    nanosleep(&tenms, NULL);

    do_write(dev, 3, 1);
    nanosleep(&tenms, NULL);
    result = (result << 4) | (do_read(dev)&0xf);
    do_clear(dev);
    nanosleep(&tenms, NULL);


    if (result == 'g') {
      got_it = 1;
      break;
    }

    if (!quiet) {
      printf("Sync got %02x != %02x, retrying\n", result, 'g');
    }
    // try to get back in sync
    do_write(dev, 3, 1);
    nanosleep(&tenms, NULL);
    do_clear(dev);
    nanosleep(&tenms, NULL);
  }

  if (!got_it) {
    printf("Aborting sync\n");
    do_clear(dev);
    return 0;
  }

  return 1; //Handshake(dev, quiet);
}

int Handshake(libusb_device_handle * dev, int quiet) {
  if (!quiet) printf("Handshake...\n");

  if (!(ReadWriteByte(dev, 'G') == 'g' && 
        ReadWriteByte(dev, 'T') == 't')) {

    fprintf(stderr, "Handshake Failed\n");
    return 0;
  }

  if (!quiet) printf("OK\n");
  return 1;
}

int InitGSComms(libusb_device_handle * dev, int retries) {
  //return InitGSCommsNoisy(dev, retries, 1);
  return InitGSCommsNoisy(dev, retries, 0);
}

char * GetGSVersion(libusb_device_handle * dev) {
  Handshake(dev, 1);

  ReadWriteByte(dev, 'f');
  ReadByte(dev);
  ReadByte(dev);
  ReadByte(dev);

  unsigned char length = ReadByte(dev);
  char * vs = (char*)malloc(length+1);
  for (unsigned char i = 0; i < length; i++) {
    vs[i] = ReadByte(dev);
  }
  vs[length] = '\0';

  return vs;
}

unsigned char EndTransaction(libusb_device_handle * dev, unsigned char checksum) {
  ReadWrite32(dev, 0);
  ReadWrite32(dev, 0);
  return ReadWriteByte(dev, checksum);
}

void ReadRAM(libusb_device_handle * dev, unsigned char *buf, unsigned long address, unsigned long length) {
  Handshake(dev, 1);

  ReadWriteByte(dev, 1);
  ReadWrite32(dev, address);
  ReadWrite32(dev, length);

  for (unsigned long i = 0; i < length; i++) {
    if (buf) {
      buf[i] = ReadByte(dev);
    } else {
      ReadByte(dev);
    }
  }

  EndTransaction(dev, 0);
}

void WriteRAM(libusb_context * ctx, libusb_device_handle * dev, const unsigned char *buf, unsigned long address, unsigned long length) {
  WriteRAMStart(ctx, dev, address, length);

  printf("Uploading to %x\n", (int)address);

  for (unsigned long i = 0; i < length; i++) {
    WriteRAMByte(ctx, dev, buf[i]);

    if ((i % 1000) == 0) {
      printf("%lu %2lu%%\n", i, i*100/length);
      HandleEvents(ctx, dev, 0);
    }
  }

  WriteRAMFinish(ctx, dev);
}

void WriteRAMfromFile(libusb_context * ctx, libusb_device_handle * dev, FILE * infile, unsigned long address, unsigned long length) {
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

  WriteRAMStart(ctx, dev, address, length);

  printf("Uploading to %x\n", (int)address);

  for (unsigned long i = 0; i < length; i++) {
    int c = fgetc(infile);

    if (c == EOF) {
      fprintf(stderr, "hit EOF before writing all bytes\n");
      exit(-1);
    }

    WriteRAMByte(ctx, dev, (unsigned char)c);

    if ((i % 1000) == 0) {
      printf("%lu %2lu%%\n", i, i*100/length);
      HandleEvents(ctx, dev, 0);
    }
  }

  WriteRAMFinish(ctx, dev);
}

void BulkWriteRAMfromFile(libusb_device_handle * dev, FILE * infile, unsigned long address, unsigned long length) {
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

  Handshake(dev, 1);

  ReadWriteByte(dev, 2);
  ReadWrite32(dev, address);
  ReadWrite32(dev, length);
 
  printf("Bulk Uploading %lu bytes to %x\n", length, (int)address);

  for (unsigned long i = 0; i < length; ) {
    unsigned char buf[32];

    int todo = 32;
    if (todo > length) {
      todo = length;
    }
 
    int count = fread(buf, 1, todo, infile);

    if (count != todo) {
      fprintf(stderr, "hit EOF before writing all bytes\n");
      exit(-1);
    }

   if (i % 1024 == 0) {
      printf("%lu %2lu%%\n", i, i*100/length);
    }

    do_sim_bulk_write(dev, buf, todo);

    i += todo;
  }

  printf("%lu %2lu%%\n", length, 100ul);

  EndTransaction(dev, 0);
}

static void WriteRAMStart(libusb_context * ctx, libusb_device_handle * dev, unsigned long address, unsigned long length) {
  Handshake(dev, 1);

  ReadWriteByte(dev, 2);
  ReadWrite32(dev, address);
  ReadWrite32(dev, length);

  if (writes_pending != 0) {
    fprintf(stderr, "%d writes are still pending when starting a new write\n", writes_pending);
    exit(-1);
  }

  //HandleEvents(ctx, dev, 0);
}

void HandleEvents(libusb_context * ctx, libusb_device_handle * dev, long timeout_ms) {
  struct timeval tv = {
    .tv_sec = 0,
    .tv_usec = timeout_ms*1000
  };
  libusb_handle_events_timeout(ctx, &tv);
}

static void WriteRAMByte(libusb_context * ctx, libusb_device_handle * dev, unsigned char b) {
    WriteByte(dev, b);
}

static void WriteRAMFinish(libusb_context *ctx, libusb_device_handle * dev) {
  while (writes_pending > 0) {
    HandleEvents(ctx, dev, TIMEOUT);

    if (writes_pending > 0) {
      printf("waiting on %d writes\n", writes_pending);
    }
  }

  EndTransaction(dev, 0);
}

void Disconnect(libusb_device_handle * dev) {
  Handshake(dev, 1);
  ReadWriteByte(dev, 'd');
}

///

void setup_libusb(libusb_context ** ctx, libusb_device_handle ** dev) {
  int rc;

  rc = libusb_init(ctx);
  if (rc != 0) {
    fprintf(stderr, "libusb_init failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }

  *dev = libusb_open_device_with_vid_pid(*ctx, VENDOR_ID, PRODUCT_ID);

  if (!*dev) {
    fprintf(stderr, "failed to locate/open device\n");
    exit(-1);
  }

  rc = libusb_claim_interface(*dev, 0);
  if (rc != 0) {
    fprintf(stderr, "claim failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }

  set_mode(*dev, MOS_SPP_MODE);
}

///

void cleanup_libusb(libusb_context * ctx, libusb_device_handle * dev) {
  int rc;

  rc = libusb_release_interface(dev, 0);
  if (rc != 0) {
    fprintf(stderr, "release failed: %s\n", libusb_error_name(rc));
    exit(-1);
  }

  libusb_exit(ctx);
}
