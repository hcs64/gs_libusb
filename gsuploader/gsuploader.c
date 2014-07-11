#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include <arpa/inet.h>  // for htons
#include <unistd.h>

#include "mips.h"
#include "gscomms.h"

int upload_embedded(libusb_device_handle *dev);
int Upload(libusb_device_handle *dev, const unsigned char * buffer, unsigned long size, unsigned long address);
int UploadBulk(libusb_device_handle *dev, const unsigned char * buffer, unsigned long size, unsigned long address);
int run(libusb_device_handle * dev, unsigned long addr);
void patch_FIFO_receive(libusb_device_handle * dev);
void unpatch_FIFO_receive(libusb_device_handle * dev);

#define UPLOAD_ADDR 0xA0000400UL 
#define ENTRYPOINT  0x80000400UL
#define EMBED_ADDR  0xA0300000UL

//#define INSN_PATCH_ADDR 0xA07C5C00UL //GS Code Handler(uncached)
#define INSN_PATCH_ADDR 0xA07C5C00UL //GS Code Handler(uncached)
#define GLOBAL_OFFSET_TABLE 0xA0000200UL //Where to store exported function GOT.

#define GOT_ENTRY_SIZE 16

#define DETACH_GS_GOT_IDX     0
#define FIFO_PATCH_GOT_IDX    1
#define FIFO_RECEIVE_GOT_IDX  2

/* Embedded pre-setup code */
unsigned long codebuf_pre[]=
{
  /* Stop GameShark traps */
  MTC0(MIPS_R0, 18),
  NOP,
  MTC0(MIPS_R0, 19),
  NOP,

  /* Disable Interrupts */
  MFC0(MIPS_T0, 12),
  MIPS_ADDIU(MIPS_T1, MIPS_R0, 0xfffe),
  MIPS_AND(MIPS_T0, MIPS_T0, MIPS_T1),
  MTC0(MIPS_T0, 12),

  /* Modify EPC */
  LUI(MIPS_K0, ENTRYPOINT>>16), 
  ORI(MIPS_K0, MIPS_K0, ENTRYPOINT), 
  NOP,
  MTC0(MIPS_K0, 14),
  NOP,

  /* Patch back modified code handler */
  LUI(MIPS_K1, 0x3c1a),
  ORI(MIPS_K1, MIPS_K1, 0x8000),
  NOP,
  LUI(MIPS_K0, INSN_PATCH_ADDR>>16),
  ORI(MIPS_K0, MIPS_K0, INSN_PATCH_ADDR),
  NOP,
  SW(MIPS_K1, 0, MIPS_K0),
  NOP,
  SYNC,
  NOP,

  /* Halt RSP */
  LUI(MIPS_T1, 2),
  LUI(MIPS_T0, 0xa404),
  ORI(MIPS_T0, MIPS_T0, 0x0010),
  NOP,
  SW(MIPS_T1, 0, MIPS_T0),
  NOP,

  /* Halt RDP */
  LUI(MIPS_T1, 1|4|0x10|0x40|0x80|0x100|0x200),
  LUI(MIPS_T0, 0xa410),
  ORI(MIPS_T0, MIPS_T0, 0x000c),
  NOP,
  SW(MIPS_T1, 0, MIPS_T0),
  NOP,

  /* Return from interrupt - execute code */
  ERET,
  NOP,
};

unsigned long codebuf_patch_fifo[] = {
  /* Patch in FIFO receive */
#if 0
#if 0
  LUI(A1, GLOBAL_OFFSET_TABLE>>16),
  ORI(A1, A1,  GLOBAL_OFFSET_TABLE&0xffff),
  LW(A0, FIFO_RECEIVE_GOT_IDX*GOT_ENTRY_SIZE+8, A1),
#else
  LUI(A0, 0x0c1e),
  ORI(A0, A0, 0x1f57),
#endif
  LUI(A1, 0xA079),
  SW(A0, 0x19B0, A1),

  //CACHE(A1, CACH_PI|C_IINV, 0x19B0),
#endif
  
  /* Patch back modified code handler */
  LUI(MIPS_K1, 0x3c1a),
  ORI(MIPS_K1, MIPS_K1, 0x8000),
  NOP,
  LUI(MIPS_K0, INSN_PATCH_ADDR>>16),
  ORI(MIPS_K0, MIPS_K0, INSN_PATCH_ADDR),
  NOP,
  SW(MIPS_R0, 0, MIPS_K0),
  NOP,
  CACHE(MIPS_K0, CACH_PI|C_IINV, 0),

  /* resume GS execution */
  J(INSN_PATCH_ADDR),
  NOP,
};

unsigned long codebuf_start_gscomms[] = {
  /* Callback function: Start GScomms */
  MFC0(MIPS_T0, 12),
  MIPS_ADDIU(MIPS_T1, MIPS_R0, 0xfffe),
  MIPS_AND(MIPS_T0, MIPS_T0, MIPS_T1),
  MTC0(MIPS_T0, 12),
  LUI(MIPS_V0,0xa000),
  ORI(MIPS_A2,MIPS_V0,0x180),
  LW(MIPS_A0,0,MIPS_A2),
  ORI(MIPS_V1,MIPS_V0,0x120),
  SW(MIPS_A0,0, MIPS_V1),
  ORI(MIPS_A1,MIPS_V0,0x184),
  LW(MIPS_A0,0,MIPS_A1),
  ORI(MIPS_V1,MIPS_V0,0x124),
  SW(MIPS_A0,0,MIPS_V1),
  ORI(MIPS_A0,MIPS_V0,0x188),
  LW(MIPS_A3,0,MIPS_A0),
  ORI(MIPS_V1,MIPS_V0,0x128),
  SW(MIPS_A3,0,MIPS_V1),
  ORI(MIPS_V1,MIPS_V0,0x18c),
  LW(MIPS_A3,0,MIPS_V1),
  ORI(MIPS_V0,MIPS_V0,0x12c),
  SW(MIPS_A3,0,MIPS_V0),
  LUI(MIPS_V0,0x3c1a),
  ORI(MIPS_V0,MIPS_V0,0xa079),
  SW(MIPS_V0,0,MIPS_A2),
  LUI(MIPS_V0,0x275a),
  ORI(MIPS_V0,MIPS_V0,0x4aec),
  SW(	MIPS_V0,0,MIPS_A1),
  LUI(MIPS_V0,0x340),
  ORI(MIPS_V0,MIPS_V0,0x8),
  SW(MIPS_V0,0,MIPS_A0),
  SW(MIPS_R0,0,MIPS_V1),
  LUI(MIPS_T1,0xa000),
  ORI(MIPS_T1,MIPS_T1,0x120),
  CACHE(MIPS_T1, 0x10, 0),
  NOP,
  LUI(MIPS_T1,0xa000),
  ORI(MIPS_T1,MIPS_T1,0x124),
  CACHE(MIPS_T1, 0x10, 0),
  NOP,
  LUI(MIPS_T1,0xa000),
  ORI(MIPS_T1,MIPS_T1,0x128),
  CACHE(MIPS_T1, 0x10, 0),
  NOP,
  LUI(MIPS_T1,0xa000),
  ORI(MIPS_T1,MIPS_T1,0x12c),
  CACHE(MIPS_T1, 0x10, 0),
  NOP,
  LUI(MIPS_T1,0xa000),
  ORI(MIPS_T1,MIPS_T1,0x180),
  CACHE(MIPS_T1, 0x10, 0),
  NOP,
  LUI(MIPS_T1,0xa000),
  ORI(MIPS_T1,MIPS_T1,0x184),
  CACHE(MIPS_T1, 0x10, 0),
  NOP,
  LUI(MIPS_T1,0xa000),
  ORI(MIPS_T1,MIPS_T1,0x188),
  CACHE(MIPS_T1, 0x10, 0),
  NOP,
  LUI(MIPS_T1,0xa000),
  ORI(MIPS_T1,MIPS_T1,0x18c),
  CACHE(MIPS_T1, 0x10, 0),
  MFC0(MIPS_T0, 12),
  ORI(MIPS_T0, MIPS_T0, 1),
  MTC0(MIPS_T0, 12),
  JR(MIPS_RA),    
  NOP,
};

unsigned long codebuf_check_gsbutton[] = {
  /* Callback function: Check GS Button */
  LUI(MIPS_V0, 0xbe40),
  LBU(MIPS_V0, 0, MIPS_V0),
  SRA(MIPS_V0, MIPS_V0, 0x2),
  XORI(MIPS_V0, MIPS_V0, 0x1),
  ANDI(MIPS_V0, MIPS_V0, 0x1),
  JR(MIPS_RA),
  NOP,
};

#if 0
  /* Function: Patch in FIFO receive */
  GAMESHARK_PATCH_FIFO_RECEIVE,
  /* generate JAL instruction */
  LUI(A1, GLOBAL_OFFSET_TABLE>>16),
  ORI(A1, GLOBAL_OFFSET_TABLE&0xffff),
  LW(A1, 4*4, A1),
  LUI(A0, 0x1000),
  ADDIU(A0, A0, -1),
  MIPS_AND(A1, A1, A0),
  SRL(A1, A1, 2),
  LUI(A0, 0x03 << (26-16)),
  OR(A0, A0, A1),

  LUI(A1, 0x8079),
  SW(A0, 0x19B0, A1),
  CACHE(A1, CACH_PI|CACHE_IINV, 0x19B0),

  /* Function: Patch out FIFO receive */
  GAMESHARK_UNPATCH_FIFO_RECEIVE,
  LUI(A0, (0x80787D5C>>(2+16))&0x3fff),
  ORI(A0,  0x80787D5C>>2),
  LUI(A1. 0x8079),
  SW(A0, 0x19B0, A1),
  CACHE(A1, CACH_PI|CACHE_IINV, 0x19B0),
#endif

#define PROMPT_PULSE 50
#define ACK_PULSE 10
//              /ERROR  /BUSY   /ACK
// 0x18 = 0x08    1       0       0
// 0x10 = 0x88    1       1       0
// 0x1C = 0x48    1       0       1
// 0x14 = 0xC8    1       1       1

// PHASE0 = 0x14, PHASE2 = 0x10, PHASE3 = 0x14 works

#define PHASE0 0x14

#define SIMULATE_PROMPT             \
  JAL(0x80787C24),                  \
  ORI(A0, R0, PHASE0)

#define PHASE2 0x10
#define PHASE3 0x1C

#define SIMULATE_ACK                \
  JAL(0x80787C24),                  \
  ORI(A0, R0, PHASE2),              \
  ORI(A0, A0, ACK_PULSE),           \
  BNE(A0, R0, -1*4),                \
  ADDIU(A0, A0, -1),                \
  JAL(0x80787C24),                  \
  ORI(A0, R0, PHASE3)

unsigned long codebuf_fifo_receive[] = {
  /* Function: FIFO receive byte */
  ADDIU(SP, SP, 0xFFE0),
  SW(S0, 0x10, SP),
  SW(S1, 0x14, SP),
  SW(RA, 0x18, SP),
  /* disable interrupts */
  MFC0(S1, 12),
  ADDIU(V0, R0, 0xfffe),
  MIPS_AND(V0, S1, V0),
  MTC0(V0, 12),

  SIMULATE_PROMPT,

  /* wait for high nibble */
  JAL(0x80787C88),
  NOP,
  ANDI(V0, V0, 0x10),
  BEQ(V0, R0, -4*4),
  NOP,

  /* wait again (debounce) */
  JAL(0x80787C88),
  NOP,
  ANDI(A0, V0, 0x10),
  BEQ(A0, R0, -4*4),
  /* collect the nibble */
  ANDI(S0, V0, 0xF),
  SLL(S0, S0, 4),

  SIMULATE_ACK,

  SIMULATE_PROMPT,

   /* wait for low nibble */
  JAL(0x80787C88),
  NOP,
  ANDI(V0, V0, 0x10),
  BNE(V0, R0, -4*4),
  NOP,

  /* wait again (debounce) */
  JAL(0x80787C88),
  NOP,
  ANDI(A0, V0, 0x10),
  BNE(A0, R0, -4*4),
  /* collect the nibble */
  ANDI(V0, V0, 0xF),
  OR(S0, V0, S0),

  SIMULATE_ACK,

  /* load return value */
  OR(V0, S0, R0),
  /* restore saved regs */
  LW(S0, 0x10, SP),
  LW(S1, 0x14, SP),
  LW(RA, 0x18, SP),
  ADDIU(SP, SP, +0x20),
  JR(RA),
  NOP,
};

typedef struct {
  unsigned long * const codebuf;
  const unsigned long size;
  unsigned long ram_address;
  const char * const name;
}  embedded_code;

#define EMBEDDED_ENTRY(_codebuf) {  \
  .codebuf = _codebuf,              \
  .size = sizeof(_codebuf), \
  .name = #_codebuf \
}

embedded_code embedded_codes[] = {
  EMBEDDED_ENTRY(codebuf_pre),            // 0
  EMBEDDED_ENTRY(codebuf_patch_fifo),     // 1
  EMBEDDED_ENTRY(codebuf_fifo_receive),   // 2
//  EMBEDDED_ENTRY(codebuf_start_gscomms),  // 3
//  EMBEDDED_ENTRY(codebuf_check_gsbutton), // 4
};

int main(int argc, char ** argv)
{

  libusb_context * ctx;
  libusb_device_handle * dev;

  printf("\nN64 HomeBrew Loader - ppcasm (Based on HCS GSUpload)\n");
  printf("MCS7705 USB version via libusb\n\n");

  if(argc!=2)
  {
    printf("Wrong Usage:\n(Homebrew Uploader): %s <binary>\n", argv[0]);
    return 1;
  }

  setup_libusb(&ctx, &dev);

  if (!InitGSCommsNoisy(dev, RETRIES, 1)) {
    printf("Init failed\n");
    do_clear(dev);
    return 1;
  }
  //Handshake(dev, 1);

  FILE* infile=fopen(argv[1], "rb");
  if(!infile)
  {
    printf("error opening %s\n", argv[1]);
    do_clear(dev);
    return 1;
  }

  upload_embedded(dev);

#if 1
  printf("Patching in modified loader...\n");
  //run(dev, embedded_codes[FIFO_PATCH_GOT_IDX].ram_address);
  patch_FIFO_receive(dev);

  Disconnect(dev);
  sleep(2); // might take a little bit for the instruction cache to turn over
  InitGSComms(dev, RETRIES);
  printf("Done.\n");

  printf("Ok, now try loading...\n");
#endif

#if 1
  /*Upload binary to specified address.*/

  BulkWriteRAMfromFile(dev, infile, UPLOAD_ADDR, -1);
  fclose(infile);
#endif

  printf("Load finished.\n");

#if 1
  printf("Patching out modified loader...\n");
  InitGSCommsNoisy(dev, RETRIES, 1);
  unpatch_FIFO_receive(dev);
  Disconnect(dev);
  sleep(1);
  InitGSCommsNoisy(dev, RETRIES, 1);
  printf("Done.\n");
#endif

  run(dev, embedded_codes[DETACH_GS_GOT_IDX].ram_address);
  Disconnect(dev);

  do_clear(dev);

  printf("Done.\n");

  cleanup_libusb(ctx, dev);

  return 0;
}

void patch_FIFO_receive(libusb_device_handle * dev) {
  unsigned long addr = embedded_codes[FIFO_RECEIVE_GOT_IDX].ram_address;

  unsigned long jal = JAL(addr);
  unsigned char insn[4];
  write32BE(insn, jal);

  if(Upload(dev, insn, 4, 0xA07919B0))
  {  
    printf("FIFO patch failed...\n");
    do_clear(dev);
    exit(-1);
  }
}

void unpatch_FIFO_receive(libusb_device_handle * dev) {
  unsigned long jal = 0x0c1e1f57;
  unsigned char insn[4];
  write32BE(insn, jal);

  if(UploadBulk(dev, insn, 4, 0xA07919B0))
  {  
    printf("FIFO unpatch failed...\n");
    do_clear(dev);
    exit(-1);
  }
}

int upload_embedded(libusb_device_handle *dev)
{
  printf("\nDbg: Function Callbacks:\n");
  printf("\nGlobal Offset Table base address: %lx\n", GLOBAL_OFFSET_TABLE);
  int i = 0, j = 0;
  unsigned char GOT_buf[GOT_ENTRY_SIZE];
  unsigned long got_addr = GLOBAL_OFFSET_TABLE;
  unsigned long embed_addr = EMBED_ADDR;

  for(i=0;i<sizeof(embedded_codes)/sizeof(embedded_codes[0]);i++)
  {
    embedded_code * ecp = &embedded_codes[i];

    ecp->ram_address = embed_addr;

    /* byteswap */
    for(j=0;j<=ecp->size/sizeof(ecp->codebuf[0]);j++)
    {
      ecp->codebuf[j] = htonl(ecp->codebuf[j]);
    }

    /*Upload embedded code */
    if(Upload(dev, (unsigned char *)ecp->codebuf, ecp->size, embed_addr))
    {
      printf("Failed to upload embedded code %s...\n", ecp->name);
      do_clear(dev);
      return 1;
    }

    printf("Uploaded embedded code %s to: 0x%08lx.\n", ecp->name, embed_addr);


    /* upload GOT entry */

    memset(GOT_buf, 0, sizeof(GOT_buf));
    write32BE(GOT_buf, J(embed_addr));
    write32BE(GOT_buf+8, JAL(embed_addr));

    if(Upload(dev, GOT_buf, GOT_ENTRY_SIZE, got_addr))
    {  
      printf("GOT patch failed...\n");
      return 1;    
    }

    printf("%d: 0x%lx: GOT: 0x%lx\n", i, embed_addr, got_addr);

    embed_addr += ecp->size;
    got_addr += GOT_ENTRY_SIZE;
  }

  printf("\n");
  return 0;
}

int Upload(libusb_device_handle *dev, const unsigned char * buffer, unsigned long size, unsigned long address) {
  unsigned long c=0;

  Handshake(dev, 1);
  ReadWriteByte(dev, 2);
  ReadWrite32(dev, address);
  ReadWrite32(dev, size);

  for (c=0; c < size; c++) ReadWriteByte(dev, buffer[c]);

  EndTransaction(dev, 0);

#if 0
  // verify

  Handshake(dev, 1);
  ReadWriteByte(dev, 1);
  ReadWrite32(dev, address);
  ReadWrite32(dev, size);

  for (c=0; c < size; c++) {
    unsigned char b = ReadByte(dev);
    if (b != buffer[c]) {
      fprintf(stderr, "Verify error at 0x%lx, %02x != %02x\n", address+c, b, buffer[c]);
    }
  }

  EndTransaction(dev, 0);
#endif

  return 0;
}

int UploadBulk(libusb_device_handle *dev, const unsigned char * buffer, unsigned long size, unsigned long address) {
  unsigned long c=0;

  Handshake(dev, 1);
  ReadWriteByte(dev, 2);
  ReadWrite32(dev, address);
  ReadWrite32(dev, size);

  for (c=0; c < size; c++) {
    do_write(dev, buffer[c] >> 4, 1);
    do_write(dev, buffer[c], 0);
  }

  EndTransaction(dev, 0);

  return 0;
}

int run(libusb_device_handle * dev, unsigned long addr) {
  /*Make synthetic jump instruction based on address.*/
  unsigned long instruction=J(addr);

  unsigned char check_sig[4] = {0xff,0xff,0xff,0xff};
  ReadRAM(dev, check_sig, INSN_PATCH_ADDR, 4);
  printf("patching 0x%08lx %02x%02x%02x%02x->%08lx\n", INSN_PATCH_ADDR, check_sig[0], check_sig[1], check_sig[2], check_sig[3], instruction);

  /*Unload jump instruction into byte buffer for easy transfer.*/
  unsigned char insn[4];
  write32BE(insn, instruction);

  /*Inject synthetic jump instruction into code handler to ensure it runs.*/
  if(Upload(dev, insn, 4, INSN_PATCH_ADDR))
  {  
    printf("Instruction patch failed...\n");
    do_clear(dev);
    return 1;    
  }

  return 0;
}
