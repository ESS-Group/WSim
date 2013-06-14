/**
 *  \file   msp430_adc12.c
 *  \brief  MSP430 Adc12 controller
 *  \author Antoine Fraboulet, David Gr�ff
 *  \date   2006, 2013
 **/

#include <stdlib.h>
#include <string.h>
#include "arch/common/hardware.h"
#include "msp430.h"
#include "src/options.h"




#if defined(__msp430_have_adc12)

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#define HW_DMSG_ADC12(x...) HW_DMSG_MCUDEV(x)

#define ADC12_DEBUG_LEVEL_2 0

#if ADC12_DEBUG_LEVEL_2 != 0
#define HW_DMSG_2_DBG(x...) HW_DMSG_ADC12(x)
#else
#define HW_DMSG_2_DBG(x...) do { } while (0)
#endif


/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#if !defined(ADC12_BASE)
#define ADC12_BASE   0x01A0
#define ADC12M_BASE  0x0120
#define ADC12MC_BASE 0x0070
#endif

enum adc12_addr_t {
  /* control offset address are different for ADC12_A */
  #ifdef ADC12A_PLUS
  ADC12CTL0   = (ADC12_BASE + 0x0000), /* 16 */
  ADC12CTL1   = (ADC12_BASE + 0x0002), /* 16 */
  ADC12CTL2   = (ADC12_BASE + 0x0004), /* 16 */
  ADC12IFG    = (ADC12_BASE + 0x000A), /* 16 */
  ADC12IE     = (ADC12_BASE + 0x000C), /* 16 */
  ADC12IV     = (ADC12_BASE + 0x000E), /* 16 */
  #else
  ADC12CTL0   = (ADC12_BASE + 0x0000), /* 16 */
  ADC12CTL1   = (ADC12_BASE + 0x0002), /* 16 */
  ADC12IFG    = (ADC12_BASE + 0x0004), /* 16 */
  ADC12IE     = (ADC12_BASE + 0x0006), /* 16 */
  ADC12IV     = (ADC12_BASE + 0x0008), /* 16 */
  #endif
  
  ADC12MEM0   = (ADC12M_BASE + 0x0020), /* 16 */
  ADC12MEM1   = (ADC12M_BASE + 0x0022),
  ADC12MEM2   = (ADC12M_BASE + 0x0024),
  ADC12MEM3   = (ADC12M_BASE + 0x0026),
  ADC12MEM4   = (ADC12M_BASE + 0x0028),
  ADC12MEM5   = (ADC12M_BASE + 0x002A),
  ADC12MEM6   = (ADC12M_BASE + 0x002C),
  ADC12MEM7   = (ADC12M_BASE + 0x002E),
  ADC12MEM8   = (ADC12M_BASE + 0x0030),
  ADC12MEM9   = (ADC12M_BASE + 0x0032),
  ADC12MEM10  = (ADC12M_BASE + 0x0034),
  ADC12MEM11  = (ADC12M_BASE + 0x0036),
  ADC12MEM12  = (ADC12M_BASE + 0x0038),
  ADC12MEM13  = (ADC12M_BASE + 0x003A),
  ADC12MEM14  = (ADC12M_BASE + 0x003C),
  ADC12MEM15  = (ADC12M_BASE + 0x003E),
  
  ADC12MCTL0  = (ADC12MC_BASE + 0x010), /*  8 */
  ADC12MCTL1  = (ADC12MC_BASE + 0x011),
  ADC12MCTL2  = (ADC12MC_BASE + 0x012),
  ADC12MCTL3  = (ADC12MC_BASE + 0x013),
  ADC12MCTL4  = (ADC12MC_BASE + 0x014),
  ADC12MCTL5  = (ADC12MC_BASE + 0x015),
  ADC12MCTL6  = (ADC12MC_BASE + 0x016),
  ADC12MCTL7  = (ADC12MC_BASE + 0x017),
  ADC12MCTL8  = (ADC12MC_BASE + 0x018),
  ADC12MCTL9  = (ADC12MC_BASE + 0x019),
  ADC12MCTL10 = (ADC12MC_BASE + 0x01A),
  ADC12MCTL11 = (ADC12MC_BASE + 0x01B),
  ADC12MCTL12 = (ADC12MC_BASE + 0x01C),
  ADC12MCTL13 = (ADC12MC_BASE + 0x01D),
  ADC12MCTL14 = (ADC12MC_BASE + 0x01E),
  ADC12MCTL15 = (ADC12MC_BASE + 0x01F)
};

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

tracer_id_t MSP430_TRACER_ADC12STATE;
tracer_id_t MSP430_TRACER_ADC12INPUT[ADC12_CHANNELS];

#define ADC12_TRACER_STATE(v)   tracer_event_record(MSP430_TRACER_ADC12STATE, v)
#define ADC12_TRACER_INPUT(i,v) tracer_event_record(MSP430_TRACER_ADC12INPUT[i], v)

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#define ADC12_CHANNEL_NAMES 20
char trace_names[ADC12_CHANNELS][ADC12_CHANNEL_NAMES] = {
  "adc12_input_00", "adc12_input_01", "adc12_input_02", "adc12_input_03",
  "adc12_input_04", "adc12_input_05", "adc12_input_06", "adc12_input_07",
  "adc12_input_08", "adc12_input_09", "adc12_input_10", "adc12_input_11",
  "adc12_input_12", "adc12_input_13", "adc12_input_14", "adc12_input_15"
};

#define ADC12_MODES        4
#define ADC12_MODES_NAMES 40
char adc12_modes[ADC12_MODES][ADC12_MODES_NAMES] = {
  "Single Channel Single Conversion",  "Sequence of Channels",
  "Repeat Single Channel",  "Repeat Sequence of Channels"
};

#define ADC12_STATES        6
#define ADC12_STATES_NAMES 40
char adc12_states[ADC12_STATES][ADC12_STATES_NAMES] = {
  "Off", "Wait enable", "Wait trigger", "Sample", "Convert", "Store"
};

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/* ADC12 internal OSC is ~ 5MHz */
/* cycle_nanotime == 200        */
#define NANO                    (  1000*1000*1000)
#define ADC12OSC_FREQ           (     5*1000*1000)

#define ADC12OSC_CYCLE_NANOTIME (NANO / ADC12OSC_FREQ) 


static struct moption_t adc12_in_opt = {
  .longname    = "msp430_adc12",
  .type        = required_argument,
  .helpstring  = "msp430 adc12 input",
  .value       = NULL
};

int msp430_adc_option_add (void)
{
  options_add( &adc12_in_opt );
  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static int msp430_adc12_init(void)
{

  msp430_adc_init(& MCU.adc12.channels, 12, &adc12_in_opt);
  
  MSP430_TRACER_ADC12STATE    = tracer_event_add_id(1,  "adc12_state",   "msp430");
  
  int i;
  for(i=0; i<ADC12_CHANNELS; i++)
    {     
      if (MCU.adc12.channels.channels_valid[i] != ADC_NONE )
	{
	  MSP430_TRACER_ADC12INPUT[i] = tracer_event_add_id(16,  trace_names[i],   "msp430");
	}

      MCU.adc12.mem[i].s = 0;
    }

  
  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_adc12_create(void)
{
  msp430_io_register_range8 (ADC12MCTL0,ADC12MCTL15,msp430_adc12_read8 ,msp430_adc12_write8);
  msp430_io_register_range16(ADC12CTL0 ,ADC12IV    ,msp430_adc12_read16,msp430_adc12_write16);
  msp430_io_register_range16(ADC12MEM0 ,ADC12MEM15 ,msp430_adc12_read16,msp430_adc12_write16);
  
  msp430_adc12_init();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_adc12_reset(void)
{
  int i;
  /* set initial values */
  HW_DMSG_ADC12("msp430:adc12:reset()\n");
  MCU.adc12.ctl0.s = 0;
  MCU.adc12.ctl1.s = 0;
  MCU.adc12.ifg    = 0;
  MCU.adc12.ie     = 0;
  MCU.adc12.iv     = 0;
  for(i=0; i<ADC12_CHANNELS; i++)
    {
      MCU.adc12.mctl[i].s = 0;
    }

  MCU.adc12.adc12osc_freq           = ADC12OSC_FREQ;
  MCU.adc12.adc12osc_cycle_nanotime = ADC12OSC_CYCLE_NANOTIME;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static inline void ADC12_SET_STATE(int state)
{
  int current_state = MCU.adc12.state;

  MCU.adc12.state = state;
  ADC12_TRACER_STATE(MCU.adc12.state);
  if (current_state != state)
    {
      HW_DMSG_ADC12("msp430:adc12: mode \"%s\", state \"%s\" -> \"%s\"\n", 
		    adc12_modes[MCU.adc12.ctl1.b.conseqx], 
		    adc12_states[current_state],
		    adc12_states[state]);
    }
  switch (state)
    {
    case ADC_STATE_SAMPLE:
    case ADC_STATE_CONVERT:
    case ADC_STATE_STORE:
            MCU.adc12.ctl1.b.adc12busy = 1; /* operation done */
	    break;
    default:
            MCU.adc12.ctl1.b.adc12busy = 0; /* operation done */
	    break;
    }
}



#define CHECK_ENC()					\
  do {							\
    if (MCU.adc12.ctl0.b.enc == 0)			\
      {							\
	ADC12_SET_STATE( ADC_STATE_WAIT_ENABLE );	\
	return;						\
      }							\
  } while(0)


static const int shtdiv[16] = 
{
  4, 8, 16, 32, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1024, 1024, 1024 
};


void msp430_adc12_update(void)
{
  if (MCU.adc12.ctl0.b.adc12on == 0)
    {
      ADC12_SET_STATE( ADC_STATE_OFF );
      return;
    }

  MCU.adc12.adc12osc_temp     += MACHINE_TIME_GET_INCR();
  MCU.adc12.adc12osc_increment = MCU.adc12.adc12osc_temp / MCU.adc12.adc12osc_cycle_nanotime;
  MCU.adc12.adc12osc_temp      = MCU.adc12.adc12osc_temp % MCU.adc12.adc12osc_cycle_nanotime;
  MCU.adc12.adc12osc_counter  += MCU.adc12.adc12osc_increment;

  MCU.adc12.adc12clk_temp     += MCU.adc12.adc12osc_increment;
  MCU.adc12.adc12clk_increment = MCU.adc12.adc12clk_temp / (MCU.adc12.ctl1.b.adc12divx + 1);
  MCU.adc12.adc12clk_temp      = MCU.adc12.adc12clk_temp % (MCU.adc12.ctl1.b.adc12divx + 1);
  MCU.adc12.adc12clk_counter  += MCU.adc12.adc12clk_increment;

  /* FIXME:: MSC is not taken into account */
  if (MCU.adc12.ctl1.b.shp == 1)
    {
      if ((0 <= MCU.adc12.current_x) && (MCU.adc12.current_x <= 7))
	{
	  /* registers MEM0 to MEM7  */
	  MCU.adc12.sht0_temp         += MCU.adc12.adc12clk_increment;
	  MCU.adc12.sht0_increment     = MCU.adc12.sht0_temp / (shtdiv[MCU.adc12.ctl0.b.sht0x]);
	  MCU.adc12.sht0_temp          = MCU.adc12.sht0_temp % (shtdiv[MCU.adc12.ctl0.b.sht0x]);
	  MCU.adc12.sampcon            = MCU.adc12.sht0_increment;
	}
      else if ((8 <= MCU.adc12.current_x) && (MCU.adc12.current_x <= 15))
	{
	  /* registers MEM8 to MEM15 */
	  MCU.adc12.sht1_temp         += MCU.adc12.adc12clk_increment;
	  MCU.adc12.sht1_increment     = MCU.adc12.sht1_temp / (shtdiv[MCU.adc12.ctl0.b.sht1x]);
	  MCU.adc12.sht1_temp          = MCU.adc12.sht1_temp % (shtdiv[MCU.adc12.ctl0.b.sht1x]);
	  MCU.adc12.sampcon            = MCU.adc12.sht1_increment;
	}
      else
	{
	  ERROR("msp430:adc12: channel out of bounds\n");
	  machine_exit_error();
	}
    }
  else
    {
      /* FIXME::sampcon = SHI */
      // ERROR("msp430:adc12: ADC does not currently support SHP = 0 mode\n");
      MCU.adc12.sampcon = 0;
    }


  switch (MCU.adc12.state)
    {
      /***************/
      /* OFF         */
      /***************/
    case ADC_STATE_OFF: 
      ADC12_SET_STATE( ADC_STATE_WAIT_ENABLE );
      /* no break */

      /***************/
      /* ENABLE      */
      /***************/
    case ADC_STATE_WAIT_ENABLE:
      if (MCU.adc12.ctl0.b.enc)
	{
	  ADC12_SET_STATE( ADC_STATE_WAIT_TRIGGER ); 
	}
      if ((MCU.adc12.ctl0.b.enc) && 
	  (MCU.adc12.ctl0.b.adc12sc == 1) &&
	  (MCU.adc12.ctl1.b.shsx == 0))
	{
	  ADC12_SET_STATE( ADC_STATE_SAMPLE );
	}
      break;
      
      /***************/
      /* TRIGGER     */
      /***************/
    case ADC_STATE_WAIT_TRIGGER:
      CHECK_ENC();
      if (MCU.adc12.sampcon > 0)
	{
	  ADC12_SET_STATE( ADC_STATE_SAMPLE );
	  MCU.adc12.sampcon --;
	}
      else
	{
	  return;
	}
      /* no break */

      /***************/
      /* SAMPLE      */
      /***************/
    case ADC_STATE_SAMPLE:
      CHECK_ENC();
      if (MCU.adc12.sampcon > 0)
	{
	  /* 
	   * check port configuration. 
	   *   SEL = 1  // Selector  = 0:GPIO  1:peripheral
	   *   DIR = 0  // Direction = 0:input 1:output
	   */
	  MCU.adc12.sample = msp430_adc_sample_input(&MCU.adc12.channels, ADC_CHANNELS, MCU.adc12.current_x);
	  ADC12_TRACER_INPUT( MCU.adc12.mctl[MCU.adc12.current_x].b.inch, MCU.adc12.sample );

	  HW_DMSG_ADC12("msp430:adc12:     sampling on config %d hw_channel %d (%s) = 0x%04x [%"PRId64"]\n",
			MCU.adc12.current_x, 
			MCU.adc12.mctl[MCU.adc12.current_x].b.inch,
			trace_names[MCU.adc12.mctl[MCU.adc12.current_x].b.inch],
			MCU.adc12.sample,
			MACHINE_TIME_GET_NANO());
 
	  ADC12_SET_STATE( ADC_STATE_CONVERT );
	  MCU.adc12.adc12clk_reftime = MCU.adc12.adc12clk_counter;
	  MCU.adc12.sampcon --;
	  
	}
      else
	{
	  return;
	}
      break;

      /***************/
      /* CONVERT     */
      /***************/
    case ADC_STATE_CONVERT:
      CHECK_ENC();
      if (MCU.adc12.adc12clk_counter > (MCU.adc12.adc12clk_reftime + 12))
	{
	  HW_DMSG_ADC12("msp430:adc12:     convert = 0x%04x (%d) \n",MCU.adc12.sample,MCU.adc12.sample);
	  ADC12_SET_STATE( ADC_STATE_STORE );
	}
      else
	{ 
	  return;
	}
      /* no break */

#define SAMPLE MCU.adc12.sample
#define STATE  MCU.adc12.state
#define ENC    MCU.adc12.ctl0.b.enc
#define MSC    MCU.adc12.ctl0.b.msc
#define SHP    MCU.adc12.ctl1.b.shp
#define ADC12x MCU.adc12.current_x

      /***************/
      /* STORE       */
      /***************/
    case ADC_STATE_STORE:
      CHECK_ENC();
      HW_DMSG_ADC12("msp430:adc12:     ADC12MEM%d = 0x%04x\n", ADC12x, SAMPLE);
      MCU.adc12.mem[ ADC12x ].s = SAMPLE;

      if ((MCU.adc12.ifg & (1 << ADC12x)) == 0)
	{
	  HW_DMSG_ADC12("msp430:adc12: set interrupt for channel %d\n", ADC12x);
	}
      MCU.adc12.ifg |= 1 << ADC12x;
      msp430_adc12_chkifg();

      switch (MCU.adc12.ctl1.b.conseqx)
	{
	case ADC_MODE_SINGLE:
	  ENC   = 0;
	  ADC12_SET_STATE( ADC_STATE_WAIT_ENABLE );
	  break;

	case ADC_MODE_SEQ_CHAN:
	  if (MCU.adc12.mctl[ ADC12x ].b.eos == 1)
	    {
	      ENC   = 0;
	      ADC12_SET_STATE( ADC_STATE_WAIT_ENABLE );
	    }
	  else
	    {
	      ADC12x = (ADC12x + 1) % ADC12_CHANNELS;
	      if (MSC && SHP)
		{
		  ADC12_SET_STATE( ADC_STATE_SAMPLE );
		}
	      else
		{
		  ADC12_SET_STATE( ADC_STATE_WAIT_TRIGGER );
		}
	    }
	  break;

	case ADC_MODE_REPEAT_SINGLE:
	  if (ENC == 0)
	    {
	      ADC12_SET_STATE( ADC_STATE_WAIT_ENABLE );
	    }
	  else
	    {
	      if (MSC && SHP)
		{
		  ADC12_SET_STATE( ADC_STATE_SAMPLE );
		}
	      else
		{
		  ADC12_SET_STATE( ADC_STATE_WAIT_TRIGGER );
		}
	    }
	  break;

	case ADC_MODE_REPEAT_SEQ:
	  if ((ENC == 0) && (MCU.adc12.mctl[ ADC12x ].b.eos == 1))
	    {
	      ADC12_SET_STATE( ADC_STATE_WAIT_ENABLE );
	    }
	  else
	    {
	      ADC12x = (ADC12x + 1) % ADC12_CHANNELS;
	      if (MSC && SHP)
		{
		  ADC12_SET_STATE( ADC_STATE_SAMPLE );
		}
	      else
		{
		  ADC12_SET_STATE( ADC_STATE_WAIT_TRIGGER );
		}
	    }
	  break;
	}
      break;
    }

}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int16_t msp430_adc12_read16(uint16_t addr)
{
  int16_t ret = 0;

  /*
  static int tinyos_read_this_too_many_times = 0;
  if (tinyos_read_this_too_many_times == 0)
    {
      HW_DMSG_ADC12("msp430:adc12: read16 at [0x%04x] \n",addr);
      tinyos_read_this_too_many_times = 1;
    }
  */

  switch (addr)
    {
    case ADC12CTL0    : /* 16 */
      HW_DMSG_ADC12("msp430:adc12:read16: ADC12CTL0 = 0x%04x\n",MCU.adc12.ctl0.s);
      ret = MCU.adc12.ctl0.s;
      break;

    case ADC12CTL1    : /* 16 */
      HW_DMSG_ADC12("msp430:adc12:read16: ADC12CTL1 = 0x%04x\n",MCU.adc12.ctl1.s);
      ret = MCU.adc12.ctl1.s;
      break;

    case ADC12IFG     : /* 16 */
      HW_DMSG_ADC12("msp430:adc12:read16: ADC12IFG = 0x%04x\n",MCU.adc12.ifg);
      ret = MCU.adc12.ifg;
      break;

    case ADC12IE      : /* 16 */
      HW_DMSG_ADC12("msp430:adc12:read16: ADC12IFG = 0x%04x\n",MCU.adc12.ie);
      ret = MCU.adc12.ie;
      break;

    case  ADC12IV     : /* 16 */
      MCU.adc12.ov  = 0;
      MCU.adc12.tov = 0;
      HW_DMSG_ADC12("msp430:adc12:read16: ADC12IV = 0x%04x\n",MCU.adc12.iv);
      ret = MCU.adc12.iv;
      break;

    case  ADC12MEM0   : /* 16 */
    case  ADC12MEM1   :
    case  ADC12MEM2   :
    case  ADC12MEM3   :
    case  ADC12MEM4   :
    case  ADC12MEM5   :
    case  ADC12MEM6   :
    case  ADC12MEM7   :
    case  ADC12MEM8   :
    case  ADC12MEM9   :
    case  ADC12MEM10  :
    case  ADC12MEM11  :
    case  ADC12MEM12  :
    case  ADC12MEM13  :
    case  ADC12MEM14  :
    case  ADC12MEM15  :
      HW_DMSG_ADC12("msp430:adc12:read16: ADC12MEM%d = 0x%04x\n",addr - ADC12MEM0, 
		    MCU.adc12.mem[(addr - ADC12MEM0) / 2].s);
      MCU.adc12.ifg &= ~(1 << ((addr - ADC12MEM0) / 2));
      ret = MCU.adc12.mem[(addr - ADC12MEM0) / 2].s;
      break;

    default:
      HW_DMSG_ADC12("msp430:adc12:read16: at [0x%04x], unknown address\n",addr);
      break;
    }

  return ret;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int8_t msp430_adc12_read8(uint16_t addr)
{
  int8_t ret = 0;

  /*
  static int tinyos_read_this_too_many_times = 0;
  if (tinyos_read_this_too_many_times == 0)
    {
      HW_DMSG_ADC12("msp430:adc12: read8 at [0x%04x] \n",addr);
      tinyos_read_this_too_many_times = 1;
    }
  */

  switch (addr) 
    {
    case  ADC12MCTL0  : /*  8 */
    case  ADC12MCTL1  :
    case  ADC12MCTL2  :
    case  ADC12MCTL3  :
    case  ADC12MCTL4  :
    case  ADC12MCTL5  :
    case  ADC12MCTL6  :
    case  ADC12MCTL7  :
    case  ADC12MCTL8  :
    case  ADC12MCTL9  :
    case  ADC12MCTL10 :
    case  ADC12MCTL11 :
    case  ADC12MCTL12 :
    case  ADC12MCTL13 :
    case  ADC12MCTL14 :
    case  ADC12MCTL15 :
      HW_DMSG_ADC12("msp430:adc12:read08: ADC12MEMCTL%d = 0x%02x\n", addr - ADC12MCTL0,
		    MCU.adc12.mctl[addr - ADC12MCTL0].s);
      ret = MCU.adc12.mctl[addr - ADC12MCTL0].s & 0xff;
      break;
    default:
      HW_DMSG_ADC12("msp430:adc12:read08: at [0x%04x], unknown address\n",addr);
      break;
    }
  return ret;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#define WR_IF_MOD(sarg,arg)						\
  do {									\
    if (ref-> arg != val-> arg)						\
      HW_DMSG_ADC12("msp430:adc12:%s %10s %d -> %d\n",			\
		    msg, sarg, ref-> arg, val-> arg );			\
  } while (0)

void msp430_adc12_ctl0details(char UNUSED *msg, uint16_t UNUSED *c1, int16_t UNUSED *c2)
{
  struct adc12ctl0_t *ref = (struct adc12ctl0_t*)c1;
  struct adc12ctl0_t *val = (struct adc12ctl0_t*)c2;

  WR_IF_MOD("sht1x"      ,sht1x);
  WR_IF_MOD("sht0x"      ,sht0x);
  WR_IF_MOD("msc"        ,msc);
  WR_IF_MOD("ref2_5v"    ,ref2_5v);
  WR_IF_MOD("refon"      ,refon);
  WR_IF_MOD("adc12on"    ,adc12on);
  WR_IF_MOD("adc12ovie"  ,adc12ovie);
  WR_IF_MOD("adc12tovie" ,adc12tovie);
  WR_IF_MOD("enc"        ,enc);
  WR_IF_MOD("adc12sc"    ,adc12sc);
}

void msp430_adc12_ctl1details(char UNUSED *msg, uint16_t UNUSED *c1, int16_t UNUSED *c2)
{
  struct adc12ctl1_t *ref = (struct adc12ctl1_t*) c1;
  struct adc12ctl1_t *val = (struct adc12ctl1_t*) c2;

  WR_IF_MOD("cstartaddx",cstartaddx);
  WR_IF_MOD("shsx"      ,shsx);
  WR_IF_MOD("shp"       ,shp);
  WR_IF_MOD("issh"      ,issh);
  WR_IF_MOD("adc12divx" ,adc12divx);
  WR_IF_MOD("adc12sselx",adc12sselx);
  WR_IF_MOD("conseqx"   ,conseqx);
  WR_IF_MOD("adc12busy" ,adc12busy);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_adc12_start_enc(void)
{
  char buff[70];
#define FORMAT "msp430:adc12:    * %-50s *\n"

#define ADC12PPP(x...) do {			\
    snprintf(buff,70,x);			\
    HW_DMSG_ADC12(FORMAT,buff);			\
  } while (0)

  ADC12PPP("sht0 = 0x%x /%d",MCU.adc12.ctl0.b.sht0x & 0xf,shtdiv[MCU.adc12.ctl0.b.sht0x]);
  ADC12PPP("sht1 = 0x%x /%d",MCU.adc12.ctl0.b.sht1x & 0xf,shtdiv[MCU.adc12.ctl0.b.sht1x]);

  switch (MCU.adc12.ctl1.b.adc12sselx)
    {
    case 0: /* ADC12OSC */
      ADC12PPP("CLK source = ADC12OSC");
      break;
    case 1: /* ACLK */
      ADC12PPP("CLK source = ACLK");
      break;
    case 2: /* MCLK */
      ADC12PPP("CLK source = MCLK");
      break;
    case 3: /* SMCLK */
      ADC12PPP("CLK source = SMCLK");
      break;
    }
  ADC12PPP("CLK div %d :: /%d",MCU.adc12.ctl1.b.adc12divx,MCU.adc12.ctl1.b.adc12divx+1);
  ADC12PPP("ISSH input signal [%s]",(MCU.adc12.ctl1.b.issh == 0) ? "normal":"inverted");
  ADC12PPP("MSC %d",MCU.adc12.ctl0.b.msc);
  ADC12PPP("SHP %d :: sourced from %s signal",MCU.adc12.ctl1.b.shp,(MCU.adc12.ctl1.b.shp == 0) ? "sample-input":"sampling");
  switch (MCU.adc12.ctl1.b.shsx)
    {
    case 0: /* ADC12SX bit */
      ADC12PPP("SHS source = ADC12SC bit");
      break;
    case 1: /* TimerA.out1 */
      ADC12PPP("SHS source = TimerA.OUT1");
      break;
    case 2: /* TimerB.out0 */
      ADC12PPP("SHS source = TimerB.OUT0");
      break;
    case 3: /* TimerB.out1 */
      ADC12PPP("SHS source = TimerB.OUT1");
      break;
    }
  ADC12PPP("ADC12SC             : %d",MCU.adc12.ctl0.b.adc12sc);
  ADC12PPP("start sequence      : %d",MCU.adc12.ctl1.b.cstartaddx);
  ADC12PPP("conversion sequence : %s",adc12_modes[MCU.adc12.ctl1.b.conseqx]);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_adc12_write16(uint16_t addr, int16_t val)
{
  switch (addr)
    {
    case ADC12CTL0    : /* 16 */
      {
	union {
	  uint16_t s;
	  struct adc12ctl0_t b;
	} pval;
	pval.s = val;

	HW_DMSG_ADC12("msp430:adc12:write16: ADC12CTL0 = 0x%04x\n",val);

	if (val != MCU.adc12.ctl0.s)
	  {
	    msp430_adc12_ctl0details("    ctl0 set ", &MCU.adc12.ctl0.s, &val);
	    if (MCU.adc12.ctl0.b.enc == 0)
	      {
		if (pval.b.enc == 1) /* configuratioin is fixed */
		  {
		    HW_DMSG_ADC12("msp430:adc12:    *** START ENC **\n");
		    msp430_adc12_start_enc();
		    HW_DMSG_ADC12("msp430:adc12:    *** ********* **\n");
		    MCU.adc12.current_x = MCU.adc12.ctl1.b.cstartaddx;
		  }
		MCU.adc12.ctl0.s = val;
	      }
	    else /* enc == 1, change only 4 lower bits at most */
	      {
		HW_DMSG_ADC12("msp430:adc12:    ctl0 modified while \"enc\" == 1\n");
		MCU.adc12.ctl0.s = (MCU.adc12.ctl0.s & 0xfff0) | (val & 0xf);
	      }
	  }
      }
      break;

    case ADC12CTL1    : /* 16 */
      HW_DMSG_ADC12("msp430:adc12:write16: ADC12CTL1 = 0x%04x\n",val);
      if (val != MCU.adc12.ctl1.s)
	{
	  msp430_adc12_ctl1details("    ctl1 set ",&MCU.adc12.ctl1.s, &val);
	}
      if (MCU.adc12.ctl0.b.enc == 0)
	{
	  MCU.adc12.ctl1.s = val;
	}
      else
	{
	  MCU.adc12.ctl1.s = (MCU.adc12.ctl1.s & 0xfff0) | (val & 0xf);
	}
      break;

    case ADC12IFG     : /* 16 */
      if (MCU.adc12.ifg != val)
	{
	  HW_DMSG_ADC12("msp430:adc12:write16: ADC12IFG changed from 0x%04x to 0x%04x\n",MCU.adc12.ifg,val);
	}
      MCU.adc12.ifg = val;
      break;

    case ADC12IE      : /* 16 */
      if (MCU.adc12.ie != val)
	{
	  HW_DMSG_ADC12("msp430:adc12:write16: ADC12IE changed from 0x%04x to 0x%04x\n",MCU.adc12.ie,val);
	}
      MCU.adc12.ie = val;
      break;

    case  ADC12IV     : /* 16 */
      MCU.adc12.ov  = 0;
      MCU.adc12.tov = 0;
      HW_DMSG_ADC12("msp430:adc12:write16: write to ADC12IV, read only register\n");
      break;

    case  ADC12MEM0   : /* 16 */
    case  ADC12MEM1   :
    case  ADC12MEM2   :
    case  ADC12MEM3   :
    case  ADC12MEM4   :
    case  ADC12MEM5   :
    case  ADC12MEM6   :
    case  ADC12MEM7   :
    case  ADC12MEM8   :
    case  ADC12MEM9   :
    case  ADC12MEM10  :
    case  ADC12MEM11  :
    case  ADC12MEM12  :
    case  ADC12MEM13  :
    case  ADC12MEM14  :
    case  ADC12MEM15  :
      HW_DMSG_ADC12("msp430:adc12:write16: ADC12MEM%d = 0x%04x\n", (addr - ADC12MEM0)/2, val & 0xffff);
      MCU.adc12.mem[(addr - ADC12MEM0) / 2].b.value = val & 0x0FFF;
      MCU.adc12.ifg &= ~(1 << ((addr - ADC12MEM0) / 2));
      break;

    default:
      HW_DMSG_ADC12("msp430:adc12:write16: at [0x%04x] = 0x%04x, unknown address\n",addr,val & 0xffff);
      break;
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void msp430_adc12_mctl_details(int UNUSED n, struct adc12mctlx_t UNUSED *mctl)
{
  HW_DMSG_ADC12("msp430:adc12:    mctl%d  eos :%d\n",n,mctl->eos);
  HW_DMSG_ADC12("msp430:adc12:    mctl%d  sref:%x\n",n,mctl->sref);
  HW_DMSG_ADC12("msp430:adc12:    mctl%d  inch:%x\n",n,mctl->inch);
}

void msp430_adc12_write8(uint16_t addr, int8_t val)
{
  switch (addr)
    {
    case  ADC12MCTL0  : /*  8 */
    case  ADC12MCTL1  :
    case  ADC12MCTL2  :
    case  ADC12MCTL3  :
    case  ADC12MCTL4  :
    case  ADC12MCTL5  :
    case  ADC12MCTL6  :
    case  ADC12MCTL7  :
    case  ADC12MCTL8  :
    case  ADC12MCTL9  :
    case  ADC12MCTL10 :
    case  ADC12MCTL11 :
    case  ADC12MCTL12 :
    case  ADC12MCTL13 :
    case  ADC12MCTL14 :
    case  ADC12MCTL15 :
      HW_DMSG_ADC12("msp430:adc12:write08: ADC12MCTL%d = 0x%02x\n", addr - ADC12MCTL0, val & 0xff);
      if (MCU.adc12.ctl0.b.enc == 0)
	{
	  MCU.adc12.mctl[addr - ADC12MCTL0].s = val & 0xff;
	  msp430_adc12_mctl_details(addr - ADC12MCTL0, & (MCU.adc12.mctl[addr - ADC12MCTL0].b));
	}
      break;
    default:
      HW_DMSG_ADC12("msp430:adc12:write08: unknown address for [0x%04x] = 0x%02x\n",addr,val & 0xff);
      break;
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
/*
ADC12IV                                           Interrupt
Contents                                          Priority
                Interrupt Source   Interrupt Flag
  000h   No interrupt pending             -
  002h   ADC12MEMx overflow               -       Highest
  004h   Conversion time overflow         -
  006h   ADC12MEM0  interrupt flag  ADC12IFG0
  008h   ADC12MEM1  interrupt flag  ADC12IFG1
  00Ah   ADC12MEM2  interrupt flag  ADC12IFG2
  00Ch   ADC12MEM3  interrupt flag  ADC12IFG3
  00Eh   ADC12MEM4  interrupt flag  ADC12IFG4
  010h   ADC12MEM5  interrupt flag  ADC12IFG5
  012h   ADC12MEM6  interrupt flag  ADC12IFG6
  014h   ADC12MEM7  interrupt flag  ADC12IFG7
  016h   ADC12MEM8  interrupt flag  ADC12IFG8
  018h   ADC12MEM9  interrupt flag  ADC12IFG9
  01Ah   ADC12MEM10 interrupt flag ADC12IFG10
  01Ch   ADC12MEM11 interrupt flag ADC12IFG11
  01Eh   ADC12MEM12 interrupt flag ADC12IFG12
  020h   ADC12MEM13 interrupt flag ADC12IFG13
  022h   ADC12MEM14 interrupt flag ADC12IFG14
  024h   ADC12MEM15 interrupt flag ADC12IFG15     Lowest
*/

int msp430_adc12_chkifg(void)
{
  int ret = 0;
  if (MCU.adc12.ov)
    {
      MCU.adc12.iv = 2;
      msp430_interrupt_set( INTR_ADC12 );
      return 1;
    }
  if (MCU.adc12.tov)
    {
      MCU.adc12.iv = 4;
      msp430_interrupt_set( INTR_ADC12 );
      return 1;
    }
  if ((MCU.adc12.ifg & MCU.adc12.ie) != 0)
    {
      int i;
      for(i=0; i<ADC12_CHANNELS; i++)
	{
	  if ((MCU.adc12.ifg & MCU.adc12.ie) == (1 << i))
	    {
	      MCU.adc12.iv = 6 + i*2;
	      msp430_interrupt_set( INTR_ADC12 );
	      return 1;
	    }
	}
    }
  return ret;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#endif
