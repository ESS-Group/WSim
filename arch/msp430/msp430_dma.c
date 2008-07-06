/**
 *  \file   msp430_dma.c
 *  \brief  MSP430 DMA engine
 *  \author Antoine Fraboulet
 *  \date   2006
 **/

#include <stdio.h>

#include "arch/common/hardware.h"
#include "msp430.h"

#if defined(__msp430_have_dma)
/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_dma_reset()
{
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_dma_update()
{
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int16_t msp430_dma_read  (uint16_t addr)
{
  ERROR("msp430:dma: read  [0x%04x] block not implemented (PC=0x%04x)\n",addr,mcu_get_pc());
  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_dma_write (uint16_t addr, int16_t val)
{
  ERROR("msp430:dma: write [0x%04x] = 0x%04x, block not implemented (PC=0x%04x)\n",addr,val,mcu_get_pc());
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#if 0
int msp430_dma_chkifg()
{
  msp430_interrupt_set(INTR_DMA);
  return 0;
}
#endif

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
#endif

