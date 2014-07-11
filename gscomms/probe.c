#include <stdio.h>
#include <stdlib.h>

#include "gscomms.h"

// ***********

int main(int argc, char ** argv) { 
  libusb_context * ctx = NULL;
  libusb_device_handle * dev = NULL;

  setup_libusb(&ctx, &dev);

  ///
  if (!InitGSCommsNoisy(dev, RETRIES, 1)) {
    fprintf(stderr, "init failed\n");
    exit(-1);
  }

  // check for Neon64 already present
  unsigned char check_sig[4] = {0xff,0xff,0xff,0xff};
//  ReadRAM(dev, check_sig, 0x80000404UL, 4);
  //ReadRAM(dev, check_sig, 0xA07919B0UL, 4);
  //ReadRAM(dev, check_sig, 0x80300000UL, 4);
  ReadRAM(dev, check_sig, 0xA0787CD8, 4);
  printf("%02x%02x%02x%02x\n", check_sig[0], check_sig[1], check_sig[2], check_sig[3]);

  Disconnect(dev);
 
  cleanup_libusb(ctx, dev);
  dev = NULL;
  ctx = NULL;

  return 0;
}
