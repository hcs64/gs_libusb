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
  ReadRAM(dev, check_sig, 0x80000400UL, 4);

  if (check_sig[0] != 0x10 || check_sig[1] != 0x00)
  {

    FILE * infile = fopen(argv[1], "rb");
    if (!infile) {
      perror("failed neon64 open");
      return -1;
    }

    fseek(infile, 0, SEEK_END);
    long size = ftell(infile);

    fseek(infile, 0, SEEK_SET);

    unsigned char * data = malloc(size);
    if (size != fread(data, 1, size, infile)) {
      perror("neon64 read failed");
      return -1;
    }
    fclose(infile);

    printf("sending Neon64...\n");
    WriteRAM(ctx, dev, data, 0x80300000, size);
    printf("done!\n");

    free(data);

    unsigned char patch_data[4] = {0x8,0xc,0,0};

    WriteRAM(ctx, dev, patch_data, 0x80263844UL, 4);

    Disconnect(dev);

    printf("hit enter when Neon64 is running\n");
    getchar();

    InitGSComms(dev, RETRIES);
  } else {
    printf("Neon64 already loaded.\n");
  }

  {
    FILE * infile = fopen(argv[2], "rb");
    if (!infile) {
      perror("failed nes open");
      return -1;
    }

    fseek(infile, 0, SEEK_END);
    long size = ftell(infile);

    fseek(infile, 0, SEEK_SET);

    unsigned char * data = malloc(size);
    if (size != fread(data, 1, size, infile)) {
      perror("nes read failed");
      return -1;
    }
    fclose(infile);

    WriteRAM(ctx, dev, data, 0x80300000, size);

    free(data);
  }

  Disconnect(dev);
 
  cleanup_libusb(ctx, dev);
  dev = NULL;
  ctx = NULL;

  return 0;
}
