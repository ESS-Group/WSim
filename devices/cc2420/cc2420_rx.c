
/**
 *  \file   cc2420_rx.c
 *  \brief  CC2420 Rx methods
 *  \author Nicolas Boulicault
 *  \date   2007
 **/

/*
 *  cc2420_rx.c
 *  
 *
 *  Created by Nicolas Boulicault on 04/06/07.
 *  Copyright 2007 __WorldSens__. All rights reserved.
 *
 */

#include <string.h>
#include <math.h>

#include "cc2420.h"
#include "cc2420_ram.h"
#include "cc2420_registers.h"
#include "cc2420_fifo.h"
#include "cc2420_tx.h"
#include "cc2420_rx.h"
#include "cc2420_macros.h"
#include "cc2420_internals.h"
#include "cc2420_debug.h"
#include "cc2420_crc_ccitt.h"


/**
 * update rssi value
 */

void cc2420_record_rssi(struct _cc2420_t * cc2420, uint8_t dBm) {

    uint8_t cca_mode;
    uint8_t cca_threshold;
    uint8_t cca_hyst;
    uint8_t old_CCA_pin;

    cc2420->rx_rssi_value = ( (cc2420->rx_rssi_values * cc2420->rx_rssi_value) + (dBm - CC2420_RSSI_OFFSET) ) / (cc2420->rx_rssi_values + 1);
    cc2420->rx_rssi_values ++;					

    if (cc2420->rx_rssi_values < 8) {				
	cc2420->rx_rssi_valid = 0;
    }								

    cc2420->rx_rssi_valid = 1;
    
    /* update RSSI value register */
    cc2420->registers[CC2420_REG_RSSI] |= cc2420->rx_rssi_value;

    /* 
     * update CCA 
     */

    cca_mode      = CC2420_REG_MDMCTRL0_CCA_MODE(cc2420->registers[CC2420_REG_MDMCTRL0]) >> 6;
    cca_hyst      = CC2420_REG_MDMCTRL0_CCA_HYST(cc2420->registers[CC2420_REG_MDMCTRL0]) >> 8;
    cca_threshold = CC2420_REG_RSSI_CCA_THR (cc2420->registers[CC2420_REG_RSSI]) >> 8;

    old_CCA_pin = cc2420->CCA_pin;
	
    switch (cca_mode) {
    case CC2420_CCA_MODE_RESERVED :
	cc2420->CCA_pin = 0xFF;
	break;

    case CC2420_CCA_MODE_THRESHOLD :
	if (!cc2420->rx_rssi_valid) {
	    cc2420->CCA_pin = 0x00;
	    break;
	}
	if (cc2420->CCA_pin == 0x00) {
	    if (cc2420->rx_rssi_value > cca_threshold)
		cc2420->CCA_pin = 0xFF;
	}
	else if (cc2420->CCA_pin == 0xFF) {
	    if (cc2420->rx_rssi_value <= (cca_threshold - cca_hyst) ) {
		cc2420->CCA_pin = 0x00;
	    }
	}
	break;
    
    case CC2420_CCA_MODE_DATA :
	if (cc2420->rx_data_bytes == 0) {
	    cc2420->CCA_pin = 0xFF;
	}
	else {
	    cc2420->CCA_pin = 0x00;
	}
	break;
    
    case CC2420_CCA_MODE_BOTH :
	/* valid data first */
	if (cc2420->rx_data_bytes != 0) {
	    cc2420->CCA_pin = 0x00;
	    break;
	}
	/* and check threshold */
	if (!cc2420->rx_rssi_valid) {
	    cc2420->CCA_pin = 0x00;
	    break;
	}
	if (cc2420->CCA_pin == 0x00) {
	    if (cc2420->rx_rssi_value > cca_threshold)
		cc2420->CCA_pin = 0xFF;
	}
	else if (cc2420->CCA_pin == 0xFF) {
	    if (cc2420->rx_rssi_value <= (cca_threshold - cca_hyst) ) {
		cc2420->CCA_pin = 0x00;
	    }
	}
	break;

    default : 
	CC2420_DEBUG("cc2420_record_rssi : bad CCA mode %d\n", cca_mode);
	cc2420->CCA_pin = 0xFF;
	break;
    }
	
    if (cc2420->CCA_pin != old_CCA_pin) {
	cc2420->CCA_set = 1;
    }

    return;
}


/**
 * check if CCA is OK
 * returns 0 if OK, -1 else
 */

int cc2420_check_cca(struct _cc2420_t * cc2420) {

    /* we just read the value of CCA_pin since CCA is calculated at RX time with RSSI */
    if (cc2420->CCA_pin)
	return 0;

    return -1;
}


/**
 * check if we can receive bytes or not
 * todo : check modulation (see with Guillaume what are the values of each modulation)
 * todo : SNR
 */

int cc2420_rx_filter(struct _cc2420_t * cc2420, double frequency, int modulation,
		     double dBm, double UNUSED snr) 
{
  double freq_cc;

  /* used to calculate the time between reception of two symbols */
  int64_t sync_delta;

  /* if we're not in RX mode, drop byte */
  if ( (cc2420->fsm_state != CC2420_STATE_RX_SFD_SEARCH) &&
       (cc2420->fsm_state != CC2420_STATE_RX_FRAME) ) 
    {
      CC2420_DBG_RX("cc2420:rx: dropping received data, not in Rx mode\n");
      return -1;
    }


  /* check sensitivity */
  if ( dBm < -95) 
    {
      CC2420_DBG_RX("cc2420:rx: dropping received data, below sensibility\n");
      return -1;
    }

  /* check frequency */
#define FREQ_FILTER_THRESHOLD 1.0

  freq_cc = cc2420_get_frequency_mhz(cc2420);
  if ((freq_cc - frequency > FREQ_FILTER_THRESHOLD) || 
      (frequency - freq_cc > FREQ_FILTER_THRESHOLD)) /* fabs */
    {
      CC2420_DBG_RX("cc2420:rx: dropping received data, wrong frequency\n");
      return -1;
    }

  /* Verify cc1100 modulation */
  if (cc2420_get_modulation(cc2420) != modulation) 
    {
      CC2420_DBG_RX("cc2420:rx: dropping received data, wrong modulation\n");
      return -1;
    }

  /* record RSSI twice since we have two bytes / symbol */
  cc2420_record_rssi(cc2420, dBm);
  cc2420_record_rssi(cc2420, dBm);

  /* if not 1st byte of frame check synchronisation */
  /* todo : check sync_delta limit values */
  if (cc2420->rx_data_bytes > 0) 
    {
      sync_delta = MACHINE_TIME_GET_NANO() - cc2420->rx_sync_timer;
      if (sync_delta < -33333) 
	{
	  CC2420_DEBUG("cc2420_callback_rx : bad sync, got byte too early : sync_delta is %"PRId64"d\n", sync_delta);
	  CC2420_RX_SFD_SEARCH_ENTER(cc2420);
	  return -1;
	}
      if (sync_delta > 33333) {
	CC2420_DEBUG("cc2420_callback_rx : bad sync, got byte too late\n");
	CC2420_RX_SFD_SEARCH_ENTER(cc2420);
	return -1;
      }
    }
    
  cc2420->rx_sync_timer = MACHINE_TIME_GET_NANO() + 2 * CC2420_SYMBOL_PERIOD;
  
  return 0;
}


/**
 * calculate the number of required zero symbols during synchronisation
 * cf [1] p 37
 */

int cc2420_sync_zero_symbols(struct _cc2420_t * cc2420) {
    uint16_t syncword;
    uint8_t  zero_symbols = 1;

    syncword = cc2420->registers[CC2420_REG_SYNCWORD];

    if (syncword & 0x00F0) {
	zero_symbols ++;
	if (syncword & 0x000F)
	    zero_symbols ++;
    }

    return zero_symbols;
}


/**
 * check frame crc
 */

int cc2420_rx_check_crc(struct _cc2420_t * cc2420) {
    uint16_t fcs;
    uint16_t received_fcs;
    uint8_t  buffer[CC2420_RAM_RXFIFO_LEN];

    /* copy last received frame to a buffer to compute and check crc */
    cc2420_rx_fifo_get_buffer(cc2420, cc2420->rx_frame_start + 1, buffer, cc2420->rx_len);

    if (cc2420->rx_len - 2 < 0) {
	CC2420_DEBUG("invalid len %d, can't check crc\n", cc2420->rx_len);
	return -1;
    }

    /* calculate fcs */
    fcs = cc2420_icrc(buffer, cc2420->rx_len - 2);

    /* get fcs received within frame */
    received_fcs = *((uint16_t *)(buffer + cc2420->rx_len - 2));

    /* and compare them */
    if (fcs != received_fcs) {
	CC2420_DEBUG("cc2420_rx_check_frame : bad crc\n");
	/* todo : set crc status bit */
	return -1;
    }

    return 0;
}


/**
 * swap bits within a byte
 */

uint8_t swapbyte(uint8_t c) {
    uint8_t result=0;
    int     i;
 
    for(i = 0; i < 8; i++) {
	result  = result << 1;
	result |= (c & 1);
	c = c >> 1;
    }
    return result;
}


/**
 * process frame control field to determine addressing modes etc...
 */

int cc2420_rx_process_fcf(struct _cc2420_t * cc2420) {

    CC2420_DEBUG("got fcf %.2x\n", cc2420->rx_fcf);

    cc2420->rx_frame_type    = CC2420_FCF_FRAME_TYPE   (cc2420->rx_fcf);
    cc2420->rx_sec_enabled   = CC2420_FCF_SEC_ENABLED  (cc2420->rx_fcf);
    cc2420->rx_frame_pending = CC2420_FCF_FRAME_PENDING(cc2420->rx_fcf);
    cc2420->rx_ack_req       = CC2420_FCF_ACK_REQUEST  (cc2420->rx_fcf);
    cc2420->rx_intra_pan     = CC2420_FCF_INTRA_PAN    (cc2420->rx_fcf);
    cc2420->rx_dst_addr_mode = CC2420_FCF_DST_ADDR_MODE(cc2420->rx_fcf);
    cc2420->rx_src_addr_mode = CC2420_FCF_SRC_ADDR_MODE(cc2420->rx_fcf);

    CC2420_DEBUG("ack_req is %d\n", cc2420->rx_ack_req);

    /* process addressing modes to determine addresses lengths and positions */

    switch (cc2420->rx_dst_addr_mode) {
    case CC2420_ADDR_MODE_EMPTY :
	cc2420->rx_dst_pan_len     = 0;
	cc2420->rx_dst_addr_len    = 0;
	break;
    case CC2420_ADDR_MODE_RESERVED :
	cc2420->rx_dst_pan_len     = 0;
	cc2420->rx_dst_addr_len    = 0;
	CC2420_DEBUG("cc2420_process_fcf : using reserved addressing mode !\n");
	break;
    case CC2420_ADDR_MODE_16_BITS :
	cc2420->rx_dst_pan_len     = 2;
	cc2420->rx_dst_addr_len    = 2;
	break;
    case CC2420_ADDR_MODE_64_BITS :
	cc2420->rx_dst_pan_len     = 2;
	cc2420->rx_dst_addr_len    = 8;
	break;
    }

    switch (cc2420->rx_src_addr_mode) {
    case CC2420_ADDR_MODE_EMPTY :
	cc2420->rx_src_pan_len     = 0;
	cc2420->rx_src_addr_len    = 0;
	break;
    case CC2420_ADDR_MODE_RESERVED :
	cc2420->rx_src_pan_len     = 0;
	cc2420->rx_src_addr_len    = 0;
	CC2420_DEBUG("cc2420_process_fcf : using reserved addressing mode !\n");
	break;
    case CC2420_ADDR_MODE_16_BITS :
	cc2420->rx_src_pan_len     = 2;
	cc2420->rx_src_addr_len    = 2;
	break;
    case CC2420_ADDR_MODE_64_BITS :
	cc2420->rx_src_pan_len     = 2;
	cc2420->rx_src_addr_len    = 8;
	break;
    }

//    cc2420->rx_dst_pan_offset  = 3; /* 3 for length field and FCF */
    cc2420->rx_dst_pan_offset  = 4; /* 4 for length field and FCF ( ??? ) */
    cc2420->rx_dst_addr_offset = cc2420->rx_dst_pan_offset  + cc2420->rx_dst_pan_len;
    cc2420->rx_src_pan_offset  = cc2420->rx_dst_addr_offset + cc2420->rx_dst_addr_len;
    cc2420->rx_src_addr_offset = cc2420->rx_src_pan_offset  + cc2420->rx_src_pan_len;
    
    CC2420_DEBUG("in process_fcf, dst addr len is %d/%d, src addr len is %d/%d\n", cc2420->rx_src_addr_len, cc2420->rx_src_pan_len, 
			 cc2420->rx_dst_addr_len, cc2420->rx_dst_pan_len);

    return 0;
}


/**
 * print a buffer, for debug
 */

void print_buf(char * msg, uint8_t * buf, uint8_t len) {

    char debug_str[256];
    char tmp      [256];
    int  i;

    strcpy(debug_str, msg);
    for (i = 0; i < len; i++) {
	sprintf(tmp, "%.2x:", buf[i]);
	strcat(debug_str, tmp);
    }
    strcat(debug_str, "\n");
    CC2420_DEBUG(debug_str);
}


/**
 * check address recognition (nutshell)
 * returns 0 if address test passes, -1 otherwise
 */

int cc2420_rx_check_address(struct _cc2420_t * cc2420 UNUSED) {
    
//#define CC2420_RX_CHECK_ADDRESS_TEST
#ifdef  CC2420_RX_CHECK_ADDRESS_TEST
    /* for test, refuse half of packets */
    static int i = 0;
    if (i) {
	/* refuse address */
	i = 0;
	return 1;
    }
    else {
	i = 1;
	return 0;
    }
#else

    /* TODO : implement here different address recognition modes */
    /* calculate size corresponding to addresses */

    uint8_t addr_len = cc2420->rx_src_addr_offset + cc2420->rx_src_addr_len - cc2420->rx_dst_pan_offset;
    CC2420_DEBUG("address len is %d\n", addr_len);

    CC2420_DEBUG("in check_address, dst addr len is %d/%d, src addr len is %d/%d\n", cc2420->rx_src_addr_len, cc2420->rx_src_pan_len, 
			 cc2420->rx_dst_addr_len, cc2420->rx_dst_pan_len);


    if (addr_len == 0) {
	CC2420_DEBUG("no address, check what to do here\n");
	return 0;
    }

    uint8_t buffer[256];
    
    /* get buffer corresponding to src and dst addresses */
    cc2420_rx_fifo_get_buffer(cc2420, cc2420->rx_dst_pan_offset, buffer, addr_len);

    uint8_t offset = 0;

    uint8_t * ptr = &buffer[0];

    uint8_t src_addr[8];
    uint8_t src_pan [2];
    uint8_t dst_addr[8];
    uint8_t dst_pan [2];

    uint8_t broadcast_addr [8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (cc2420->rx_src_pan_len > 0) {
	memcpy(src_pan, ptr + offset, cc2420->rx_src_pan_len);
	offset += cc2420->rx_src_pan_len;
	print_buf("src_pan  is ", src_pan, cc2420->rx_src_pan_len);
    }

    if (cc2420->rx_src_addr_len > 0) {
	memcpy(src_addr, ptr + offset, cc2420->rx_src_addr_len);
	offset += cc2420->rx_src_addr_len;
	print_buf("src_addr is ", src_addr, cc2420->rx_src_addr_len);
    }

    if (cc2420->rx_dst_pan_len > 0) {
	memcpy(dst_pan, ptr + offset, cc2420->rx_dst_pan_len);
	offset += cc2420->rx_dst_pan_len;
	print_buf("dst_pan  is ", dst_pan, cc2420->rx_dst_pan_len);
    }

    if (cc2420->rx_dst_addr_len > 0) {
	memcpy(dst_addr, ptr + offset, cc2420->rx_dst_addr_len);
	offset += cc2420->rx_dst_addr_len;
	print_buf("dst_addr is ", dst_addr, cc2420->rx_dst_addr_len);
    }

    /* cf [1] p. 31, 41 and [2] p. 139 for address recognition */

    if (cc2420->rx_frame_type == CC2420_FRAME_TYPE_BEACON) {
        if (cc2420->rx_src_pan_len != 2) {
            CC2420_DEBUG("no src pan ID for a beacon frame, dropping\n");
            return -1;
        }
        /* get pan id */

        /* if pan id is 0xFFFF accept any source pan id */
        if (!memcmp(cc2420->ram + CC2420_RAM_PANID, broadcast_addr, 2)) {
            CC2420_DEBUG("local pan id is 0xFFFF, won't check src pan id\n");
        }
        else {
            if (memcmp(cc2420->ram + CC2420_RAM_PANID, src_pan, 2)) {
                CC2420_DEBUG("local pan id and src pan id are different on a beacon frame, dropping\n");
                return -1;
            }
        }
    }

    /* check dst pan id */
    if (cc2420->rx_dst_pan_len > 0) {
        /* if not broadcast */
        if (!memcmp(dst_pan, broadcast_addr, 2)) {
            CC2420_DEBUG("dst pan id is broadcast, not checking\n");
        }
        else {
            if (memcmp(cc2420->ram + CC2420_RAM_PANID, src_pan, 2)) {
                CC2420_DEBUG("local pan id and dst pan id are different, dropping\n");
                return -1;
            }
        }
    }

    /* check short dst address */
    if (cc2420->rx_dst_addr_len == 2) {
        /* if not broadcast */
        if (!memcmp(cc2420->rx_dst_addr, broadcast_addr, 2)) {
            CC2420_DEBUG("dst short address is broadcast, not checking\n");
        }
        else {
            if (memcmp(cc2420->ram + CC2420_RAM_SHORTADR, dst_addr, 2)) {
                CC2420_DEBUG("dst short address and local short addresses are different, dropping\n");
                return -1;
            }
        }
    }

    /* check extended dst address */
    else if (cc2420->rx_dst_addr_len == 8) {
        /* if not broadcast */
        if (!memcmp(cc2420->rx_dst_addr, broadcast_addr, 8)) {
            CC2420_DEBUG("dst short address is broadcast, not checking\n");
        }
        else {
            if (memcmp(cc2420->ram + CC2420_RAM_IEEEADR, dst_addr, 8)) {
                CC2420_DEBUG("dst short address and local extended addresses are different, dropping\n");
                return -1;
            }
        }
    }

    /* if there only source addresses, check if we are coordinator and pan ID */
    if ( (cc2420->rx_dst_pan_len == 0) && (cc2420->rx_dst_addr_len == 0) ) {
        if (!CC2420_REG_MDMCTRL0_PAN_COORDINATOR(cc2420->registers[CC2420_REG_MDMCTRL0])) {
            CC2420_DEBUG("only source addressing fields, and I'm not a coordinator, dropping\n");
            return -1;
        }
        /* if there is no pan id, drop */
        if (cc2420->rx_src_pan_len == 0) {
            CC2420_DEBUG("only source addressing fields, but no src pan id, dropping\n");
            return -1;
        }
        /* if source pan id doesn't match , drop */
        if (memcmp(cc2420->ram + CC2420_RAM_PANID, src_pan, 2)) {
            CC2420_DEBUG("only source addressing fields, but bad pan id, dropping\n");
            return -1;
        }
    }

    return 0;
#endif
}


/**
 * flush current rx frame
 */

void cc2420_rx_flush_current_frame(struct _cc2420_t * cc2420) {
    
    /* reset write pointer to the first byte of the current rx frame */
    cc2420->rx_fifo_write = cc2420->rx_frame_start;
    
    return;
}


/**
 * rx callback
 * todo : autoack
 */

uint64_t cc2420_callback_rx(void *arg, struct wsnet_rx_info *wrx)
{
  struct _cc2420_t *cc2420 = (struct _cc2420_t *) arg;

  uint8_t rx       = wrx->data;;
  double frequency = wrx->freq_mhz;
  int modulation   = wrx->modulation;
  double dBm       = wrx->power_dbm;
  double snr       = wrx->SiNR;

    /* check if we are able to receive data */
    if (cc2420_rx_filter(cc2420, frequency, modulation, dBm, snr)) {
	return 0;
    }

    uint16_t addr_decode = CC2420_REG_MDMCTRL0_ADR_DECODE(cc2420->registers[CC2420_REG_MDMCTRL0]);

    /* according to current state, deal with data */
    switch (cc2420->fsm_state) {
    case CC2420_STATE_RX_SFD_SEARCH :
	/* one more sync byte */
	if (rx == 0) {
	    cc2420->rx_zero_symbols += 2;
	    return 0;
	}

	/* byte is not 0, and we got enough sync zeros : compare received byte with expected syncword */
	if (cc2420->rx_zero_symbols >= cc2420_sync_zero_symbols(cc2420) ) {
	    if (rx == CC2420_HIBYTE(cc2420->registers[CC2420_REG_SYNCWORD]) ) {
		CC2420_RX_FRAME_ENTER(cc2420);
		return 0;
	    }
	}

	CC2420_DEBUG("cc2420_callback_rx : non 0 byte not expected, dropping\n");

	/* not a zero, not syncword -> drop and resynchronize */
	CC2420_RX_SFD_SEARCH_ENTER(cc2420);
	return 0;

    case CC2420_STATE_RX_FRAME :
	/* we are synchronized, receive data */

	/* if this is the first RX byte, it is length field */
	if (cc2420->rx_data_bytes == 0) {
	    /* just keep the 7 low bits */
	    cc2420->rx_len = CC2420_TX_LEN_FIELD(rx);
	    CC2420_DEBUG("got rx_len %d\n", cc2420->rx_len);

	    /* set FIFO pin since we have data in fifo */
	    cc2420->FIFO_pin    = 0xFF;
	    cc2420->FIFO_set    = 1;
	}

	/* update number of received bytes */
	cc2420->rx_data_bytes ++;

	/* write byte to RX FIFO */
	if (cc2420_rx_fifo_push(cc2420, rx) < 0) {
	    CC2420_DEBUG("cc2420_callback_rx : failed in push\n");
	    CC2420_RX_OVERFLOW_ENTER(cc2420);
	    cc2420->FIFOP_pin = 0xFF;
	    cc2420->FIFOP_set = 1;
	    cc2420->FIFO_pin  = 0x00;
	    cc2420->FIFO_set  = 1;
	    return 0;
	}

	/* first byte of FCF */
	if (cc2420->rx_data_bytes == 2) {
	    CC2420_DEBUG("1st byte of fcf is %.1x, swapped %.1x\n", rx, swapbyte(rx));
	    CC2420_DEBUG("swapping byte, todo : check that it's not a bug in cc2420 in cc2420.c/com_send driver\n");
	    cc2420->rx_fcf = swapbyte(rx);
	}

	/* second byte of FCF */
	if (cc2420->rx_data_bytes == 3) {
	    CC2420_DEBUG("2nd byte of fcf is %.1x, swapped %.1x\n", rx, swapbyte(rx));
	    cc2420->rx_fcf |= swapbyte(rx) << 8;
	    /* the FCF was fully received, process it */
	    cc2420_rx_process_fcf(cc2420);
	}

	/* sequence field */
	if (cc2420->rx_data_bytes == 4) {
	    CC2420_DEBUG("seq is %d\n", rx);
	}

	/* if first byte of data, set position, needed to unset FIFOP */
	if (cc2420->rx_data_bytes == 4) {
	    /* if we don't already have data bytes in RX FIFO */
	    if (cc2420->rx_first_data_byte == -1)
		cc2420->rx_first_data_byte = cc2420->rx_fifo_write;
	}

	/* if address recognition is set, and addressing fields were received, deal with address recognition */
	if (addr_decode) {
	    if (cc2420->rx_data_bytes == cc2420->rx_src_addr_offset + cc2420->rx_src_addr_len) {
		CC2420_DEBUG("got last addressing byte, data bytes is %d\n", cc2420->rx_data_bytes);
		/* if address checking recognition fails, set flag to indicate that frame has to be flushed */
		if (cc2420_rx_check_address(cc2420)) {
		    CC2420_DEBUG("address recognition failed, will flush received frame when complete\n");
		    cc2420->rx_addr_decode_failed = 1;
		    return 0;
		}
		else {
                    uint8_t rx_threshold = CC2420_REG_IOCFG0_FIFOP_THR(cc2420->registers[CC2420_REG_MDMCTRL0]);
                    CC2420_DEBUG("address recognition OK, checking threshold (%d)\n", rx_threshold);
                    if (cc2420->rx_data_bytes >= rx_threshold) {
                        CC2420_DEBUG("rx_threshold reached, setting up FIFOP\n");
                        cc2420->FIFOP_pin = 0xFF;
                        cc2420->FIFOP_set =    1;
                    }
		}
	    }
	}

	/* if we got a complete frame, (+1 for length field) */
	if ( cc2420->rx_data_bytes == (cc2420->rx_len + 1) ) {

	    /* check that received frame is ok, ie crc and address recognition 
	       else flush the frame */
	    if (cc2420_rx_check_crc(cc2420)) {
		CC2420_DEBUG("crc check failed, flushing received frame\n");
		cc2420_rx_flush_current_frame(cc2420);
		CC2420_RX_SFD_SEARCH_ENTER(cc2420);
		return 0;
	    }

            if (cc2420->rx_addr_decode_failed) {
                CC2420_DEBUG("at end of frame, address recog was bad, flushing frame\n");
                cc2420_rx_flush_current_frame(cc2420);
                CC2420_RX_SFD_SEARCH_ENTER(cc2420);
                return 0;
            }

	    /* update number of fully received frames in RX FIFO */
	    cc2420->nb_rx_frames ++;

	    CC2420_DEBUG("cc2420 : got a new frame, nb_rx_frames is now %d\n", cc2420->nb_rx_frames);

	    /* if this is the only frame in fifo, calculate rx_frame_end */
	    /* rx_frame_end will be used in rx_fifo_pop to update the state of FIFOP */
	    if (cc2420->nb_rx_frames == 1) {
		cc2420->rx_frame_end = cc2420->rx_fifo_write - 1;
		if (cc2420->rx_frame_end < 0) {
		    cc2420->rx_frame_end += CC2420_RAM_RXFIFO_LEN;
		}
	    }

	    /* we are at the end of a new frame, if address recognition is disabled set FIFOP */

	    cc2420->FIFOP_pin = 0xFF;
	    cc2420->FIFOP_set = 1;

            /* if ack is requested */
            if (cc2420->rx_ack_req) {
                CC2420_TX_ACK_CALIBRATE_ENTER(cc2420);
                return 0;
            }

	    CC2420_RX_SFD_SEARCH_ENTER(cc2420);
	    return 0;
	}
	
	break;

    default :
	CC2420_DEBUG("cc2420_callback_rx : bad state for rx\n");
	return 0;
    }

    return 0;
}