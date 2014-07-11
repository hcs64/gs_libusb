/*Spinouts mips macro set*/

/***********************************
* MIPS General Purpose Header File *
***********************************/

#include <stdint.h>
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef char   s8;
typedef short  s16;
typedef long  s32;
typedef long long  s64;
typedef float    f32;
typedef double   f64;


/*
** Segment base addresses and sizes
*/
#define KUBASE      0x00000000
#define KUSIZE      0x80000000
#define K0BASE      0x80000000
#define K0SIZE      0x20000000
#define K1BASE      0xA0000000
#define K1SIZE      0x20000000
#define K2BASE      0xC0000000
#define K2SIZE      0x20000000


/*
** Exception vectors
*/
#define SIZE_EXCVEC 0x80                /* Size of an exc. vec      */
#define UT_VEC      K0BASE              /* utlbmiss vector          */
#define R_VEC       (K1BASE+0x1fc00000) /* reset vector             */
#define XUT_VEC     (K0BASE+0x80)       /* extended address tlbmiss */
#define ECC_VEC     (K0BASE+0x100)      /* Ecc exception vector     */
#define E_VEC       (K0BASE+0x180)      /* Gen. exception vector    */


/*
** Address conversion macros
*/

#define K0_TO_K1(x)     ((x)|0xA0000000)    /* kseg0 to kseg1            */
#define K1_TO_K0(x)     ((x)&0x9FFFFFFF)    /* kseg1 to kseg0            */
#define K0_TO_PHYS(x)   ((x)&0x1FFFFFFF)    /* kseg0 to physical         */
#define K1_TO_PHYS(x)   ((x)&0x1FFFFFFF)    /* kseg1 to physical         */
#define KDM_TO_PHYS(x)  ((x)&0x1FFFFFFF)    /* direct mapped to physical */
#define PHYS_TO_K0(x)   ((x)|0x80000000)    /* physical to kseg0         */
#define PHYS_TO_K1(x)   ((x)|0xA0000000)    /* physical to kseg1         */


/*
** Address predicates
*/
#define IS_KSEG0(x)     ((u32)(x) >= K0BASE && (u32)(x) < K1BASE)
#define IS_KSEG1(x)     ((u32)(x) >= K1BASE && (u32)(x) < K2BASE)
#define IS_KSEGDM(x)    ((u32)(x) >= K0BASE && (u32)(x) < K2BASE)
#define IS_KSEG2(x)     ((u32)(x) >= K2BASE && (u32)(x) < KPTE_SHDUBASE)
#define IS_KPTESEG(x)   ((u32)(x) >= KPTE_SHDUBASE)
#define IS_KUSEG(x)     ((u32)(x) < K0BASE)


/*
** Status register
*/
#define SR_CUMASK   0xF0000000  /* coproc usable bits                 */
#define SR_CU3      0x80000000  /* Coprocessor 3 usable               */
#define SR_CU2      0x40000000  /* Coprocessor 2 usable               */
#define SR_CU1      0x20000000  /* Coprocessor 1 usable               */
#define SR_CU0      0x10000000  /* Coprocessor 0 usable               */
#define SR_RP       0x08000000  /* Reduced power (quarter speed)      */
#define SR_FR       0x04000000  /* MIPS III FP register mode          */
#define SR_RE       0x02000000  /* Reverse endian                     */
#define SR_ITS      0x01000000  /* Instruction trace support          */
#define SR_BEV      0x00400000  /* Use boot exception vectors         */
#define SR_TS       0x00200000  /* TLB shutdown                       */
#define SR_SR       0x00100000  /* Soft reset occured                 */
#define SR_CH       0x00040000  /* Cache hit for last 'cache' op      */
#define SR_CE       0x00020000  /* Create ECC                         */
#define SR_DE       0x00010000  /* ECC of parity does not cause error */


/*
** Interrupt enable bits
*/
#define SR_IMASK    0x0000FF00  /* Interrupt mask */
#define SR_IMASK8   0x00000000  /* mask level 8 */
#define SR_IMASK7   0x00008000  /* mask level 7 */
#define SR_IMASK6   0x0000C000  /* mask level 6 */
#define SR_IMASK5   0x0000E000  /* mask level 5 */
#define SR_IMASK4   0x0000F000  /* mask level 4 */
#define SR_IMASK3   0x0000F800  /* mask level 3 */
#define SR_IMASK2   0x0000FC00  /* mask level 2 */
#define SR_IMASK1   0x0000FE00  /* mask level 1 */
#define SR_IMASK0   0x0000FF00  /* mask level 0 */

#define SR_IBIT8    0x00008000  /* bit level 8 */
#define SR_IBIT7    0x00004000  /* bit level 7 */
#define SR_IBIT6    0x00002000  /* bit level 6 */
#define SR_IBIT5    0x00001000  /* bit level 5 */
#define SR_IBIT4    0x00000800  /* bit level 4 */
#define SR_IBIT3    0x00000400  /* bit level 3 */
#define SR_IBIT2    0x00000200  /* bit level 2 */
#define SR_IBIT1    0x00000100  /* bit level 1 */

#define SR_IMASKSHIFT   8

#define SR_KX       0x00000080  /* extended-addr TLB vec in kernel   */
#define SR_SX       0x00000040  /* xtended-addr TLB vec supervisor   */
#define SR_UX       0x00000020  /* xtended-addr TLB vec in user mode */
#define SR_KSU_MASK 0x00000018  /* mode mask                         */
#define SR_KSU_USR  0x00000010  /* user mode                         */
#define SR_KSU_SUP  0x00000008  /* supervisor mode                   */
#define SR_KSU_KER  0x00000000  /* kernel mode                       */
#define SR_ERL      0x00000004  /* Error level, 1=>cache error       */
#define SR_EXL      0x00000002  /* Exception level, 1=>exception     */
#define SR_IE       0x00000001  /* interrupt enable, 1=>enable       */


/*
** Cause register
*/
#define CAUSE_BD        0x80000000  /* Branch delay slot */
#define CAUSE_CEMASK    0x30000000  /* coprocessor error */
#define CAUSE_CESHIFT   28

/* Interrupt pending bits */
#define CAUSE_IP8   0x00008000  /* External level 8 pending - COMPARE */
#define CAUSE_IP7   0x00004000  /* External level 7 pending - INT4    */
#define CAUSE_IP6   0x00002000  /* External level 6 pending - INT3    */
#define CAUSE_IP5   0x00001000  /* External level 5 pending - INT2    */
#define CAUSE_IP4   0x00000800  /* External level 4 pending - INT1    */
#define CAUSE_IP3   0x00000400  /* External level 3 pending - INT0    */
#define CAUSE_SW2   0x00000200  /* Software level 2 pending           */
#define CAUSE_SW1   0x00000100  /* Software level 1 pending           */

#define CAUSE_IPSHIFT   8
#define CAUSE_IPMASK    0x0000FF00  /* Pending interrupt mask */

#define CAUSE_EXCSHIFT  2
#define CAUSE_EXCMASK   0x0000007C  /* Cause code bits */

/* Cause register exception codes */
#define EXC_CODE(x) ((x)<<2)

/* Hardware exception codes */
#define EXC_INT     EXC_CODE(0)     /* interrupt                      */
#define EXC_MOD     EXC_CODE(1)     /* TLB mod                        */
#define EXC_RMISS   EXC_CODE(2)     /* Read TLB Miss                  */
#define EXC_WMISS   EXC_CODE(3)     /* Write TLB Miss                 */
#define EXC_RADE    EXC_CODE(4)     /* Read Address Error             */
#define EXC_WADE    EXC_CODE(5)     /* Write Address Error            */
#define EXC_IBE     EXC_CODE(6)     /* Instruction Bus Error          */
#define EXC_DBE     EXC_CODE(7)     /* Data Bus Error                 */
#define EXC_SYSCALL EXC_CODE(8)     /* SYSCALL                        */
#define EXC_BREAK   EXC_CODE(9)     /* BREAKpoint                     */
#define EXC_II      EXC_CODE(10)    /* Illegal Instruction            */
#define EXC_CPU     EXC_CODE(11)    /* CoProcessor Unusable           */
#define EXC_OV      EXC_CODE(12)    /* OVerflow                       */
#define EXC_TRAP    EXC_CODE(13)    /* Trap exception                 */
#define EXC_VCEI    EXC_CODE(14)    /* Virt. Coherency on Inst. fetch */
#define EXC_FPE     EXC_CODE(15)    /* Floating Point Exception       */
#define EXC_WATCH   EXC_CODE(23)    /* Watchpoint reference           */
#define EXC_VCED    EXC_CODE(31)    /* Virt. Coherency on data read   */

/* C0_PRID Defines */
#define C0_IMPMASK  0xff00
#define C0_IMPSHIFT 8
#define C0_REVMASK  0xff
#define C0_MAJREVMASK   0xf0
#define C0_MAJREVSHIFT  4
#define C0_MINREVMASK   0xf


/*
** Coprocessor 0 operations
*/
#define C0_READI  0x1       /* read ITLB entry addressed by C0_INDEX   */
#define C0_WRITEI 0x2       /* write ITLB entry addressed by C0_INDEX  */
#define C0_WRITER 0x6       /* write ITLB entry addressed by C0_RAND   */
#define C0_PROBE  0x8       /* probe for ITLB entry addressed by TLBHI */
#define C0_RFE    0x10      /* restore for exception                   */


/*
** CACHE instruction
*/

/* Target cache */
#define CACH_PI     0x0 /* specifies primary inst. cache */
#define CACH_PD     0x1 /* primary data cache            */
#define CACH_SI     0x2 /* secondary instruction cache   */
#define CACH_SD     0x3 /* secondary data cache          */

/* Cache operations */
#define C_IINV      0x0     /* index invalidate (inst, 2nd inst) */
#define C_IWBINV    0x0     /* index writeback inval (d, sd)     */
#define C_ILT       0x4     /* index load tag (all)              */
#define C_IST       0x8     /* index store tag (all)             */
#define C_CDX       0xC     /* create dirty exclusive (d, sd)    */
#define C_HINV      0x10    /* hit invalidate (all)              */
#define C_HWBINV    0x14    /* hit writeback inv. (d, sd)        */
#define C_FILL      0x14    /* fill (i)                          */
#define C_HWB       0x18    /* hit writeback (i, d, sd)          */
#define C_HSV       0x1C    /* hit set virt. (si, sd)            */


/*
** CACHE size definitions
*/
#define ICACHE_SIZE         0x4000      /* 16K     */
#define ICACHE_LINESIZE     32          /* 8 words */
#define ICACHE_LINEMASK     (ICACHE_LINESIZE-1)

#define DCACHE_SIZE         0x2000      /* 8K      */
#define DCACHE_LINESIZE     16          /* 4 words */
#define DCACHE_LINEMASK     (DCACHE_LINESIZE-1)


/*
** C0_CONFIG register definitions
*/
#define CONFIG_CM       0x80000000  /* 1 == Master-Checker enabled */
#define CONFIG_EC       0x70000000  /* System Clock ratio          */
#define CONFIG_EC_1_1   0x6         /* System Clock ratio 1 :1     */
#define CONFIG_EC_3_2   0x7         /* System Clock ratio 1.5 :1   */
#define CONFIG_EC_2_1   0x0         /* System Clock ratio 2 :1     */
#define CONFIG_EC_3_1   0x1         /* System Clock ratio 3 :1     */
#define CONFIG_EP       0x0f000000  /* Transmit Data Pattern       */
#define CONFIG_SB       0x00c00000  /* Secondary cache block size  */

#define CONFIG_SS   0x00200000  /* Split scache: 0 == I&D combined     */
#define CONFIG_SW   0x00100000  /* scache port: 0==128, 1==64          */
#define CONFIG_EW   0x000c0000  /* System Port width: 0==64, 1==32     */
#define CONFIG_SC   0x00020000  /* 0 -> 2nd cache present              */
#define CONFIG_SM   0x00010000  /* 0 -> Dirty Shared Coherency enabled */
#define CONFIG_BE   0x00008000  /* Endian-ness: 1 --> BE               */
#define CONFIG_EM   0x00004000  /* 1 -> ECC mode, 0 -> parity          */
#define CONFIG_EB   0x00002000  /* Block order:1->sequent,0->subblock  */

#define CONFIG_IC   0x00000e00  /* Primary Icache size         */
#define CONFIG_DC   0x000001c0  /* Primary Dcache size         */
#define CONFIG_IB   0x00000020  /* Icache block size           */
#define CONFIG_DB   0x00000010  /* Dcache block size           */
#define CONFIG_CU   0x00000008  /* Update on Store-conditional */
#define CONFIG_K0   0x00000007  /* K0SEG Coherency algorithm   */

#define CONFIG_UNCACHED     0x00000002  /* K0 is uncached             */
#define CONFIG_NONCOHRNT    0x00000003
#define CONFIG_COHRNT_EXLWR 0x00000005
#define CONFIG_SB_SHFT      22          /* shift SB to bit position 0 */
#define CONFIG_IC_SHFT      9           /* shift IC to bit position 0 */
#define CONFIG_DC_SHFT      6           /* shift DC to bit position 0 */
#define CONFIG_BE_SHFT      15          /* shift BE to bit position 0 */


/*
** C0_TAGLO definitions for setting/getting cache states and physaddr bits
*/
#define SADDRMASK   0xFFFFE000  /* 31..13 -> scache paddr bits 35..17 */
#define SVINDEXMASK 0x00000380  /* 9..7: prim virt index bits 14..12 */
#define SSTATEMASK  0x00001c00  /* bits 12..10 hold scache line state */
#define SINVALID    0x00000000  /* invalid --> 000 == state 0 */
#define SCLEANEXCL  0x00001000  /* clean exclusive --> 100 == state 4 */
#define SDIRTYEXCL  0x00001400  /* dirty exclusive --> 101 == state 5 */
#define SECC_MASK   0x0000007f  /* low 7 bits are ecc for the tag */
#define SADDR_SHIFT 4           /* shift STagLo (31..13) to 35..17 */

#define PADDRMASK   0xFFFFFF00  /* PTagLo31..8->prim paddr bits35..12   */
#define PADDR_SHIFT 4           /* roll bits 35..12 down to 31..8       */
#define PSTATEMASK  0x00C0      /* bits 7..6 hold primary line state    */
#define PINVALID    0x0000      /* invalid --> 000 == state 0           */
#define PCLEANEXCL  0x0080      /* clean exclusive --> 10 == state 2    */
#define PDIRTYEXCL  0x00C0      /* dirty exclusive --> 11 == state 3    */
#define PPARITY_MASK    0x0001  /* low bit is parity bit (even).        */


/*
** C0_CACHE_ERR definitions.
*/
#define CACHERR_ER          0x80000000  /* 0: inst ref, 1: data ref     */
#define CACHERR_EC          0x40000000  /* 0: primary, 1: secondary     */
#define CACHERR_ED          0x20000000  /* 1: data error                */
#define CACHERR_ET          0x10000000  /* 1: tag error                 */
#define CACHERR_ES          0x08000000  /* 1: external ref, e.g. snoop  */
#define CACHERR_EE          0x04000000  /* error on SysAD bus           */
#define CACHERR_EB          0x02000000  /* complicated, see spec.       */
#define CACHERR_EI          0x01000000  /* complicated, see spec.       */
#define CACHERR_SIDX_MASK   0x003ffff8  /* secondary cache index        */
#define CACHERR_PIDX_MASK   0x00000007  /* primary cache index          */
#define CACHERR_PIDX_SHIFT  12          /* bits 2..0 are paddr14..12    */


/*
** Memory breakpoints (WatchHI/WatchLO)
*/
#define WATCHLO_WTRAP           0x00000001
#define WATCHLO_RTRAP           0x00000002
#define WATCHLO_ADDRMASK        0xfffffff8
#define WATCHLO_VALIDMASK       0xfffffffb
#define WATCHHI_VALIDMASK       0x0000000f

/*
** Coprocessor 0 registers
*/

#define C0_INX          0
#define C0_RAND         1
#define C0_ENTRYLO0     2
#define C0_ENTRYLO1     3
#define C0_CONTEXT      4
#define C0_PAGEMASK     5       /* page mask                        */
#define C0_WIRED        6       /* # wired entries in tlb           */
#define C0_BADVADDR     8
#define C0_COUNT        9       /* free-running counter             */
#define C0_ENTRYHI      10
#define C0_SR           12
#define C0_CAUSE        13
#define C0_EPC          14
#define C0_PRID         15      /* revision identifier              */
#define C0_COMPARE      11      /* counter comparison reg.          */
#define C0_CONFIG       16      /* hardware configuration           */
#define C0_LLADDR       17      /* load linked address              */
#define C0_WATCHLO      18      /* watchpoint                       */
#define C0_WATCHHI      19      /* watchpoint                       */
#define C0_ECC          26      /* S-cache ECC and primary parity   */
#define C0_CACHE_ERR    27      /* cache error status               */
#define C0_TAGLO        28      /* cache operations                 */
#define C0_TAGHI        29      /* cache operations                 */
#define C0_ERROR_EPC    30      /* ECC error prg. counter           */

/*
** Floating point status register
*/
#define FPCSR_FS        0x01000000  /* flush denorm to zero             */
#define FPCSR_C         0x00800000  /* condition bit                    */  
#define FPCSR_CE        0x00020000  /* cause: unimplemented operation   */
#define FPCSR_CV        0x00010000  /* cause: invalid operation         */
#define FPCSR_CZ        0x00008000  /* cause: division by zero          */
#define FPCSR_CO        0x00004000  /* cause: overflow                  */
#define FPCSR_CU        0x00002000  /* cause: underflow                 */
#define FPCSR_CI        0x00001000  /* cause: inexact operation         */
#define FPCSR_EV        0x00000800  /* enable: invalid operation        */
#define FPCSR_EZ        0x00000400  /* enable: division by zero         */
#define FPCSR_EO        0x00000200  /* enable: overflow                 */
#define FPCSR_EU        0x00000100  /* enable: underflow                */
#define FPCSR_EI        0x00000080  /* enable: inexact operation        */
#define FPCSR_FV        0x00000040  /* flag: invalid operation          */
#define FPCSR_FZ        0x00000020  /* flag: division by zero           */
#define FPCSR_FO        0x00000010  /* flag: overflow                   */
#define FPCSR_FU        0x00000008  /* flag: underflow                  */
#define FPCSR_FI        0x00000004  /* flag: inexact operation          */
#define FPCSR_RM_MASK   0x00000003  /* rounding mode mask               */
#define FPCSR_RM_RN     0x00000000  /* round to nearest                 */
#define FPCSR_RM_RZ     0x00000001  /* round to zero                    */
#define FPCSR_RM_RP     0x00000002  /* round to positive infinity       */
#define FPCSR_RM_RM     0x00000003  /* round to negative infinity       */

/**
|**
*\* MIPS64 assembly in macros 
**|
**/

/**
 * If you don't want the instruction names to have a prefix infront of them,
 * compile with the flag -D__MIPS_NO_PREFIX and there will be register names
 * and instruction names compiled without the MIPS_ prefix.
**/

/* Quick... */
#define OP(x)   ((x)<<26)
#define OF(x)   (((uint32_t)(x)>>2)&0xFFFF)
#define SA(x)   (((x)&0x1F)<<6)
#define RD(x)   (((x)&0x1F)<<11)
#define RT(x)   (((x)&0x1F)<<16)
#define RS(x)   (((x)&0x1F)<<21)
#define IM(x)   ((uint32_t)(x)&0xFFFF)
#define JT(x)   (((uint32_t)(x)>>2)&0x3FFFFFF)

/* MIPS Registers */
enum
{
    MIPS_R0,
    MIPS_AT,
    MIPS_V0,
    MIPS_V1,
    MIPS_A0,
    MIPS_A1,
    MIPS_A2,
    MIPS_A3,
    MIPS_T0,
    MIPS_T1,
    MIPS_T2,
    MIPS_T3,
    MIPS_T4,
    MIPS_T5,
    MIPS_T6,
    MIPS_T7,
    MIPS_S0,
    MIPS_S1,
    MIPS_S2,
    MIPS_S3,
    MIPS_S4,
    MIPS_S5,
    MIPS_S6,
    MIPS_S7,
    MIPS_T8,
    MIPS_T9,
    MIPS_K0,
    MIPS_K1,
    MIPS_GP,
    MIPS_SP,
    MIPS_FP,
    MIPS_RA
};

/* MIPS assembly */
#define MIPS_ADD(rd, rs, rt)        (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x20)
#define MIPS_ADDI(rt, rs, immd)     (OP(0x08) | RS(rs) | RT(rt) | IM(immd))
#define MIPS_ADDIU(rt, rs, immd)    (OP(0x09) | RS(rs) | RT(rt) | IM(immd))
#define MIPS_AND(rd, rs, rt)        (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x24)
#define MIPS_ANDI(rt, rs, immd)     (OP(0x0C) | RS(rs) | RT(rt) | IM(immd))
#define MIPS_BC1F(off)              (OP(0x11) | RS(0x08) | OF(off))
#define MIPS_BC1FL(off)             (OP(0x11) | RS(0x08) | RT(0x02) | OF(off))
#define MIPS_BC1T(off)              (OP(0x11) | RS(0x08) | RT(0x01) | OF(off))
#define MIPS_BC1TL(off)             (OP(0x11) | RS(0x08) | RT(0x03) | OF(off))
#define MIPS_BEQ(rs, rt, off)       (OP(0x04) | RS(rs) | RT(rt) | OF(off))
#define MIPS_BEQL(rs, rt, off)      (OP(0x14) | RS(rs) | RT(rt) | OF(off))
#define MIPS_BGEZ(rs, off)          (OP(0x01) | RS(rs) | RT(0x01) | OF(off))
#define MIPS_BGEZAL(rs, off)        (OP(0x01) | RS(rs) | RT(0x11) | OF(off))
#define MIPS_BGEZALL(rs, off)       (OP(0x01) | RS(rs) | RT(0x13) | OF(off))
#define MIPS_BGEZL(rs, off)         (OP(0x01) | RS(rs) | RT(0x03) | OF(off))
#define MIPS_BGTZ(rs, off)          (OP(0x07) | RS(rs) | OF(off))
#define MIPS_BGTZL(rs, off)         (OP(0x17) | RS(rs) | OF(off))
#define MIPS_BLEZ(rs, off)          (OP(0x06) | RS(rs) | OF(off))
#define MIPS_BLEZL(rs, off)         (OP(0x16) | RS(rs) | OF(off))
#define MIPS_BLTZ(rs, off)          (OP(0x01) | RS(rs) | OF(off))
#define MIPS_BLTZAL(rs, off)        (OP(0x01) | RS(rs) | RT(0x10) | OF(off))
#define MIPS_BLTZALL(rs, off)       (OP(0x01) | RS(rs) | RT(0x12) | OF(off))
#define MIPS_BLTZL(rs, off)         (OP(0x01) | RS(rs) | RT(0x02) | OF(off))
#define MIPS_BNE(rs, rt, off)       (OP(0x05) | RS(rs) | RT(rt) | OF(off))
#define MIPS_BNEL(rs, rt, off)      (OP(0x15) | RS(rs) | RT(rt) | OF(off))
#define MIPS_BREAK(code)            ((code) << 6 | 0x0D)
#define MIPS_CACHE(base, op, off)   (OP(0x2F) | RS(base) | RT(op) | OF(off))
#define MIPS_CFC1(rt, rd)           (OP(0x11) | RS(0x02) | RT(base) | RD(rd))
#define MIPS_COP1(cofun)            (OP(0x11) | (1 << 25) | ((cofun) & 0x1FFFFFF))
#define MIPS_CTC1(rt, rd)           (OP(0x11) | RS(0x06) | RT(base) | RD(rd))
#define MIPS_DADD(rd, rs, rt)       (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x2C)
#define MIPS_DADDI(rt, rs, immd)    (OP(0x18) | RS(rs) | RT(rt) | IM(immd))
#define MIPS_DADDIU(rt, rs, immd)   (OP(0x19) | RS(rs) | RT(rt) | IM(immd))
#define MIPS_DADDU(rd, rs, rt)      (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x2D)
#define MIPS_DDIV(rs, rt)           (OP(0x00) | RS(rs) | RT(rt) | 0x1E)
#define MIPS_DDIVU(rs, rt)          (OP(0x00) | RS(rs) | RT(rt) | 0x1F)
#define MIPS_DIV(rs, rt)            (OP(0x00) | RS(rs) | RT(rt) | 0x1A)
#define MIPS_DIVU(rs, rt)           (OP(0x00) | RS(rs) | RT(rt) | 0x1B)
#define MIPS_DMFC0(rt, rd)          (OP(0x10) | RS(0x01) | RT(rt) | RD(rd))
#define MIPS_DMTC0(rt, rd)          (OP(0x10) | RS(0x05) | RT(rt) | RD(rd))
#define MIPS_DMULT(rs, rt)          (OP(0x00) | RS(rs) | RT(rt) | 0x1C)
#define MIPS_DMULTU(rs, rt)         (OP(0x00) | RS(rs) | RT(rt) | 0x1D)
#define MIPS_DSLL(rd, rt, sa)       (OP(0x00) | RT(rt) | RD(rd) | SA(sa) | 0x38)
#define MIPS_DSLLV(rd, rt, rs)      (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x14)
#define MIPS_DSLL32(rd, rt, sa)     (OP(0x00) | RT(rt) | RD(rd) | SA(sa) | 0x3C)
#define MIPS_DSRA(rd, rt, sa)       (OP(0x00) | RT(rt) | RD(rd) | SA(sa) | 0x3B)
#define MIPS_DSRAV(rd, rt, rs)      (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x17)
#define MIPS_DSRA32(rd, rt, sa)     (OP(0x00) | RT(rt) | RD(rd) | SA(sa) | 0x3F)
#define MIPS_DSRL(rd, rt, sa)       (OP(0x00) | RT(rt) | RD(rd) | SA(sa) | 0x3A)
#define MIPS_DSRLV(rd, rt, rs)      (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x16)
#define MIPS_DSRL32(rd, rt, sa)     (OP(0x00) | RT(rt) | RD(rd) | SA(sa) | 0x3E)
#define MIPS_DSUB(rd, rs, rt)       (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x2E)
#define MIPS_DSUBU(rd, rs, rt)      (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x2F)
#define MIPS_J(target)              (OP(0x02) | JT(target))
#define MIPS_JAL(target)            (OP(0x03) | JT(target))
#define MIPS_JALR(rd, rs)           (OP(0x00) | RS(rs) | RD(rd) | 0x09)
#define MIPS_JR(rs)                 (OP(0x00) | RS(rs) | 0x08)
#define MIPS_LB(rt, off, base)      (OP(0x20) | RS(base) | RT(rt) | IM(off))
#define MIPS_LBU(rt, off, base)     (OP(0x24) | RS(base) | RT(rt) | IM(off))
#define MIPS_LD(rt, off, base)      (OP(0x37) | RS(base) | RT(rt) | IM(off))
#define MIPS_LDC1(rt, off, base)    (OP(0x35) | RS(base) | RT(rt) | IM(off))
#define MIPS_LDL(rt, off, base)     (OP(0x1A) | RS(base) | RT(rt) | IM(off))
#define MIPS_LDR(rt, off, base)     (OP(0x1B) | RS(base) | RT(rt) | IM(off))
#define MIPS_LH(rt, off, base)      (OP(0x21) | RS(base) | RT(rt) | IM(off))
#define MIPS_LHU(rt, off, base)     (OP(0x25) | RS(base) | RT(rt) | IM(off))
#define MIPS_LL(rt, off, base)      (OP(0x30) | RS(base) | RT(rt) | IM(off))
#define MIPS_LLD(rt, off, base)     (OP(0x34) | RS(base) | RT(rt) | IM(off))
#define MIPS_LUI(rt, immd)          (OP(0x0F) | RT(rt) | IM(immd))
#define MIPS_LW(rt, off, base)      (OP(0x23) | RS(base) | RT(rt) | IM(off))
#define MIPS_LWC1(rt, off, base)    (OP(0x31) | RS(base) | RT(rt) | IM(off))
#define MIPS_LWL(rt, off, base)     (OP(0x22) | RS(base) | RT(rt) | IM(off))
#define MIPS_LWR(rt, off, base)     (OP(0x26) | RS(base) | RT(rt) | IM(off))
#define MIPS_LWU(rt, off, base)     (OP(0x27) | RS(base) | RT(rt) | IM(off))
#define MIPS_MFC0(rt, rd)           (OP(0x10) | RT(rt) | RD(rd))
#define MIPS_MFC1(rt, rd)           (OP(0x11) | RT(rt) | RD(rd))
#define MIPS_MFHI(rd)               (OP(0x00) | RD(rd) | 0x10)
#define MIPS_MFLO(rd)               (OP(0x00) | RD(rd) | 0x12)
#define MIPS_MTC0(rt, rd)           (OP(0x10) | RS(0x04) | RT(rt) | RD(rd))
#define MIPS_MTC1(rt, rd)           (OP(0x11) | RS(0x04) | RT(rt) | RD(rd))
#define MIPS_MTHI(rd)               (OP(0x00) | RD(rd) | 0x11)
#define MIPS_MTLO(rd)               (OP(0x00) | RD(rd) | 0x13)
#define MIPS_MULT(rs, rt)           (OP(0x00) | RS(rs) | RT(rt) | 0x18)
#define MIPS_MULTU(rs, rt)          (OP(0x00) | RS(rs) | RT(rt) | 0x19)
#define MIPS_NOR(rd, rs, rt)        (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x27)
#define MIPS_OR(rd, rs, rt)         (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x25)
#define MIPS_ORI(rt, rs, immd)      (OP(0x0D) | RS(rs) | RT(rt) | IM(immd))
#define MIPS_SB(rt, off, base)      (OP(0x28) | RS(base) | RT(rt) | IM(off))
#define MIPS_SC(rt, off, base)      (OP(0x38) | RS(base) | RT(rt) | IM(off))
#define MIPS_SCD(rt, off, base)     (OP(0x3C) | RS(base) | RT(rt) | IM(off))
#define MIPS_SD(rt, off, base)      (OP(0x3F) | RS(base) | RT(rt) | IM(off))
#define MIPS_SDC1(rt, off, base)    (OP(0x3D) | RS(base) | RT(rt) | IM(off))
#define MIPS_SDL(rt, off, base)     (OP(0x2C) | RS(base) | RT(rt) | IM(off))
#define MIPS_SDR(rt, off, base)     (OP(0x2D) | RS(base) | RT(rt) | IM(off))
#define MIPS_SH(rt, off, base)      (OP(0x29) | RS(base) | RT(rt) | IM(off))
#define MIPS_SLL(rd, rt, sa)        (OP(0x00) | RT(rt) | RD(rd) | SA(sa))
#define MIPS_SLLV(rd, rt, rs)       (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x04)
#define MIPS_SLT(rd, rs, rt)        (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x2A)
#define MIPS_SLTI(rt, rs, immd)     (OP(0x0A) | RS(rs) | RT(rt) | IM(immd))
#define MIPS_SLTIU(rt, rs, immd)    (OP(0x0B) | RS(rs) | RT(rt) | IM(immd))
#define MIPS_SLTU(rd, rs, rt)       (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x2B)
#define MIPS_SRA(rd, rt, sa)        (OP(0x00) | RT(rt) | RD(rd) | SA(sa) | 0x03)
#define MIPS_SRAV(rd, rt, rs)       (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x07)
#define MIPS_SRL(rd, rt, sa)        (OP(0x00) | RT(rt) | RD(rd) | SA(sa) | 0x02)
#define MIPS_SRLV(rd, rt, rs)       (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x06)
#define MIPS_SUB(rd, rs, rt)        (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x22)
#define MIPS_SUBU(rd, rs, rt)       (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x23)
#define MIPS_SW(rt, off, base)      (OP(0x2B) | RS(base) | RT(rt) | IM(off))
#define MIPS_SWC1(rt, off, base)    (OP(0x39) | RS(base) | RT(rt) | IM(off))
#define MIPS_SWL(rt, off, base)     (OP(0x2A) | RS(base) | RT(rt) | IM(off))
#define MIPS_SWR(rt, off, base)     (OP(0x2E) | RS(base) | RT(rt) | IM(off))
#define MIPS_SYSCALL(code)          ((code) << 6 | 0x0C)
#define MIPS_TEQ(rs, rt, code)      (OP(0x00) | RS(rs) | RT(rt) | ((code)&0x3FF) << 6 | 0x34)
#define MIPS_TEQI(rs, immd)         (OP(0x01) | RS(rs) | RT(0x0C) | IM(immd))
#define MIPS_TGE(rs, rt, code)      (OP(0x00) | RS(rs) | RT(rt) | ((code)&0x3FF) << 6 | 0x34)
#define MIPS_TGEI(rs, immd)         (OP(0x01) | RS(rs) | RT(0x08) | IM(immd))
#define MIPS_TGEIU(rs, immd)        (OP(0x01) | RS(rs) | RT(0x09) | IM(immd))
#define MIPS_TGEU(rs, rt, code)     (OP(0x00) | RS(rs) | RT(rt) | ((code)&0x3FF) << 6 | 0x31)
#define MIPS_TLBP()                 (OP(0x10) | 1 << 25 | 0x08)
#define MIPS_TLBR()                 (OP(0x10) | 1 << 25 | 0x01)
#define MIPS_TLBWI()                (OP(0x10) | 1 << 25 | 0x02)
#define MIPS_TLBWR()                (OP(0x10) | 1 << 25 | 0x06)
#define MIPS_TLT(rs, rt, code)      (OP(0x00) | RS(rs) | RT(rt) | ((code)&0x3FF) << 6 | 0x32)
#define MIPS_TLTI(rs, immd)         (OP(0x01) | RS(rs) | RT(0x0A) | IM(immd))
#define MIPS_TLTIU(rs, immd)        (OP(0x01) | RS(rs) | RT(0x0B) | IM(immd))
#define MIPS_TLTU(rs, rt, code)     (OP(0x00) | RS(rs) | RT(rt) | ((code)&0x3FF) << 6 | 0x33)
#define MIPS_TNE(rs, rt, code)      (OP(0x00) | RS(rs) | RT(rt) | ((code)&0x3FF) << 6 | 0x36)
#define MIPS_TNEI(rs, immd)         (OP(0x01) | RS(rs) | RT(0x0E) | IM(immd))
#define MIPS_XOR(rd, rs, rt)        (OP(0x00) | RS(rs) | RT(rt) | RD(rd) | 0x26)
#define MIPS_XORI(rt, rs, immd)     (OP(0x0E) | RS(rs) | RT(rt) | IM(immd))

/* MIPS Registers */
#define R0  MIPS_R0
#define AT  MIPS_AT
#define V0  MIPS_V0
#define V1  MIPS_V1
#define A0  MIPS_A0
#define A1  MIPS_A1
#define A2  MIPS_A2
#define A3  MIPS_A3
#define T0  MIPS_T0
#define T1  MIPS_T1
#define T2  MIPS_T2
#define T3  MIPS_T3
#define T4  MIPS_T4
#define T5  MIPS_T5
#define T6  MIPS_T6
#define T7  MIPS_T7
#define S0  MIPS_S0
#define S1  MIPS_S1
#define S2  MIPS_S2
#define S3  MIPS_S3
#define S4  MIPS_S4
#define S5  MIPS_S5
#define S6  MIPS_S6
#define S7  MIPS_S7
#define T8  MIPS_T8
#define T9  MIPS_T9
#define K0  MIPS_K0
#define K1  MIPS_K1
#define GP  MIPS_GP
#define SP  MIPS_SP
#define FP  MIPS_FP
#define RA  MIPS_RA

/* Definitions for prefixless macros */
#define ADD     MIPS_ADD
#define ADDI    MIPS_ADDI
#define ADDIU   MIPS_ADDIU
#define ANDI    MIPS_ANDI
#define BC1F    MIPS_BC1F
#define BC1FL   MIPS_BC1FL
#define BC1T    MIPS_BC1T
#define BC1TL   MIPS_BC1TL
#define BEQ     MIPS_BEQ
#define BEQL    MIPS_BEQL
#define BGEZ    MIPS_BGEZ
#define BGEZAL  MIPS_BGEZAL
#define BGEZALL MIPS_BGEZALL
#define BGEZL   MIPS_BGEZL
#define BGTZ    MIPS_BGTZ
#define BGTZL   MIPS_BGTZL
#define BLEZ    MIPS_BLEZ
#define BLEZL   MIPS_BLEZL
#define BLTZ    MIPS_BLTZ
#define BLTZAL  MIPS_BLTZAL
#define BLTZALL MIPS_BLTZALL
#define BLTZL   MIPS_BLTZL
#define BNE     MIPS_BNE
#define BNEL    MIPS_BNEL
#define BREAK   MIPS_BREAK
#define CACHE   MIPS_CACHE
#define CFC1    MIPS_CFC1
#define COP1    MIPS_COP1
#define CTC1    MIPS_CTC1
#define DADD    MIPS_DADD
#define DADDI   MIPS_DADDI
#define DADDIU  MIPS_DADDIU
#define DADDU   MIPS_DADDU
#define DDIV    MIPS_DDIV
#define DDIVU   MIPS_DDIVU
#define DIV     MIPS_DIV
#define DIVU    MIPS_DIVU
#define DMFC0   MIPS_DMFC0
#define DMTC0   MIPS_DMTC0
#define DMULT   MIPS_DMULT
#define DMULTU  MIPS_DMULTU
#define DSLL    MIPS_DSLL
#define DSLLV   MIPS_DSLLV
#define DSLL32  MIPS_DSLL32
#define DSRA    MIPS_DSRA
#define DSRAV   MIPS_DSRAV
#define DSRA32  MIPS_DSRA32
#define DSRL    MIPS_DSRL
#define DSRLV   MIPS_DSRLV
#define DSRL32  MIPS_DSRL32
#define DSUB    MIPS_DSUB
#define DSUBU   MIPS_DSUBU
#define J       MIPS_J
#define JAL     MIPS_JAL
#define JALR    MIPS_JALR
#define JR      MIPS_JR
#define LB      MIPS_LB
#define LBU     MIPS_LBU
#define LD      MIPS_LD
#define LDC1    MIPS_LDC1
#define LDL     MIPS_LDL
#define LDR     MIPS_LDR
#define LH      MIPS_LH
#define LHU     MIPS_LHU
#define LL      MIPS_LL
#define LLD     MIPS_LLD
#define LUI     MIPS_LUI
#define LW      MIPS_LW
#define LWC1    MIPS_LWC1
#define LWL     MIPS_LWL
#define LWR     MIPS_LWR
#define LWU     MIPS_LWU
#define MFC0    MIPS_MFC0
#define MFC1    MIPS_MFC1
#define MFHI    MIPS_MFHI
#define MFLO    MIPS_MFLO
#define MTC0    MIPS_MTC0
#define MTC1    MIPS_MTC1
#define MTHI    MIPS_MTHI
#define MTLO    MIPS_MTLO
#define MULT    MIPS_MULT
#define MULTU   MIPS_MULTU
#define NOR     MIPS_NOR
#define OR      MIPS_OR
#define ORI     MIPS_ORI
#define SB      MIPS_SB
#define SC      MIPS_SC
#define SCD     MIPS_SCD
#define SD      MIPS_SD
#define SDC1    MIPS_SDC1
#define SDL     MIPS_SDL
#define SDR     MIPS_SDR
#define SH      MIPS_SH
#define SLL     MIPS_SLL
#define SLLV    MIPS_SLLV
#define SLT     MIPS_SLT
#define SLTI    MIPS_SLTI
#define SLTIU   MIPS_SLTIU
#define SLTU    MIPS_SLTU
#define SRA     MIPS_SRA
#define SRAV    MIPS_SRAV
#define SRL     MIPS_SRL
#define SRLV    MIPS_SRLV
#define SUB     MIPS_SUB
#define SUBU    MIPS_SUBU
#define SW      MIPS_SW
#define SWC1    MIPS_SWC1
#define SWL     MIPS_SWL
#define SWR     MIPS_SWR
#define SYSCALL MIPS_SYSCALL
#define TEQ     MIPS_TEQ
#define TEQI    MIPS_TEQI
#define TGE     MIPS_TGE
#define TGEI    MIPS_TGEI
#define TGEIU   MIPS_TGEIU
#define TGEU    MIPS_TGEU
#define TLBP    MIPS_TLBP
#define TLBR    MIPS_TLBR
#define TLBWI   MIPS_TLBWI
#define TLBWR   MIPS_TLBWR
#define TLT     MIPS_TLT
#define TLTI    MIPS_TLTI
#define TLTIU   MIPS_TLTIU
#define TLTU    MIPS_TLTU
#define TNE     MIPS_TNE
#define TNEI    MIPS_TNEI
#define XOR     MIPS_XOR
#define XORI    MIPS_XORI
#define NOP     (0x00000000)
#define ERET    (OP(0x10) | 1 << 25 | 0x18)
#define SYNC    (0x0F)

static void write32BE(unsigned char *buf, unsigned long v) {
  buf[0] = v>>24;
  buf[1] = v>>16;
  buf[2] = v>>8;
  buf[3] = v;
}

