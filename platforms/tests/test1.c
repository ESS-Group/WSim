
/**
 *  \file   test1.c
 *  \brief  Platform devices handling functions, test1 platform
 *  \author Antoine Fraboulet
 *  \date   2005
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __DEBUG
#  undef DEBUG
#endif

#include "arch/common/hardware.h"
#include "arch/msp430/msp430.h"
#include "devices/devices.h"
#include "devices/led/led_dev.h"
#include "devices/ptty/ptty_dev.h"
#include "src/options.h"

#define UI_EVENT_SKIP (10*1000)



/* ****************************************
 * platform description 
 *
 * MCU MSP430f135
 *
 * Port 1: led array
 * ================
 *   8 leds
 *
 * Port 2: serial output
 * =====================
 *   stdio output of value sent to the port
 *
 * Port 3: 
 * =======
 *   3.0 asserting P3.1 halts the simulation 
 *         using MCU_SIGQUIT
 *
 * ***************************************/


/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#define LED1            0
#define LED2            1
#define LED3            2
#define LED4            3
#define LED5            4
#define LED6            5
#define LED7            6
#define LED8            7

#if defined(PTTY)
#   define SERIAL0                8
#   if #defined(__msp430_have_usart1)
#       define SERIAL1            9
#       define FREE_DEVICE_MAX   10
#   else
#       define FREE_DEVICE_MAX    9
#   endif
#else
#   define FREE_DEVICE_MAX        8
#endif


/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#if defined(SERIAL0)
static struct moption_t ptty0_opt = {
  .longname    = "serial0",
  .type        = required_argument,
  .helpstring  = "serial port 0",
  .value       = NULL
};
#endif

#if defined(SERIAL1)
static struct moption_t ptty1_opt = {
  .longname    = "serial1",
  .type        = required_argument,
  .helpstring  = "serial port 1",
  .value       = NULL
};
#endif

int devices_options_add()
{
#if defined(SERIAL0)
  options_add(&ptty0_opt);
#endif

#if defined(SERIAL1)
  options_add(&ptty1_opt);
#endif
  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int devices_create()
{
  int res = 0;

  /*********************************/
  /* MSP430 MCU                    */
  /*********************************/
  res += msp430_mcu_create(32768 /* xin_freq*/, 8000000 /* xt2in_freq */);

  /*********************************/
  /* fix peripheral sizes          */
  /*********************************/
  machine.device_max           = FREE_DEVICE_MAX;
  machine.device_size[LED1]    = led_device_size();    // Led1
  machine.device_size[LED2]    = led_device_size();    // Led2
  machine.device_size[LED3]    = led_device_size();    // Led3
  machine.device_size[LED4]    = led_device_size();    // Led1
  machine.device_size[LED5]    = led_device_size();    // Led2
  machine.device_size[LED6]    = led_device_size();    // Led3
  machine.device_size[LED7]    = led_device_size();    // Led1
  machine.device_size[LED8]    = led_device_size();    // Led2
#if defined(SERIAL0)
  machine.device_size[SERIAL0] = ptty_device_size();
#endif
#if defined(SERIAL1)
  machine.device_size[SERIAL1] = ptty_device_size();
#endif

  /*********************************/
  /* allocate memory               */
  /*********************************/
  res += devices_memory_allocate();
  /*********************************/
  /* create peripherals            */
  /*********************************/
  res += led_device_create      (LED1,0xee,0,0);
  res += led_device_create      (LED2,0xee,0,0);
  res += led_device_create      (LED3,0xee,0,0);
  res += led_device_create      (LED4,0xee,0,0);
  res += led_device_create      (LED5,0xee,0,0);
  res += led_device_create      (LED6,0xee,0,0);
  res += led_device_create      (LED7,0xee,0,0);
  res += led_device_create      (LED8,0xee,0,0);
#if defined(SERIAL0)
  res += ptty_device_create     (SERIAL0,0,ptty0_opt.value);
#endif
#if defined(SERIAL1)
  res += ptty_device_create     (SERIAL1,0,ptty1_opt.value);
#endif

  /*********************************/
  /* place peripherals Gui         */
  /*********************************/
  {
    int lw,lh;

    machine.device[LED1].ui_get_size(LED1,&lw,&lh);

    machine.device[LED1].ui_set_pos(LED1,    0,  0);
    machine.device[LED2].ui_set_pos(LED2,   lw,  0);
    machine.device[LED3].ui_set_pos(LED3, 2*lw,  0);
    machine.device[LED4].ui_set_pos(LED4, 3*lw,  0);
    machine.device[LED5].ui_set_pos(LED5, 4*lw,  0);
    machine.device[LED6].ui_set_pos(LED6, 5*lw,  0);
    machine.device[LED7].ui_set_pos(LED7, 6*lw,  0);
    machine.device[LED8].ui_set_pos(LED8, 7*lw,  0);
  }
  /*********************************/
  /* end of platform specific part */
  /*********************************/
  return res;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/* this function is called after devices reset    */
/* devices init conditions should be written here */
int devices_reset_post()
{
  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int devices_update()
{
  int res = 0;
  int refresh = 0;
  uint8_t  val8;

  /* *************************************************************************** */
  /* MCU -> devices                                                              */
  /* *************************************************************************** */

  /* port 1 :                          */
  /* ========                          */
  /*   P5.7 -- P5.0 leds               */
  if (msp430_digiIO_dev_read(PORT1,&val8))
    {
      int i;
      for(i = LED1; i <= LED8 ; i++)
	{
	  machine.device[i].write(i,LED_DATA,BIT(val8,(i - LED1)));
	  UPDATE(i);
	  REFRESH(i);
	}
    }

  /* port 2 :                           */
  /* ========                           */
#if defined(SERIAL0)
  if (msp430_usart0_dev_read_uart(&val8))
    {
      machine.device[SERIAL0].write(SERIAL0, PTTY_D, val8);
    }
#endif

#if defined(SERIAL1)
  if (msp430_usart1_dev_read_uart(&val8))
    {
      machine.device[SERIAL1].write(SERIAL1, PTTY_D, val8);
    }
#endif

  /* port 3:                            */
  /* =======                            */


  /* *************************************************************************** */
  /* devices -> MCU                                                              */
  /* *************************************************************************** */

  /* input on usart0 line */
#if defined(SERIAL0)
  if (msp430_usart0_dev_write_uart_ok())
     {
       uint32_t mask,value;
       machine.device[SERIAL0].read(SERIAL0,&mask,&value);
       if ((mask & PTTY_D) != 0)
	 msp430_usart0_dev_write_uart(value & PTTY_D);
     }
#endif

  /* input on UI */
  {
    /* poll event every */
    static int loop_count = UI_EVENT_SKIP;
    if ((loop_count--) == 0)
      {
	int ev;
	loop_count = UI_EVENT_SKIP;
	switch ((ev = ui_getevent()))
	  {
	  case UI_EVENT_QUIT:
	    HW_DMSG_UI("  devices UI event QUIT\n");
	    MCU_SIGNAL = SIG_UI;
	    break;
	  case UI_EVENT_USER:
	  case UI_EVENT_NONE:
	    break;
	  default:
	    ERROR("devices: unknown ui event\n");
	    break;
	  }
      }
  }


  /* *************************************************************************** */
  /* update                                                                      */
  /* *************************************************************************** */
  LIBSELECT_UPDATE();
  LIBWSNET_UPDATE();

#if defined(SERIAL0)
  UPDATE(SERIAL0);
#endif

#if defined(SERIAL1)
  UPDATE(SERIAL1);
#endif

  if (refresh) 
    {
      ui_refresh();
    }

  return res;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */