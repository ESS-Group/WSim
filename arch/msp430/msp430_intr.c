
/**
 *  \file   msp430_intr.c
 *  \brief  MSP430 MCU interrupt
 *  \author Antoine Fraboulet
 *  \date   2006
 **/

#include "arch/common/hardware.h"
#include "msp430.h"

/******************************************************************************************/
/** INTERRUPTS ****************************************************************************/
/******************************************************************************************/

void msp430_interrupt_set(int intr_num)
{
#if defined(__msp430_have_16_interrupts)
   uint16_t one = 1;
#elif defined(__msp430_have_32_interrupts)
   uint32_t one = 1;
#elif defined(__msp430_have_64_interrupts)
   uint64_t one = 1;
#endif
  if ((SR & MASK_GIE) || (intr_num >= INTR_FIRST_NMI)) // interrupt enable or NMI
    {
      HW_DMSG_INTR("msp430:intr: Interrupt %d to be scheduled vector : 0x%04x -> 0x%04x [%"PRIu64"]\n",
		   intr_num,MCU_IV, MCU_IV | (one << intr_num), MACHINE_TIME_GET_NANO());
      MCU_IV |= one << intr_num;
    }
  else
    {
      WARNING("msp430:intr: Interrupt %d received but GIE = 0 (current PC = 0x%04x [%"PRIu64"])\n",
	      intr_num,mcu_get_pc(), MACHINE_TIME_GET_NANO());
      MCU_IV |= one << intr_num;
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int msp430_interrupt_start_if_any(void)
{
  /**
   * 3 types of interrupts
   *
   * [msp430f1611.pdf page 11]
   *
   * System reset
   * (Non)-maskable NMI
   * Maskable 
   *
   * GIE: General interrupt enable. This bit, when set, enables maskable
   * interrupts. When reset, all maskable interrupts are disabled.
   *
   *
   * Power-up, external
   * reset, watchdog,      WDTIFG   Reset          0FFFEh  15, highest
   * flash password        KEYV              
   *                               
   * NMI, oscillator fault, NMIIFG (non)-maskable
   * flash memory access    OFIFG  (non)-maskable  0FFFCh      14
   * violation              ACCVIFG(non)-maskable
   * 
   * device-specific                               0FFFAh      13
   * device-specific                               0FFF8h      12
   * device-specific                               0FFF6h      11
   * Watchdog timer         WDTIFG     maskable    0FFF4h      10     
   * device-specific                               0FFF2h       9     USART0 rx
   * device-specific                               0FFF0h       8     USART0 tx
   * device-specific                               0FFEEh       7     ADC12
   * device-specific                               0FFECh       6     TimerA ccr0
   * device-specific                               0FFEAh       5     TimerA ccr1 ccr2
   * device-specific                               0FFE8h       4     P1
   * device-specific                               0FFE6h       3     USART1 rx
   * device-specific                               0FFE4h       2     USART1 tx
   * device-specific                               0FFE2h       1     P2
   * device-specific                               0FFE0h  0, lowest  Basic Timer
   * 
   *
   */

  uint32_t ivector;
  uint16_t next_pc;

#if defined(SOFT_INTR)
  if ((MCU.soft_intr == 1) && (MACHINE_TIME_GET_NANO() >= MCU.soft_intr_timeend))
    {
      etracer_slot_event(SOFT_INTR_EVT, ETRACER_PER_EVT_MODE_CHANGED, 1, 0);
      MCU.soft_intr = 0;
    }
#endif

  if (MCU_IV && ((SR & MASK_GIE) || (MCU_IV & (1 << 14)) || (MCU_IV & (1 << 15))))
    {
      /* get ISR address with biggest priority */
#if defined(__msp430_have_16_interrupts)
      uint16_t ibit,inum;
      for(ibit=0x8000u, inum=15; ibit; ibit >>= 1, inum--)
#elif defined(__msp430_have_32_interrupts)
      uint32_t ibit,inum;
      for(ibit=0x80000000u, inum=31; ibit; ibit >>= 1, inum--)
#elif defined(__msp430_have_64_interrupts)
      uint64_t ibit,inum;
      for(ibit=0x8000000000000000u, inum=63; ibit; ibit >>= 1, inum--)
#endif
	{
	  if (MCU_IV & ibit)
	    {
	      ivector = INTR_BASE_IV_ADDR + inum * 2;
	      next_pc = msp430_read_short(ivector);
	      
	      HW_DMSG_INTR("msp430:intr: IRQ %d [%"PRId64"] PC [0x%04x] jumps to [0x%04x], IV=0x%04x, SR=0x%04x\n",
			   inum,MACHINE_TIME_GET_NANO(),mcu_get_pc(),next_pc,MCU_IV,SR);
	      TRACER_TRACE_INTR(inum);
	      /* push pc*/
	      SP -= 2;
	      /* we are executed at the end of an instruction. As this instruction is
	       * done we have to push what should be the next instruction (resume insn)
	       */
	      msp430_write_short(SP,mcu_get_pc_next()); 
	      /* push sr */
	      SP -= 2;
	      msp430_write_short(SP,SR);
	      /* load pc : substitutes next instruction */
	      mcu_set_pc_next( next_pc );

	      if (RUNNING_MODE() != RUNNING_AM)
		{
		  uint16_t UNUSED nsr;
		  HW_DMSG_LPM("msp430:intr: IRQ %d, LPM changed, going to AM\n",inum);
		  HW_DMSG_LPM("msp430:intr:     from (SCG1:%d,SCG0:%d,OSCOFF:%d,CPUOFF:%d)\n",
			      (SR>>SHIFT_SCG1)&1,(SR>>SHIFT_SCG0)&1,(SR>>SHIFT_OSCOFF)&1,(SR>>SHIFT_CPUOFF)&1);
		  nsr = SR & MASK_SCG0;
		  HW_DMSG_LPM("msp430:intr:     to   (SCG1:%d,SCG0:%d,OSCOFF:%d,CPUOFF:%d)\n",
			      (nsr>>SHIFT_SCG1)&1,(nsr>>SHIFT_SCG0)&1,(nsr>>SHIFT_OSCOFF)&1,(nsr>>SHIFT_CPUOFF)&1);
		  mcu_signal_add(SIG_MCU_LPM_CHANGE);
		}

	      /* says SCG0 is not cleared on interrupt */
	      SR = SR & MASK_SCG0; // reset SR, except SCG0

	      MCU_ALU.irq_counter += 1;

	      /*****************************************/
	      /* reset IFG for single-source interrupt */
	      /*****************************************/
	      switch (inum)
		{
		  /* NMI interrupt flag must be reset by software */

		case INTR_WATCHDOG:
		  if (msp430_watchdog_getmode() == WDT_MODE_INTERVAL)
		    {
		      HW_DMSG_INTR("msp430:intr:   Watchdog interval mode reset IFG flag\n");
		      MCU.sfr.ifg1.b.wdtifg = 0;
		    }
		  break;

#if defined(__msp430_have_timera3)
		case INTR_TIMERA_0:
		  HW_DMSG_INTR("msp430:intr:   Reset timerA taccr0 IFG flag\n");
		  MCU.timerA.tacctl[0].b.ccifg = 0;
#if defined(SOFT_INTR)
		  MCU.soft_intr         = 1;
		  MCU.soft_intr_timeend = MACHINE_TIME_GET_NANO() + SOFT_INTR_DUR;
		  etracer_slot_event(SOFT_INTR_EVT, ETRACER_PER_EVT_MODE_CHANGED, 2, 0);
#endif
		  break;
#endif

#if defined(__msp430_have_timerb3) || defined(__msp430_have_timerb7)
		case INTR_TIMERB_0:
		  HW_DMSG_INTR("msp430:intr:   Reset timerB IFG flag\n");
		  MCU.timerB.tbcctl[0].b.ccifg = 0;
		  break;
#endif

#if defined(__msp430_have_timerTA0)
		case INTR_TIMERTA0_0:
		  HW_DMSG_INTR("msp430:intr:   Reset timerTA0 taccr0 IFG flag\n");
		  MCU.timerTA0.ta0cctl[0].b.ccifg = 0;
#if defined(SOFT_INTR)
		  MCU.soft_intr         = 1;
		  MCU.soft_intr_timeend = MACHINE_TIME_GET_NANO() + SOFT_INTR_DUR;
		  etracer_slot_event(SOFT_INTR_EVT, ETRACER_PER_EVT_MODE_CHANGED, 2, 0);
#endif
		  break;
#endif

#if defined(__msp430_have_timerTA1)
 		case INTR_TIMERTA1_0:
		  HW_DMSG_INTR("msp430:intr:   Reset timerTA1 taccr1 IFG flag\n");
		  MCU.timerTA1.ta1cctl[0].b.ccifg = 0;
#if defined(SOFT_INTR)
		  MCU.soft_intr         = 1;
		  MCU.soft_intr_timeend = MACHINE_TIME_GET_NANO() + SOFT_INTR_DUR;
		  etracer_slot_event(SOFT_INTR_EVT, ETRACER_PER_EVT_MODE_CHANGED, 2, 0);
#endif
		  break;
#endif

#if ( defined(__msp430_have_uscia0) || defined(__msp430_have_uscib0) ) && !defined(__msp430_have_new_sfr)
		case INTR_USCIX0_RX:
		  #if defined(__msp430_have_uscia0)
		    HW_DMSG_INTR("msp430:intr:   Reset USCIA0 rx IFG flag\n");
		    MCU.sfr.ifg2.b.uca0rxifg = 0;
		  #else
		    HW_DMSG_INTR("msp430:intr:   Reset USCIB0 rx IFG flag\n");
		    MCU.sfr.ifg2.b.ucb0rxifg = 0;
		  #endif
		  break;
		case INTR_USCIX0_TX:
		  #if defined(__msp430_have_uscia0)
		    HW_DMSG_INTR("msp430:intr:   Reset USCIA0 tx IFG flag\n");
		    MCU.sfr.ifg2.b.uca0txifg = 0;
		  #else
		    HW_DMSG_INTR("msp430:intr:   Reset USCIB0 tx IFG flag\n");
		    MCU.sfr.ifg2.b.ucb0txifg = 0;
		  #endif
		  break;
#endif
		  	  
#if defined(__msp430_have_usart0) && !defined(__msp430_have_new_sfr)
		case INTR_USART0_RX:
		  HW_DMSG_INTR("msp430:intr:   Reset USART0 rx IFG flag\n");
		  MCU.sfr.ifg1.b.urxifg0 = 0;
		  break;
		case INTR_USART0_TX:
		  HW_DMSG_INTR("msp430:intr:   Reset USART0 tx IFG flag\n");
		  MCU.sfr.ifg1.b.utxifg0 = 0;
		  break;
#endif
		
#if defined(__msp430_have_usart1) && !defined(__msp430_have_new_sfr)
		case INTR_USART1_RX:
		  HW_DMSG_INTR("msp430:intr:   Reset USART1 rx IFG flag\n");
		  MCU.sfr.ifg2.b.urxifg1 = 0;
		  break;
		case INTR_USART1_TX:
		  HW_DMSG_INTR("msp430:intr:   Reset USART1 tx IFG flag\n");
		  MCU.sfr.ifg2.b.utxifg1 = 0;
		  break;
#endif
		}
	      
	      MCU_IV &= ~ibit;
	      return 1;
	    }
	}
    }
  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/* called on RETI */

int msp430_interrupt_checkifg(void)
{
  int res = 0;

  /* sfr */
  /* svs */
  /* system clock */
  /* general */
  res |= msp430_digiIO_chkifg();
#if defined(__msp430_have_dma)
  res |= msp430_dma_chkifg();
#endif
#if defined(__msp430_have_flash)
  res |= msp430_flash_chkifg();
#endif
  /* usci */
#if defined(__msp430_have_uscia0)
  res |= msp430_uscia0_chkifg();
#endif
#if defined(__msp430_have_uscib0)
  res |= msp430_uscib0_chkifg();
#endif
  /* usart */
#if defined(__msp430_have_usart0)
  res |= msp430_usart0_chkifg();
#endif
#if defined(__msp430_have_usart1)
  res |= msp430_usart1_chkifg();
#endif
  /* timers */
#if defined(__msp430_have_watchdog)
  res |= msp430_watchdog_chkifg();
#endif
#if defined(__msp430_have_basic_timer)
  res |= msp430_basic_timer_chkifg();
#endif

  res |= msp430_timerA_chkifg();
  res |= msp430_timerB_chkifg();

#if defined(__msp430_have_adc12)
  res |= msp430_adc12_chkifg();
#endif
#if defined(__msp430_have_adc10)
  res |= msp430_adc10_chkifg();
#endif
#if defined(__msp430_have_dac12)
  res |= msp430_dac12_chkifg();
#endif
#if defined(__msp430_have_cc1101)
  res |= cc1101mm_chkifg();
#endif
  return res;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
