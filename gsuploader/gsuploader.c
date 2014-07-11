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
int run(libusb_device_handle * dev, unsigned long addr);
void patch_FIFO_receive(libusb_device_handle * dev);

#define UPLOAD_ADDR 0xA0000400UL 
#define ENTRYPOINT  0x80000400UL
#define EMBED_ADDR  0xA0300000UL

#define INSN_PATCH_ADDR 0xA07C5C00UL //GS Code Handler(uncached)

#define GLOBAL_OFFSET_TABLE 0xA0000200UL //Where to store exported function GOT.

#define GOT_ENTRY_SIZE 16

#define DETACH_GS_GOT_IDX     0

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
  EMBEDDED_ENTRY(codebuf_start_gscomms),  // 1
  EMBEDDED_ENTRY(codebuf_check_gsbutton), // 2
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

  FILE* infile=fopen(argv[1], "rb");
  if(!infile)
  {
    printf("error opening %s\n", argv[1]);
    do_clear(dev);
    return 1;
  }

  upload_embedded(dev);

  /*Upload binary to specified address.*/

  WriteRAMfromFile(ctx, dev, infile, UPLOAD_ADDR, -1);
  fclose(infile);

  run(dev, embedded_codes[DETACH_GS_GOT_IDX].ram_address);
  Disconnect(dev);

  do_clear(dev);

  printf("Done.\n");

  cleanup_libusb(ctx, dev);

  return 0;
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
