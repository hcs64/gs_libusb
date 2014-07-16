#ifndef _CODEGEN_H
#define _CODEGEN_H

typedef struct {
  unsigned char * code;
  unsigned long size;
  const char * name;
}  code_block;

code_block * cb_from_dwords( unsigned long * dwords, size_t dword_count, const char * name );
void free_cb( code_block * );

code_block * generate_setup(unsigned long payload_entrypoint, unsigned long patch_addr);
code_block * generate_2x_receive();
code_block * generate_bulk_receive();
code_block * generate_jump(unsigned long dest, const char * name);
code_block * generate_jal(unsigned long dest, const char * name);
#endif
