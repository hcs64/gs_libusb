#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>

#include "codegen.h"
#include "gscomms.h"

int upload_cb(gscomms * g, code_block * cb, unsigned long address);

#define NEON64_MODE 1

#if NEON64_MODE
#define UPLOAD_ADDR 0x80300000UL
#define ENTRYPOINT  0x80300000UL
#else
#define UPLOAD_ADDR 0xA0000400UL
#define ENTRYPOINT  0x80000400UL
#endif

#define EMBED_ADDR  0xA0300000UL-1024

// GS Code Handler(uncached)
#define INSN_PATCH_ADDR 0xA07C5C00UL 

// call to receive byte in GS upload
#define GET_BYTE_PATCH_ADDR 0xA07919B0


// flag: use 2x transfer?
#define USE_FAST_RECEIVE 0

// flag: use bulk 2x transfer?
#define USE_BULK_RECEIVE 1

int main(int argc, char ** argv)
{
  gscomms * g = NULL;
  int two_stage = 0;

  FILE * infile1 = NULL;
  FILE * infile2 = NULL;

  // commandline
  printf("\nN64 HomeBrew Loader - hcs, ppcasm\n");
  printf("MCS7705 USB version via libusb\n\n");


  if (argc == 2)
  {
    two_stage = 0;
  }
  else if (argc == 3)
  {
    two_stage = 1;
  }
  else
  {
    printf("Wrong Usage:\n(Homebrew Uploader): %s <binary>\n", argv[0]);
    printf("(Two Stage Loader): %s <loader> <binary>\n\n", argv[0]);
    return 1;
  }

  // open inputs
  infile1 = fopen(argv[1], "rb");
  if(!infile1)
  {
    printf("error opening %s\n", argv[1]);
    do_clear(g);
    return 1;
  }

  if (two_stage) {
    infile2 = fopen(argv[2], "rb");
    if(!infile2)
    {
      printf("error opening %s\n", argv[2]);
      do_clear(g);
      return 1;
    }
  }

  // get in touch with the GS
  g = setup_gscomms();

  if (!InitGSCommsNoisy(g, RETRIES, 1)) {
    printf("Init failed\n");
    do_clear(g);
    return 1;
  }

  unsigned long setup_addr;
#if USE_FAST_RECEIVE || USE_BULK_RECEIVE
  unsigned long byte_loader_addr;
#endif

  {
    // generate embedded code
    code_block *setup_cb = generate_setup(ENTRYPOINT, INSN_PATCH_ADDR);
#if USE_FAST_RECEIVE
    code_block *receive_cb = generate_2x_receive();
#elif USE_BULK_RECEIVE
    code_block *receive_cb = generate_bulk_receive();
#endif

    // upload embedded code
    unsigned long embed_addr = EMBED_ADDR;

    upload_cb(g, setup_cb, embed_addr);
    setup_addr = embed_addr;
    embed_addr += setup_cb->size;
    embed_addr = (embed_addr + 3)/4*4;

#if USE_FAST_RECEIVE || USE_BULK_RECEIVE
    upload_cb(g, receive_cb, embed_addr);
    byte_loader_addr = embed_addr;
    embed_addr += receive_cb->size;
    embed_addr = (embed_addr + 3)/4*4;
#endif

    free_cb(setup_cb);
#if USE_FAST_RECEIVE || USE_BULK_RECEIVE
    free_cb(receive_cb);
#endif
  }

#if USE_FAST_RECEIVE || USE_BULK_RECEIVE
  {
    code_block * recv_jal_cb = generate_jal(byte_loader_addr, "upload driver patch");

    upload_cb(g, recv_jal_cb, GET_BYTE_PATCH_ADDR);

    free_cb(recv_jal_cb);
  }

#if USE_FAST_RECEIVE
  set_mode(g, GSCOMMS_MODE_FAST);
#elif USE_BULK_RECEIVE
  set_mode(g, GSCOMMS_MODE_BULK);
#endif 

#if 1
  // might take a little bit for the instruction cache to turn over
  Disconnect(g);
  sleep(1); 
  if (!InitGSComms(g, RETRIES)) {
    printf("Init failed\n");
    do_clear(g);
    return 1;
  }
#endif

#endif

  /*Upload binary to specified address.*/

  WriteRAMfromFile(g, infile1, UPLOAD_ADDR, -1);
  fclose(infile1);

  {
    code_block * setup_j_cb = generate_jump(setup_addr, "patch into code list");
    upload_cb(g, setup_j_cb, INSN_PATCH_ADDR);
    free_cb(setup_j_cb);
  }

  Disconnect(g);

  do_clear(g);

  printf("Done.\n");


  if (two_stage) {
    /* let the loader get settled */
    printf("Send second stage in 1 second...\n");
    sleep(1);

    if (!InitGSComms(g, RETRIES)) {
      printf("Init failed\n");
      do_clear(g);
      return 1;
    }

    WriteRAMfromFile(g, infile2, UPLOAD_ADDR, -1);

    fclose(infile2);

    // TODO: find out why this seems necessary, probably some residual stuff from the bulk transfer
    ReadRAM(g, NULL, UPLOAD_ADDR, 4);

    Disconnect(g);
  }

  cleanup_gscomms(g);
  g = NULL;

  return 0;
}

int upload_cb(gscomms * g, code_block * cb, unsigned long address) {
  printf("Send %s\n", cb->name);
  return WriteRAM(g, cb->code, address, cb->size);
}
