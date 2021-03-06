
/**
 *  \file   cc1100_2500_state.c
 *  \brief  CC1100/CC2500 state functions
 *  \author Guillaume Chelius
 *  \date   2006
 **/

/*
 *  cc1100_2500_state.c
 *  
 *
 *  Created by Guillaume Chelius on 16/02/06.
 *  Copyright 2006 __WorldSens__. All rights reserved.
 *  Modified by Loic Lemaitre 2010
 *
 */

/**
 * TODO:
 *   - wake-on-radio
 **/

/**
 * Implemented states:
 *   - POWER_DOWN
 *   - POWER_UP
 *   - IDLE
 *   - MANCAL
 *   - SLEEP
 *   - XOFF
 *   - TX_UNDERFLOW
 *   - RX_UNDERFLOW
 *   - FS_WAKEUP
 *   - CALIBRATE
 *   - FS_CALIBRATE
 *   - SETTLING
 *   - FSTXON
 *   - TXRX_SETTLING
 *   - RXTX_SETTLING
 *   - RX
 *   - TX
 **/

#include "cc1100_2500_internals.h"

/***************************************************/
/***************************************************/
/***************************************************/

int cc1100_update_state_idle (struct _cc1100_t *cc1100) 
{
  /* check if we are in wake on radio mode */
  if (cc1100->wor)
    {
      /* going to rx mode or to sleep mode? */
      if (cc1100->fsm_pending != CC1100_STATE_SLEEP)
	{
	  /* IDLE -> FS_WAKEUP */
	  /* check if wor event1 is reached */
	  if (MACHINE_TIME_GET_NANO() >= cc1100->wor_timer_event1)
	    {
	      /* wake on radio, we have reached event1 point, so start going to RX state (AN047 p7) */
	      cc1100->fsm_pending = CC1100_STATE_RX;
	      CC1100_FS_WAKEUP_ENTER(cc1100);
	    }
	}
      else
	{
	  /* IDLE -> SLEEP */
	  if (cc1100->CSn_pin == 0xFF)
	    {
	      /* cf AN047 p8 */
	      if (cc1100->registers[CC1100_REG_WORCTRL] & 0x08)   /* RC oscillator automatically calibrated */
		{
		  if (cc1100->rc_cal_timer <= MACHINE_TIME_GET_NANO())   /* RC calibration over */
		    {
		      CC1100_SLEEP_REALLY_ENTER(cc1100);
		    }
		}
	      else
		{
		  CC1100_SLEEP_REALLY_ENTER(cc1100);
		}
	    }
	}
    }

  /* Read pins */
  return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/

int cc1100_update_state_mancal (struct _cc1100_t *cc1100) 
{
  /* Check if calibration is over */
  if (MACHINE_TIME_GET_NANO() >= cc1100->fsm_timer) 
    {
      CC1100_CALIBRATE(cc1100); 
      CC1100_IDLE_ENTER(cc1100);
    }
	
  /* Read pins */
  return cc1100_io_pins(cc1100);
}

int cc1100_update_state_calibrate (struct _cc1100_t *cc1100) 
{
  /* Check if calibration is over */
  if (MACHINE_TIME_GET_NANO() >= cc1100->fsm_timer) 
    {
      CC1100_CALIBRATE(cc1100); 
      CC1100_IDLE_ENTER(cc1100);
    }
	
  /* Read pins */
  return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/

int cc1100_update_state_sleep (struct _cc1100_t *cc1100) 
{
  /* check if we are in wake on radio mode */
  if (cc1100->wor)
    {
      /* check if wor event0 is reached */
      if (MACHINE_TIME_GET_NANO() >= cc1100->wor_timer_event0)
	{
	  cc1100->wor_timer_event1 = MACHINE_TIME_GET_NANO() + cc1100_get_event1_period(cc1100);
	  CC1100_DBG_STATE("cc1100:state: event1 set at %"PRIu64" [%"PRIu64"]\n",
			   cc1100->wor_timer_event1, MACHINE_TIME_GET_NANO());
	  CC1100_IDLE_ENTER(cc1100);
	  cc1100->rc_cal_timer = MACHINE_TIME_GET_NANO() + CC1100_RC_CALIBRATE_DELAY_NS;
	}
    }
  else
    {
      /* Check if we are waking up through CSn pin */
      /* CSn will go low at end of the strobe      */
      if (cc1100_read_pin(cc1100, CC1100_INTERNAL_CSn_PIN) == 0x00) 
	{
	  CC1100_DBG_PINS("cc1100:pins: CSn down, going from SLEEP to Idle at %"PRId64"\n",
			  MACHINE_TIME_GET_NANO());
	  CC1100_IDLE_ENTER(cc1100);
	}
    }
  return 0;
}

int cc1100_update_state_xoff (struct _cc1100_t *cc1100) 
{
  /* Check if we are waking up through CSn pin */
  if (cc1100_read_pin(cc1100, CC1100_INTERNAL_CSn_PIN) == 0x00) 
    {
      CC1100_DBG_PINS("cc1100:pins: CSn down, going from XOFF to Idle at %"PRId64"\n",
		      MACHINE_TIME_GET_NANO());
      CC1100_IDLE_ENTER(cc1100);
    }
  else
    {
      //
    }
  return 0;
}

/***************************************************/
/***************************************************/
/***************************************************/

int cc1100_update_state_tx_underflow (struct _cc1100_t *cc1100) 
{
  /* Nothing to do, except reading pins */
  return cc1100_io_pins(cc1100);
}

int cc1100_update_state_rx_overflow (struct _cc1100_t *cc1100) 
{
  /* Nothing to do, except reading pins */
  return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/
int cc1100_update_state_fs_wakeup (struct _cc1100_t *cc1100) {

	/* Check if wakeup is over */
	if (MACHINE_TIME_GET_NANO() < cc1100->fsm_timer) {
		return cc1100_io_pins(cc1100);		
	}
	
	/* Check if we need to calibrate */
	if (((cc1100->registers[CC1100_REG_MCSM0] >> 4) & 0x03) == 0x01) {
		CC1100_FS_CALIBRATE_ENTER(cc1100);
	} else {
		CC1100_SETTLING_ENTER(cc1100);
	}
	
	/* Read pins */
	return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/
int cc1100_update_state_fs_calibrate (struct _cc1100_t *cc1100) {
  
	/* Check if calibration is over */
	if (MACHINE_TIME_GET_NANO() >= cc1100->fsm_timer) {
		CC1100_CALIBRATE(cc1100);
		CC1100_SETTLING_ENTER(cc1100);
	}
	
	/* Read pins */
	return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/

int cc1100_update_state_settling (struct _cc1100_t *cc1100) 
{
  /* Check if wakeup is over */
  if (MACHINE_TIME_GET_NANO() < cc1100->fsm_timer) 
    {
      return cc1100_io_pins(cc1100);	
    }
  
  /* Check if we need to calibrate */
  switch (cc1100->fsm_pending) 
    {
    case CC1100_STATE_TX:
      CC1100_TX_ENTER(cc1100);
      break;
    case CC1100_STATE_RX:
      CC1100_RX_ENTER(cc1100);
      break;
    case CC1100_STATE_FSTXON:
      CC1100_FSTXON_ENTER(cc1100);
      break;
    default:
      CC1100_DBG_EXC("cc1100: EXCEPTION (cc1100): should not be here\n");
      break;
    }
	
  /* Read pins */
  return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/
int cc1100_update_state_rxtx_settling (struct _cc1100_t *cc1100) {
	
	/* Check if wakeup is over */
	if (MACHINE_TIME_GET_NANO() < cc1100->fsm_timer) {
		return cc1100_io_pins(cc1100);		
	}
	
	/* Check if we need to calibrate */
	switch (cc1100->fsm_pending) {
		case CC1100_STATE_TX:
			CC1100_TX_ENTER(cc1100);
			break;
		case CC1100_STATE_FSTXON:
			CC1100_DBG_STATE("cc1100:state: FSTXON (2)\n");
			CC1100_FSTXON_ENTER(cc1100);
			break;
		default:
			CC1100_DBG_EXC("cc1100:state: EXCEPTION (cc1100): should not be here\n");
			break;
	}
	
	/* Read pins */
	return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/
int cc1100_update_state_txrx_settling (struct _cc1100_t *cc1100) {
	
	/* Check if wakeup is over */
	if (MACHINE_TIME_GET_NANO() >= cc1100->fsm_timer) {
		CC1100_RX_ENTER(cc1100);
	}
	
	/* Read pins */
	return cc1100_io_pins(cc1100);
}


/***************************************************/
/***************************************************/
/***************************************************/
int cc1100_update_state_fstxon (struct _cc1100_t *cc1100) {
	/* Nothing to do, except reading pins */
	return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/
int cc1100_update_state_tx (struct _cc1100_t *cc1100) {
	if (cc1100->fsm_ustate == 5) {
		if (MACHINE_TIME_GET_NANO() >= cc1100->fsm_timer) {
		    /* PA_PD signal go from low to high when leaving TX state */
		    cc1100_assert_gdo(cc1100, 0x1B, CC1100_PIN_ASSERT);   
		    CC1100_IDLE_ENTER(cc1100);
		}
	} else {
		cc1100_tx(cc1100);
	}
	
	return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/

int cc1100_update_state_rx (struct _cc1100_t *cc1100) 
{
  CC1100_UPDATE_RSSI(cc1100);
  cc1100_compute_cca(cc1100);

  if (cc1100->fsm_pending != CC1100_STATE_IDLE) 
    {
      if (cc1100->registers[CC1100_REG_PKTSTATUS] & 0x10) 
	{
	  CC1100_RX_END_FORCED(cc1100);
	  CC1100_RXTX_SETTLING_ENTER(cc1100);
	}
      return cc1100_io_pins(cc1100);
    }

  /* check if a rx timeout is defined */
  if ((~cc1100->registers[CC1100_REG_MCSM2]) & 0x07)
    {
      /* check if a rx timeout is over */
      if (MACHINE_TIME_GET_NANO() >= cc1100->rx_timeout)
	{
	  /* check if no packet is starting to be received (cf 19.7 p46) */
	  if (cc1100->registers[CC1100_REG_MCSM2] & 0x20)  /* RX_TIME_QUAL = 1 */
	    {
	      if ( (cc1100->fsm_ustate < CC1100_RX_SEND_DATA) &&                    /* no sync word found and */
		   ((~cc1100_read_register(cc1100, CC1100_REG_PKTSTATUS)) & 0x20) ) /* preamble threshold not reached */
		{
		  CC1100_IDLE_ENTER(cc1100);
		  cc1100->fsm_pending = CC1100_STATE_SLEEP;
		}
	    }
	  else  /* RX_TIME_QUAL = 0 */
	    {
	      if (cc1100->fsm_ustate < CC1100_RX_SEND_DATA)     /* no sync word found */
		{
		  logpkt_rx_abort_pkt(cc1100->worldsens_radio_id, "no sync word found");
		  CC1100_IDLE_ENTER(cc1100);
		  cc1100->fsm_pending = CC1100_STATE_SLEEP;
		}
	    }
	}
    }

  /*  
      if (MACHINE_TIME_GET_NANO() > (cc1100->rx_io_timer + 250)) 
        {
          CC1100_INIT_CS(cc1100);
          CC1100_INIT_RSSI(cc1100);			
	}
  */

  if (cc1100->fsm_ustate >= CC1100_RX_SEND_SFD)
    {
      if (MACHINE_TIME_GET_NANO() >= cc1100->rx_io_timer + CC1100_SYNCHRO_DELAY_THRESHOLD)
	{
	  if (cc1100->fsm_ustate == CC1100_RX_SEND_END) 
	    {
	      /* packet is done */
	      logpkt_rx_complete_pkt(cc1100->worldsens_radio_id);
	      CC1100_IDLE_ENTER(cc1100); 
	    }
	  else
	    {
	      /* Interrupted reception      */
	      logpkt_rx_abort_pkt(cc1100->worldsens_radio_id, "interrupted reception");
	      CC1100_RX_END(cc1100);		
	    }
	} 
    }
	
  return cc1100_io_pins(cc1100);
}

/***************************************************/
/***************************************************/
/***************************************************/

#if defined(CC1101MM)
int cc1100_update_state (struct _cc1100_t *cc1100)
{
#else
int cc1100_update (int dev_num)
{
  struct _cc1100_t *cc1100 = (struct _cc1100_t *) machine.device[dev_num].data;
#endif
    /* Update gdo */
  cc1100_update_xosc(cc1100);
	
  switch (cc1100->fsm_state) 
    {
    case CC1100_STATE_IDLE:
      return cc1100_update_state_idle(cc1100);
    case CC1100_STATE_MANCAL:
      return cc1100_update_state_mancal(cc1100);
    case CC1100_STATE_SLEEP:
      return cc1100_update_state_sleep(cc1100);
    case CC1100_STATE_XOFF:
      return cc1100_update_state_xoff(cc1100);
    case CC1100_STATE_TX_UNDERFLOW:
      return cc1100_update_state_tx_underflow(cc1100);
    case CC1100_STATE_RX_OVERFLOW:
      return cc1100_update_state_rx_overflow(cc1100);
    case CC1100_STATE_FS_WAKEUP:
      return cc1100_update_state_fs_wakeup(cc1100);
    case CC1100_STATE_CALIBRATE:
      return cc1100_update_state_calibrate(cc1100);
    case CC1100_STATE_FS_CALIBRATE:
      return cc1100_update_state_fs_calibrate(cc1100);
    case CC1100_STATE_SETTLING:
      return cc1100_update_state_settling(cc1100);
    case CC1100_STATE_TXRX_SETTLING:
      return cc1100_update_state_txrx_settling(cc1100);
    case CC1100_STATE_RXTX_SETTLING:
      return cc1100_update_state_rxtx_settling(cc1100);
    case CC1100_STATE_FSTXON:
      return cc1100_update_state_fstxon(cc1100);
    case CC1100_STATE_RX:
      return cc1100_update_state_rx(cc1100);
    case CC1100_STATE_TX:
      return cc1100_update_state_tx(cc1100);
    default:
      CC1100_DBG_EXC("cc1100:state: state %d not implemented\n", cc1100->fsm_state);
      break;
    }
  
  return 0;
}

/***************************************************/
/***************************************************/
/***************************************************/
