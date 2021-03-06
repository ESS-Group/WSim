
/**
 *  \file   msp430_debug.c
 *  \brief  MSP430 MCU emulation debug
 *  \author Antoine Fraboulet
 *  \date   2005
 **/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "arch/common/hardware.h"
#include "msp430.h"

#define OP_MOV	  0x4
#define OP_ADD	  0x5
#define OP_ADDC	  0x6
#define OP_SUBC	  0x7
#define OP_SUB	  0x8
#define OP_CMP	  0x9
#define OP_DADD	  0xa
#define OP_BIT	  0xb
#define OP_BIC	  0xc
#define OP_BIS	  0xd
#define OP_XOR	  0xe
#define OP_AND	  0xf

#define OP_JMP	  0x3c
#define OP_JL	  0x38
#define OP_JGE	  0x34
#define OP_JN	  0x30
#define OP_JC	  0x2c
#define OP_JNC	  0x28
#define OP_JZ	  0x24
#define OP_JNZ	  0x20

#define OP_SXT	  0x118
#define OP_CALL	  0x128
#define OP_RETI	  0x130
#define OP_PUSH	  0x120
#define OP_SWPB	  0x108
#define OP_RRA	  0x110
#define OP_RRC	  0x100

#define OP_PUSHM   0x14
#define OP_POPM	   0x16

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

char* msp430_debug_opcode(unsigned short opcode, int b)
{
  static char buff[60];
  buff[0] = 0;

  switch (opcode)
    {
    case OP_MOV:  sprintf(buff,"mov%s ",b?".b":"  "); break;
    case OP_ADD:  sprintf(buff,"add%s ",b?".b":"  "); break;
    case OP_ADDC: sprintf(buff,"addc%s",b?".b":"  "); break;
    case OP_SUBC: sprintf(buff,"subc%s",b?".b":"  "); break;
    case OP_SUB:  sprintf(buff,"sub%s ",b?".b":"  "); break;
    case OP_CMP:  sprintf(buff,"cmp%s ",b?".b":"  "); break;
    case OP_DADD: sprintf(buff,"dadd%s",b?".b":"  "); break;
    case OP_BIT:  sprintf(buff,"bit%s ",b?".b":"  "); break;
    case OP_BIC:  sprintf(buff,"bic%s ",b?".b":"  "); break;
    case OP_BIS:  sprintf(buff,"bis%s ",b?".b":"  "); break;
    case OP_XOR:  sprintf(buff,"xor%s ",b?".b":"  "); break;
    case OP_AND:  sprintf(buff,"and%s ",b?".b":"  "); break;

    case OP_JMP:  sprintf(buff,"jmp   "); break;
    case OP_JL:   sprintf(buff,"jl    "); break;
    case OP_JGE:  sprintf(buff,"jge   "); break;
    case OP_JN:   sprintf(buff,"jn    "); break;
    case OP_JC:   sprintf(buff,"jc    "); break;
    case OP_JNC:  sprintf(buff,"jnc   "); break;
    case OP_JZ:   sprintf(buff,"jz    "); break;
    case OP_JNZ:  sprintf(buff,"jnz   "); break;

    case OP_SXT:  sprintf(buff,"sxt   "); break;
    case OP_RETI: sprintf(buff,"reti  "); break;
    case OP_CALL: sprintf(buff,"call  "); break;
    case OP_PUSH: sprintf(buff,"push  "); break;
    case OP_RRA:  sprintf(buff,"rra   "); break;
    case OP_SWPB: sprintf(buff,"swpb  "); break;
    case OP_RRC:  sprintf(buff,"rrc   "); break;

    case OP_PUSHM:sprintf(buff,"pushm "); break;
    case OP_POPM: sprintf(buff,"popm  "); break;

    default: sprintf(buff,"unknown"); break;
    }

  return buff;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

char* mcu_regname_str(unsigned r)
{
  /*
    char *msp430_register_names[] =
    { "r0",   "r1",  "r2",    "r3",   "r4",   "r5",   "r6",   "r7",
      "r8",   "r9",  "r10",   "r11",  "r12",  "r13",  "r14",  "r15"};
  */
  switch (r)
    {
    case  0: return "pc";
    case  1: return "sp";
    case  2: return "sr"; /* cg1 */
    case  3: return "r3"; /* cg2 */
    case  4: return "r4";
    case  5: return "r5";
    case  6: return "r6";
    case  7: return "r7";
    case  8: return "r8";
    case  9: return "r9";
    case 10: return "r10";
    case 11: return "r11";
    case 12: return "r12";
    case 13: return "r13";
    case 14: return "r14";
    case 15: return "r15";
    default: return "xx";
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_print_registers(int columns)
{
  int i;
  for(i=0; i < 16; i++)
    {
      HW_DMSG_MSP(" %3s:0x%04x",mcu_regname_str(i),MCU_REGS[i] & 0xffffu);
      if (((i+1) % columns) == 0)
	HW_DMSG_MSP("\n");
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_print_stack(int lines)
{
  int i;
  uint16_t sp = MCU_REGS[1];
  HW_DMSG_MCU(" stack dump: sp = 0x%04x\n",sp);

  for(i = 0; i < lines; i++)
    {
      UNUSED uint16_t adr  = sp + 2*(lines-1) - 2*i; 
      UNUSED uint16_t data = mcu_jtag_read_word(adr); 
      HW_DMSG_MCU("   0x%04x: 0x%04x - %c%c\n", adr, data,
	      isprint((data >> 8) & 0xff) ? (data >> 8) & 0xff : '.',
	      isprint((data     ) & 0xff) ? (data     ) & 0xff : '.');
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/*
 * bits in SR register
 * ===================
 *  7 : scg1    system clock generator 1 == SMCLK
 *  6 : scg0    system clock generator 0 == dco
 *  5 : oscoff  oscillator off           == lfxt1
 *  4 : cpuoff  cpu off                  == MCLK
 *
 * 4 bits value
 * ============
 *  0 -> AM
 *  1 -> LPM0
 *  5 -> LPM1
 *  9 -> LPM2
 * 13 -> LPM3
 * 15 -> LPM4
 *
 */

char* msp430_lpm_names[] = 
  {
    "Active mode" , "LPM0", 
    "LPM id 0x2"  , "LPM id 0x3",
    "LPM id 0x4"  , "LPM1", 
    "LPM id 0x6"  , "LPM id 0x7",
    "LPM id 0x8"  , "LPM2",
    "LPM id 0xa"  , "LPM id 0xb",
    "LPM id 0xc"  , "LPM3",
    "LPM id 0xe"  , "LPM4"
  };

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
