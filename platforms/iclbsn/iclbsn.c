
/**
 *  \file   iclbsn.c
 *  \brief  Platform devices handling functions, ICL BSN nodes
 *  \author Antoine Fraboulet
 *  \date   2007
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch/common/hardware.h"
#include "arch/msp430/msp430.h"

#include "devices/devices.h"
#include "devices/led/led_dev.h"
#include "devices/at45db/at45db_dev.h"
#include "devices/cc2420/cc2420_dev.h"
#include "devices/ptty/ptty_dev.h"
#include "src/options.h"

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/* ***************************************
 * platform description for ICLBSN devices
 *
 *
 * red         @31   1 << 4      P5
 * green       @31   1 << 5      P5
 * yellow      @31   1 << 6      P5
 *
 * flash WR    @1D   1 << 3      P4
 * flash CS    @1D   1 << 4      P4
 *
 * radio
 *   reset     @1D   1 << 6      P4
 *   vref      @1D   1 << 5      P4
 *   csn       @1D   1 << 2      P4
 *   fifop     @22   1 << 0      P1
 *   gio0      @22   1 << 3      P1
 *   sfd       @1E   1 << 1      P4
 *   gio1      @22   1 << 4      P1
 *
 *
 *
 * Port 1
 * ======
 *   1.7 : Hum_pwr
 *   1.6 : Hum_scl
 *   1.5 : Hum_sda
 *   1.4 : in, cc2420 radio_gio1
 *   1.3 : in, cc2420 radio_gio0
 *   1.2 : p_dvcc
 *   1.1 : UART1TX loopback, bsltx
 *   1.0 : in, cc2420 pkt_in fifop
 *
 * Port 2
 * ======
 *   2.7 : ??
 *   2.6 : ??
 *   2.5 : ??
 *   2.4 : ??
 *   2.3 : ??  // CC2420, Gio2
 *   2.2 : ??  // ta0, caout,  + UART1RX loopback (bslrx)
 *   2.1 : ??
 *   2.0 : ??
 *
 * Port 3
 * ======
 *   3.7 : uart1RX
 *   3.6 : uart1TX
 *   3.5 : uart0RX
 *   3.4 : uart0TX
 *   3.3 : uclk0, serial clock, Radio_sclk
 *   3.2 : simo0, Radio_Slave_0ut
 *   3.1 : somi0, Radio_Slave_In
 *   3.0 : ste0 : NC
 *
 * Port 4
 * ======
 *   4.7 : ??
 *   4.6 : out, radio_reset
 *   4.5 : out, radio_vref_en
 *   4.4 : out, flash CS
 *   4.3 : ?? , flash PWR
 *   4.2 : out, radio_cs
 *   4.1 : in,  radio_sfd
 *   4.0 : NC
 *
 * Port 5
 * ======
 *   5.7 : SVSout (unused)
 *   5.6 : LED3  Yellow
 *   5.5 : LED2  Green
 *   5.4 : LED1  Red
 *   5.3 : NC
 *   5.2 : NC
 *   5.1 : NC
 *   5.0 : NC
 *
 * Port 6
 * ======
 *   6.7 : ADC7, SVSin
 *   6.6 : ADC6, DAC0
 *   6.5 : ADC5
 *   6.4 : ADC4
 *   6.3 : ADC3
 *   6.2 : ADC2
 *   6.1 : ADC1
 *   6.0 : ADC0
 *
 * ***************************************/

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#define SYSTEM           0
#define LED1             1
#define LED2             2
#define LED3             3
#define FLASH            4
#define RADIO            5
#define SERIAL           6

#define END_DEV      SERIAL

#define BOARD_DEVICE_MAX (END_DEV+1)

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static struct moption_t xt1_opt = {
  .longname    = "xin",
  .type        = required_argument,
  .helpstring  = "xin crystal freq (Hz)",
  .value       = NULL
};

/* xt2 is not used on iclbsn */
/*
 * static struct moption_t xt2_opt = {
 *   .longname    = "xt2",
 *   .type        = required_argument,
 *   .helpstring  = "xt2 crystal freq (Hz)",
 *   .value       = NULL
 * };
 */

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int devices_options_add(void)
{
  options_add(&xt1_opt             );

  at45db_add_options (FLASH,  0, "flash"   );
  ptty_add_options   (SERIAL, 1, "serial1" );

  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

struct iclbsn_struct_t {
  int flash_cs;
  int radio_cs;
};

#define SYSTEM_DATA      ((struct iclbsn_struct_t*)(machine.device[SYSTEM].data))
#define SYSTEM_FLASH_CS  (SYSTEM_DATA->flash_cs)
#define SYSTEM_RADIO_CS  (SYSTEM_DATA->radio_cs)

int system_reset (int UNUSED dev) 
{ 
  return 0; 
}

int system_delete(int UNUSED dev) 
{ 
  return 0; 
}

int system_create(int dev_num)
{
  machine.device[dev_num].reset         = system_reset;
  machine.device[dev_num].delete        = system_delete;
  machine.device[dev_num].state_size    = sizeof(struct iclbsn_struct_t);
  machine.device[dev_num].name          = "System Platform";
  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int devices_create(void)
{
  int res = 0;
  int xin_freq, xt2_freq;

  xin_freq = 32768;
  xt2_freq = 0; /* xt2 is not used on iclbsn */

  if (xt1_opt.value) {
    xin_freq = atoi(xt1_opt.value);
    HW_DMSG_CLOCK("iclbsn: xt1 external crystal set to %d Hz\n",xin_freq);
  }

  /*********************************/
  /* MSP430 MCU                    */
  /*********************************/

  res += msp430_mcu_create(xin_freq, xt2_freq);

  /*********************************/
  /* fix peripheral sizes          */
  /*********************************/

  machine.device_max          = BOARD_DEVICE_MAX;
  machine.device_size[SYSTEM] = sizeof(struct iclbsn_struct_t);
  machine.device_size[LED1]   = led_device_size();       // Led1
  machine.device_size[LED2]   = led_device_size();       // Led2
  machine.device_size[LED3]   = led_device_size();       // Led3
  machine.device_size[FLASH]  = at45db_device_size();    // Flash RAM
  machine.device_size[RADIO]  = cc2420_device_size();    // cc2420 radio
  machine.device_size[SERIAL] = ptty_device_size();

  /*********************************/
  /* allocate memory               */
  /*********************************/

  res += devices_memory_allocate();

  /*********************************/
  /* create peripherals            */
  /*********************************/

  res += system_create          (SYSTEM);
  res += led_device_create      (LED1,0xee, 0,    0); // red
  res += led_device_create      (LED2,0,    0xee, 0); // green 
  res += led_device_create      (LED3,0xee, 0xee, 0); // yellow
  res += at45db_device_create   (FLASH,  0);
  res += cc2420_device_create   (RADIO, 16);
  res += ptty_device_create     (SERIAL, 1);

  /*********************************/
  /* place peripherals Gui         */
  /*********************************/

  {
    int lw,lh;

    machine.device[LED1].ui_get_size(LED1,&lw,&lh);

    machine.device[LED1].ui_set_pos(LED1,    0,   0);
    machine.device[LED2].ui_set_pos(LED2, 1*lw,   0);
    machine.device[LED3].ui_set_pos(LED3, 2*lw,   0);
  }

  /*********************************/
  /* end of platform specific part */
  /*********************************/

  tracer_event_add_id(TRACER_LED1,  1, "led1"  , "");
  tracer_event_add_id(TRACER_LED2,  1, "led2"  , "");
  tracer_event_add_id(TRACER_LED3,  1, "led3"  , "");

  return res;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/* this function is called after devices reset    */
/* devices init conditions should be written here */
int devices_reset_post()
{
  /* flash W~ is set to Vcc */
  machine.device[FLASH].write(FLASH, AT45DB_W, AT45DB_W);
  SYSTEM_FLASH_CS = 0;
  SYSTEM_RADIO_CS = 0;
  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int devices_update()
{
  int res       = 0;
  uint8_t  val8 = 0;
  int refresh   = 0;

  /* *************************************************************************** */
  /* MCU -> devices                                                              */
  /* *************************************************************************** */

  /* Port 1
   * ======
   *   1.7 : sensor_pwr
   *   1.6 : sensor_scl
   *   1.5 : sensor_sda
   *   1.4 : in, cc2420 radio_gio1
   *   1.3 : in, cc2420 radio_gio0
   *   1.2 : p_dvcc
   *   1.1 : UART1TX
   *   1.0 : in, cc2420 pkt_in
   */
   if (msp430_digiIO_dev_read(PORT1,&val8))
    {
      HW_DMSG("iclbsn: writing on port 1 byte 0x%02x\n",val8 & 0xff);
      // machine.device[FLASH].write(FLASH, AT45DB_W , (BIT(val8,2) << AT45DB_W_SHIFT));
    }

  /* port 2 :
   * ========
   *   2.7 : xx
   *   2.6 : xx
   *   2.5 : xx
   *   2.4 : xx
   *   2.3 : xx
   *   2.2 : Uart1 Rx
   *   2.1 : GIO1 ??
   *   2.0 : GIO0 ??
   */
  if (msp430_digiIO_dev_read(PORT2,&val8))
    {
      HW_DMSG("iclbsn: writing on port 2 byte 0x%02x\n",val8 & 0xff);
    }


  /* port 3 :
   * ========
   *   3.7 : uart1RX
   *   3.6 : uart1TX
   *   3.5 : uart0RX
   *   3.4 : uart0TX
   *   3.3 : uclk0, Radio_sclk
   *   3.2 : simo0, Radio_Slave_0ut
   *   3.1 : somi0, Radio_Slave_In
   *   3.0 : ste0 : NC
   */
  if (msp430_digiIO_dev_read(PORT3,&val8))
    {
      HW_DMSG("iclbsn: writing on port 3 byte 0x%02x\n",val8 & 0xff); 
      /* cc2420 is driven by spi                          */
      /* we could/should check here that the pins are not */
      /* driven by the GPIO                               */
    }

  /* port 4 :
   * ========
   *   4.7 : xx
   *   4.6 : out, radio_reset
   *   4.5 : out, radio_vref_en
   *   4.4 : out, flash CS
   *   4.3 : out, flash PWR
   *   4.2 : out, radio_cs
   *   4.1 : in,  radio_sfd
   *   4.0 : xx
   */
  if (msp430_digiIO_dev_read(PORT4,&val8))
    {
      HW_DMSG("iclbsn: writing on port 4 byte 0x%02x\n",val8 & 0xff);

#if defined(DEBUG)
      if (BIT(val8,4) != SYSTEM_FLASH_CS)
	{
	  HW_DMSG_DEV("iclbsn: flash CS set to %d\n",BIT(val8,4));
	}
      if (BIT(val8,2) != SYSTEM_RADIO_CS)
	{
	  HW_DMSG_DEV("iclbsn: cc2420 CS set to %d\n",BIT(val8,2));
	}
#endif

      machine.device[FLASH].write(FLASH, AT45DB_S | AT45DB_W, 
				  (BIT(val8,3) << AT45DB_W_SHIFT) |
				  (BIT(val8,4) << AT45DB_S_SHIFT));


      SYSTEM_FLASH_CS = BIT(val8,4);
      /* waiting for flash update */
      
      /* cc2420 CS out, SFD in */
      machine.device[RADIO].write(RADIO, CC2420_CSn_MASK | CC2420_RESET_MASK | CC2420_VREG_EN_MASK,
				  (BIT(val8,2) << CC2420_BIT_CSn)      |
	                          (BIT(val8,5) << CC2420_BIT_VREG_EN) |
				  (BIT(val8,6) << CC2420_BIT_RESET));
      SYSTEM_RADIO_CS = BIT(val8,2);
      /* waiting for cc2420 update */
    }

  /* port 5 :
   * ========
   *   5.7 : SVSout (unused)
   *   5.6 : LED3
   *   5.5 : LED2
   *   5.4 : LED1
   *   5.3 : NC
   *   5.2 : NC
   *   5.1 : NC
   *   5.0 : NC
   */
  if (msp430_digiIO_dev_read(PORT5,&val8))
    {
      machine.device[LED1].write(LED1,LED_DATA, ! BIT(val8,4));
      tracer_event_record(TRACER_LED1,!BIT(val8,4));
      UPDATE(LED1);
      REFRESH(LED1);

      machine.device[LED2].write(LED2,LED_DATA, !  BIT(val8,5));
      tracer_event_record(TRACER_LED2,!BIT(val8,5));
      UPDATE(LED2);
      REFRESH(LED2);

      machine.device[LED3].write(LED3,LED_DATA, ! BIT(val8,6));
      tracer_event_record(TRACER_LED3,!BIT(val8,6));
      UPDATE(LED3);
      REFRESH(LED3);
    }

  /* port 6 :
   * ========
   *   6.7 : VCCin
   *   6.6 : xx
   *   6.5 : ADC5
   *   6.4 : ADC4
   *   6.3 : ADC3
   *   6.2 : ADC2
   *   6.1 : ADC1
   *   6.0 : ADC0
   */
  if (msp430_digiIO_dev_read(PORT6,&val8))
    {
      HW_DMSG("iclbsn: writing on port 6 byte 0x%02x\n",val8 & 0xff);
    }

  /* Usart SPI mode                    */
  /* ==============                    */
  /* USART 0 : radio + flash           */

  switch (MCU.usart0.mode)
    {
    case USART_MODE_SPI:
      if (msp430_usart0_dev_read_spi(&val8))
	{
	  /*
	   * FLASH_CS == 0 -- flash on
	   * RADIO_CS == 0 -- radio on
	   */
	  if ((SYSTEM_FLASH_CS == 0) && (SYSTEM_RADIO_CS == 0))
	    {
	      WARNING("devices:iclbsn: Flash and Radio SPI are selected at the same time\n");
	    }

	  /* tests can/should be removed */
	  // if (SYSTEM_FLASH_CS == 0) /* AT45DB CS is negated */
	    {
	      machine.device[FLASH].write(FLASH, AT45DB_D, val8);
	    }

	  /* tests can/should be removed */
	  // if (SYSTEM_RADIO_CS == 0) /* CC2420 CS is negated */
	    {
	      machine.device[RADIO].write(RADIO, CC2420_DATA_MASK, val8);
	    }
	}
      break;
    case USART_MODE_UART:
      /* Uart0 on expansion port U28 */
      break;
    case USART_MODE_I2C:
      break;
    }

  /* Usart1                            */
  /* Uart : serial I/O                 */
  /*                                   */
  /*                                   */
  switch (MCU.usart1.mode)
    {
    case USART_MODE_SPI:
      break;
    case USART_MODE_UART:
      if (msp430_usart1_dev_read_uart(&val8))
	{
	  machine.device[SERIAL].write(SERIAL, PTTY_D, val8);
	}
      break;
    case USART_MODE_I2C:
      break;
    }

  /* *************************************************************************** */
  /* devices -> MCU                                                              */
  /* *************************************************************************** */

  /* input on radio */
  {
    uint32_t mask  = 0;
    uint32_t value = 0;
    machine.device[ RADIO ].read( RADIO ,&mask,&value);
    if ((mask & CC2420_DATA_MASK) != 0)
      {
	if (MCU.usart0.mode != USART_MODE_SPI)
	  {
	    ERROR("devices: read data on CC2420 radio while not in SPI mode ?\n");
	  }
	msp430_usart0_dev_write_spi(value & CC2420_DATA_MASK);
	/* ERROR("device: radio write 0x%02x on SPI0 from radio\n",value & AT45DB_D); */
	// etracer_slot_access(0x0, 1, ETRACER_ACCESS_READ, ETRACER_ACCESS_BYTE, ETRACER_ACCESS_LVL_SPI0, 0);
      }

    // << RADIO_SFD     - CC2420_SFD   - P4.1
    // << RADIO_PKT_INT - CC2420_FIFOP - P1.0
    // << RADIO_GIO0    - CC2420_FIFO  - P1.3
    // << RADIO_GIO1    - CC2420_CCA   - P1.4

    if (mask & CC2420_SFD_MASK)   /* mcu::SFD     */
      { 
	msp430_digiIO_dev_write(PORT4, (value & CC2420_SFD_MASK)   ? 0x02 : 0x00, 0x02);
	// etracer_slot_access(0x0, 1, ETRACER_ACCESS_READ, ETRACER_ACCESS_BIT, ETRACER_ACCESS_LVL_GPIO, 0);
      }
    if (mask & CC2420_FIFOP_MASK) /* mcu::PKT_INT */
      { 
	msp430_digiIO_dev_write(PORT1, (value & CC2420_FIFOP_MASK) ? 0x01 : 0x00, 0x01);
	// etracer_slot_access(0x0, 1, ETRACER_ACCESS_READ, ETRACER_ACCESS_BIT, ETRACER_ACCESS_LVL_GPIO, 0);
      }
    if (mask & CC2420_FIFO_MASK)  /* mcu::GIO0    */
      {
	msp430_digiIO_dev_write(PORT1, (value & CC2420_FIFO_MASK)  ? 0x08 : 0x00, 0x08);
	// etracer_slot_access(0x0, 1, ETRACER_ACCESS_READ, ETRACER_ACCESS_BIT, ETRACER_ACCESS_LVL_GPIO, 0);
      }
    if (mask & CC2420_CCA_MASK)   /* mcu::GIO1    */
      {
	msp430_digiIO_dev_write(PORT1, (value & CC2420_CCA_MASK)   ? 0x10 : 0x00, 0x10);
	// etracer_slot_access(0x0, 1, ETRACER_ACCESS_READ, ETRACER_ACCESS_BIT, ETRACER_ACCESS_LVL_GPIO, 0);
      }
  }


  /* input on flash */
  {
    uint32_t mask  = 0;
    uint32_t value = 0;
    machine.device[ FLASH ].read( FLASH ,&mask,&value);
    if ((mask & AT45DB_D) != 0)
      {
	if (MCU.usart0.mode != USART_MODE_SPI)
	  {
	    ERROR("devices: read data on STM flash while not in SPI mode\n");
	  }
	if (SYSTEM_FLASH_CS != 0) /* push test above device_read ? */
	  {
	    ERROR("devices: read data on flash without CS enabled\n");
	  }
	msp430_usart0_dev_write_spi(value & AT45DB_D);
	/* ERROR("device: flash write 0x%02x on SPI0 from flash\n",value & AT45DB_D); */
	// etracer_slot_access(0x0, 1, ETRACER_ACCESS_READ, ETRACER_ACCESS_BYTE, ETRACER_ACCESS_LVL_SPI1, 0);
      }
  }

  /* input on UART serial */
  if (msp430_usart1_dev_write_uart_ok())
    {
      uint32_t mask,value;
      machine.device[SERIAL].read(SERIAL,&mask,&value);
      if ((mask & PTTY_D) != 0)
	{
	  msp430_usart1_dev_write_uart(value & PTTY_D);
	  // etracer_slot_access(0x0, 1, ETRACER_ACCESS_READ, ETRACER_ACCESS_BYTE, ETRACER_ACCESS_LVL_OUT, 0);
	}
    }


  /* *************************************************************************** */
  /* update                                                                      */
  /* *************************************************************************** */
  LIBSELECT_UPDATE();
  LIBWSNET_UPDATE();

  UPDATE(RADIO);
  UPDATE(FLASH);
  UPDATE(SERIAL);

  if (refresh) 
    {
      ui_refresh();
    }

  return res;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */