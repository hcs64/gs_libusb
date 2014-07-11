#ifndef _GSCOMMS_H
#define _GSCOMMS_H

#include <stdint.h>
#include <libusb-1.0/libusb.h>

static const int RETRIES = 5;

// hardware I/O
uint8_t do_read(libusb_device_handle * dev);
uint8_t do_raw_read(libusb_device_handle * dev);
void do_clear(libusb_device_handle * dev);
void do_write(libusb_device_handle * dev, uint8_t data, int flagged);

// GS low-level operations
int InitGSComms(libusb_device_handle * dev, int retries);
int InitGSCommsNoisy(libusb_device_handle * dev, int retries, int noisy);
int Handshake(libusb_device_handle * dev, int quiet);
void WriteHandshake(libusb_device_handle * dev);
unsigned char ReadWriteNibble(libusb_device_handle * dev, unsigned char x);
void WriteNibble(libusb_device_handle * dev, unsigned char x);
void Disconnect(libusb_device_handle * dev);

// higher...
unsigned char ReadWriteByte(libusb_device_handle * dev, unsigned char b);
void WriteByte(libusb_device_handle * dev, unsigned char b);
unsigned char ReadByte(libusb_device_handle * dev);
unsigned long ReadWrite32(libusb_device_handle * dev, unsigned long v);
void Write32(libusb_device_handle * dev, unsigned long v);
unsigned char EndTransaction(libusb_device_handle * dev, unsigned char checksum);

// basic GS commands
char * GetGSVersion(libusb_device_handle * dev);
void ReadRAM(libusb_device_handle * dev, unsigned char *buf, unsigned long address, unsigned long length);
void WriteRAM(libusb_context * ctx, libusb_device_handle * dev, const unsigned char *buf, unsigned long address, unsigned long length);
void WriteRAMfromFile(libusb_context * ctx, libusb_device_handle * dev, FILE * infile, unsigned long address, unsigned long length);

// do outstanding asynchronous processing for libusb
void HandleEvents(libusb_context * ctx, libusb_device_handle * dev, long timeout_ms);

// experimental, requires the loader to be patched
void FastWriteRAMfromFile(libusb_device_handle * dev, FILE * infile, unsigned long address, unsigned long length);

// overall setup of device
void setup_libusb(libusb_context ** ctx, libusb_device_handle ** dev);
void cleanup_libusb(libusb_context * ctx, libusb_device_handle * dev);

#endif  // _GSCOMMS_H
