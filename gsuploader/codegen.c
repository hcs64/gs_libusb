#include <limits.h>
#include <stdlib.h>

#include "codegen.h"
#include "mips.h"

code_block * cb_from_dwords( unsigned long * dwords, size_t dword_count, const char * name) {
  const unsigned long code_size = dword_count * 4;

  code_block * cb = calloc(1, sizeof(code_block));

  cb->code = calloc(code_size, sizeof(unsigned char));
  cb->size = code_size;
  cb->name = name;


  for (int i = 0; i < dword_count; i++) {
    write32BE(cb->code+i*4, dwords[i]);
  }

  return cb;
}

void free_cb( code_block * cb ) {
  if (cb) {
    if (cb->code) {
      free(cb->code);
    }
    free(cb);
  }
}

code_block * generate_jump(unsigned long dest, const char *name) {
  unsigned long jump[] = { J(dest) };
  return cb_from_dwords( jump, 1, name );
}

code_block * generate_jal(unsigned long dest, const char *name) {
  unsigned long jal[] = { JAL(dest) };
  return cb_from_dwords( jal, 1, name );
}

code_block * generate_setup( unsigned long payload_entrypoint, unsigned long patch_addr ) {
  /* Embedded pre-setup code */
  unsigned long codebuf_pre[]=
  {
    /* Stop GameShark traps */
    MTC0(R0, 18),
    NOP,
    MTC0(R0, 19),
    NOP,

    /* Disable Interrupts */
    MFC0(T0, 12),
    ADDIU(T1, R0, 0xfffe),
    AND(T0, T0, T1),
    MTC0(T0, 12),

    /* Modify EPC */
    LA(K0, payload_entrypoint),
    NOP,
    MTC0(K0, 14),
    NOP,

    /* Patch back modified code handler */
    LA(K1, 0x3c1a8000UL),
    NOP,
    LA(K0, patch_addr),
    NOP,
    SW(K1, 0, K0),
    NOP,
    SYNC,
    NOP,

    /* Halt RSP */
    LUI(T1, 2),
    LA(T0, 0xa4040010UL),
    NOP,
    SW(T1, 0, T0),
    NOP,

    /* Halt RDP */
    LUI(T1, 1|4|0x10|0x40|0x80|0x100|0x200),
    LA(T0, 0xa410000cUL),
    NOP,
    SW(T1, 0, T0),
    NOP,

    /* Return from interrupt - execute code */
    ERET,
    NOP,
  };

  return cb_from_dwords( 
    codebuf_pre,
    sizeof(codebuf_pre)/sizeof(codebuf_pre[0]),
    "embedded pre-setup code"
  );
}

#define DEBOUNCE_COUNT 1
#define GS_READ_PORT  0x80787C88

code_block * generate_2x_receive( ) {
  unsigned long codebuf_2x_receive[] = {
    /* Function: fast (2x) receive byte */
    ADDIU(SP, SP, 0xFFD8),
    SW(S0, 0x10, SP),
    SW(S1, 0x14, SP),
    SW(S2, 0x18, SP),
    SW(RA, 0x1C, SP),

    /* wait for consistent high nibble */
    ORI(S2, R0, DEBOUNCE_COUNT),
    JAL(GS_READ_PORT),
    NOP,
    ANDI(A0, V0, 0x10),
    BEQ(A0, R0, -4*4),
    NOP,
    BNE(S2, R0, -6*4),
    ADDIU(S2, S2, -1),

    /* collect the nibble */
    ANDI(S0, V0, 0xF),
    SLL(S0, S0, 4),

    /* wait for consistent low nibble */
    ORI(S2, R0, DEBOUNCE_COUNT),
    JAL(GS_READ_PORT),
    NOP,
    ANDI(A0, V0, 0x10),
    BNE(A0, R0, -4*4),
    NOP,
    BNE(S2, R0, -6*4),
    ADDIU(S2, S2, -1),

    /* collect the nibble */
    ANDI(V0, V0, 0xF),
    OR(S0, V0, S0),

    /* load return value */
    OR(V0, S0, R0),
    /* restore saved regs */
    LW(S0, 0x10, SP),
    LW(S1, 0x14, SP),
    LW(S2, 0x18, SP),
    LW(RA, 0x1C, SP),
    ADDIU(SP, SP, +0x28),
    JR(RA),
    NOP,
  };

  return cb_from_dwords( 
    codebuf_2x_receive,
    sizeof(codebuf_2x_receive)/sizeof(codebuf_2x_receive[0]),
    "2x receive driver"
  );
}

#if 0
#define ACK_PULSE_WIDTH 10
#define GS_WRITE_PORT 0x80787C24

//              nERR  nBUSY nACK
// 0x00 = 0x80  0     1     0
// 0x10 = 0x88  1     1     0
// 0x14 = 0xC8  1     1     1
// 0x18 = 0x08  1     0     0
// 0x1C = 0x48  1     0     1

#define SIMULATE_PROMPT \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x14)  \

#define SIMULATE_ACK \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x10),  \
  ORI(A0, R0, ACK_PULSE_WIDTH), \
  BNE(A0, R0, -1*4),  \
  ADDIU(A0, A0, -1),  \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x14)

#define ACK_AND_CLEANUP \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x10),  \
  ORI(A0, R0, ACK_PULSE_WIDTH), \
  BNE(A0, R0, -1*4),  \
  ADDIU(A0, A0, -1),  \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x04)

code_block * generate_bulk_receive( ) {
  unsigned long codebuf_bulk_receive[] = {
    /* Function: bulk receive byte */
    ADDIU(SP, SP, 0xFFD8),
    SW(S0, 0x10, SP),
    SW(S1, 0x14, SP),
    SW(S2, 0x18, SP),
    SW(RA, 0x1C, SP),

    SIMULATE_PROMPT,

    /* wait for consistent high nibble */
    ORI(S2, R0, DEBOUNCE_COUNT),
    JAL(GS_READ_PORT),
    NOP,
    ANDI(S1, V0, 0x1F),
    ANDI(A0, V0, 0x10),
    BEQ(A0, R0, -6*4),
    NOP,

    JAL(GS_READ_PORT),
    NOP,
    ANDI(A0, V0, 0x1F),
    BNE(A0, S1, -11*4),
    ANDI(V0, V0, 0xF),
    BNE(S2, R0, -6*4),
    ADDIU(S2, S2, -1),

/* collect the nibble */
    SLL(S0, V0, 4),

    SIMULATE_ACK,

    /* wait for consistent low nibble */
    ORI(S2, R0, DEBOUNCE_COUNT),
    JAL(GS_READ_PORT),
    NOP,
    ANDI(S1, V0, 0x1F),
    ANDI(A0, V0, 0x10),
    BNE(A0, R0, -6*4),
    NOP,

    JAL(GS_READ_PORT),
    NOP,
    ANDI(A0, V0, 0x1F),
    BNE(A0, S1, -11*4),
    ANDI(V0, V0, 0xF),
    BNE(S2, R0, -6*4),
    ADDIU(S2, S2, -1),

    /* collect the nibble */
    OR(S0, S0, V0),

    SIMULATE_ACK,

    /* load return value */
    OR(V0, S0, R0),
    /* restore saved regs */
    LW(S0, 0x10, SP),
    LW(S1, 0x14, SP),
    LW(S2, 0x18, SP),
    LW(RA, 0x1C, SP),
    ADDIU(SP, SP, +0x28),
    JR(RA),
    NOP,
  };

  return cb_from_dwords(
    codebuf_bulk_receive,
    sizeof(codebuf_bulk_receive)/sizeof(codebuf_bulk_receive[0]),
    "bulk receive driver"
  );
}
#endif

#if 1
#define ACK_PULSE_WIDTH 2
#define GS_WRITE_PORT 0x80787C24

//              nERR  nBUSY nACK
// 0x00 = 0x80  0     1     0
// 0x10 = 0x88  1     1     0
// 0x14 = 0xC8  1     1     1
// 0x18 = 0x08  1     0     0
// 0x1C = 0x48  1     0     1

#define SIMULATE_PROMPT \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x14)  \

#define SIMULATE_ACK \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x10),  \
  ORI(A0, R0, ACK_PULSE_WIDTH), \
  BNE(A0, R0, -1*4),  \
  ADDIU(A0, A0, -1),  \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x14)

#define ACK_AND_CLEANUP \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x10),  \
  ORI(A0, R0, ACK_PULSE_WIDTH), \
  BNE(A0, R0, -1*4),  \
  ADDIU(A0, A0, -1),  \
  JAL(GS_WRITE_PORT), \
  ORI(A0, R0, 0x00)

code_block * generate_bulk_receive( ) {
  unsigned long codebuf_bulk_receive[] = {
    /* Function: bulk receive byte */
    ADDIU(SP, SP, 0xFFD8),
    SW(S0, 0x10, SP),
    SW(S1, 0x14, SP),
    SW(S2, 0x18, SP),
    SW(RA, 0x1C, SP),

    SIMULATE_PROMPT,

    /* wait for consistent high nibble */
    JAL(GS_READ_PORT),
    NOP,
    ANDI(A0, V0, 0x10),
    BEQ(A0, R0, -4*4),
    NOP,

    JAL(GS_READ_PORT),
    NOP,
    ANDI(A0, V0, 0x10),
    BEQ(A0, R0, -9*4),
    ANDI(V0, V0, 0xF),

    /* collect the nibble */
    SLL(S0, V0, 4),

    SIMULATE_ACK,

    /* wait for consistent low nibble */
    JAL(GS_READ_PORT),
    NOP,
    ANDI(A0, V0, 0x10),
    BNE(A0, R0, -4*4),
    NOP,

    JAL(GS_READ_PORT),
    NOP,
    ANDI(A0, V0, 0x10),
    BNE(A0, R0, -9*4),
    ANDI(V0, V0, 0xF),

    /* collect the nibble */
    OR(S0, S0, V0),

    ACK_AND_CLEANUP,

    /* load return value */
    OR(V0, S0, R0),
    /* restore saved regs */
    LW(S0, 0x10, SP),
    LW(S1, 0x14, SP),
    LW(S2, 0x18, SP),
    LW(RA, 0x1C, SP),
    ADDIU(SP, SP, +0x28),
    JR(RA),
    NOP,
  };
  return cb_from_dwords(
    codebuf_bulk_receive,
    sizeof(codebuf_bulk_receive)/sizeof(codebuf_bulk_receive[0]),
    "bulk receive driver"
  );
}
#endif
