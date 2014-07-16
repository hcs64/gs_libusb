#ifndef _GSCOMMS_H
#define _GSCOMMS_H

#include <stdint.h>
#include <libusb-1.0/libusb.h>

typedef struct {
  libusb_context * ctx;
  libusb_device_handle * dev;

  int mode;
  int writes_pending;
} gscomms;

enum {
  GSCOMMS_MODE_CAREFUL,
  GSCOMMS_MODE_STANDARD,
  GSCOMMS_MODE_FAST,
  GSCOMMS_MODE_BULK,
};

static const int RETRIES = 5;

// hardware I/O
uint8_t do_read(gscomms * g);
uint8_t do_raw_read(gscomms * g);
void do_clear(gscomms * g);
void do_write(gscomms * g, uint8_t data, int flagged);

// GS low-level operations
void set_mode(gscomms * g, int mode);
int InitGSComms(gscomms * g, int retries);
int InitGSCommsNoisy(gscomms * g, int retries, int noisy);
int Handshake(gscomms * g, int quiet);
void WriteHandshake(gscomms * g);
unsigned char ReadWriteNibble(gscomms * g, unsigned char x);
void WriteNibble(gscomms * g, unsigned char x);
void Disconnect(gscomms * g);

// higher...
unsigned char ReadWriteByte(gscomms * g, unsigned char b);
void WriteByte(gscomms * g, unsigned char b);
unsigned char ReadByte(gscomms * g);
unsigned long ReadWrite32(gscomms * g, unsigned long v);
void Write32(gscomms * g, unsigned long v);
unsigned char EndTransaction(gscomms * g, unsigned char checksum);

// basic GS commands
char * GetGSVersion(gscomms * g);
int ReadRAM(gscomms * g, unsigned char *buf, unsigned long address, unsigned long length);
int WriteRAM(gscomms * g, const unsigned char *buf, unsigned long address, unsigned long length);
int WriteRAMfromFile(gscomms * g, FILE * infile, unsigned long address, unsigned long length);

// do outstanding asynchronous processing for libusb
void HandleEvents(gscomms * g, long timeout_ms, int max_pending_writes);

// overall setup of device
gscomms * setup_gscomms();
void cleanup_gscomms(gscomms * g);

#endif  // _GSCOMMS_H
