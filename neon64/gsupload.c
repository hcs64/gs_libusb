#include <stdio.h>
#include <stdlib.h>

#include "gscomms.h"

// ***********

int main(int argc, char ** argv) { 
  gscomms * g = NULL;

  g = setup_gscomms();

  ///
  if (!InitGSCommsNoisy(g, RETRIES, 1)) {
    fprintf(stderr, "init failed\n");
    exit(-1);
  }

  // check for Neon64 already present
  unsigned char check_sig[4] = {0xff,0xff,0xff,0xff};
  ReadRAM(g, check_sig, 0x80000400UL, 4);

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
    WriteRAM(g, data, 0x80300000, size);
    printf("done!\n");

    free(data);

    unsigned char patch_data[4] = {0x8,0xc,0,0};

    WriteRAM(g, patch_data, 0x80263844UL, 4);

    Disconnect(g);

    printf("hit enter when Neon64 is running\n");
    getchar();

    InitGSComms(g, RETRIES);
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

    WriteRAM(g, data, 0x80300000, size);

    free(data);
  }

  Disconnect(g);
 
  cleanup_gscomms(g);
  g = NULL;

  return 0;
}
