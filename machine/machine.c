
/**
 *  \file   machine.c
 *  \brief  Platform Machine definition
 *  \author Antoine Fraboulet
 *  \date   2005
 **/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>

#include "arch/common/hardware.h"
#include "devices/devices.h"
#include "libelf/libelf.h"
#include "libetrace/libetrace.h"
#include "liblogger/logger.h"
#include "src/common.h"

#include "machine_mon.h"
#include "machine.h"

/* Record internal events
 *
 * TIMESTAMP : log an event every WSIM_RECORD_INTERNAL_TIME_PREC, 
 *             the event is recorded in seconds * 10
 * BACKTRACK : log an event every backtrack, event is backtrack count
 * REALTIME  : log realtime mode delta sleeps
 * LOGWRITE  : log every time log files are processed (unused at this stage)
 *
 */

#define WSIM_RECORD_INTERNAL_EVENTS 1
#define WSIM_RECORD_INTERNAL_TIME_PREC_SECONDS 0.5 /* seconds */
#define WSIM_RECORD_INTERNAL_TIME_PREC (WSIM_RECORD_INTERNAL_TIME_PREC_SECONDS * 1000 * 1000 * 1000)

#if WSIM_RECORD_INTERNAL_EVENTS != 0
#define MACHINE_TRC_TIMESTAMP()            tracer_event_add_id(32, "timestamp",  "wsim")
#define MACHINE_TRC_TIMESTAMP_RECORD()     tracer_event_record(machine.timestamp_trc, \
							       machine.state->timestamp / (1000*1000*100))
#define MACHINE_TRC_BACKTRACK()            tracer_event_add_id(32, "backtrack",  "wsim")
#define MACHINE_TRC_BACKTRACK_RECORD()     tracer_event_record(machine.backtrack_trc, machine.backtrack)
#define MACHINE_TRC_REALTIME()             tracer_event_add_id(32, "realtime",  "wsim")
#define MACHINE_TRC_REALTIME_RECORD(delta) tracer_event_record(machine.realtime_trc, delta)
#define MACHINE_TRC_LOGWRITE()             tracer_event_add_id(32, "logwrite",  "wsim")
#define MACHINE_TRC_LOGWRITE_RECORD(delta) tracer_event_record(machine.logwrite_trc, delta)
#else
#define MACHINE_TRC_TIMESTAMP()            0
#define MACHINE_TRC_TIMESTAMP_RECORD()     do { } while (0)
#define MACHINE_TRC_BACKTRACK()            0
#define MACHINE_TRC_BACKTRACK_RECORD()     do { } while (0)
#define MACHINE_TRC_REALTIME()             0
#define MACHINE_TRC_REALTIME_RECORD(delta) do { } while (0)
#define MACHINE_TRC_LOGWRITE()             0
#define MACHINE_TRC_LOGWRITEL_RECORD(delta) do { } while (0)
#endif

/*
 * global variable and defines
 */
struct machine_t  machine;
static elf32_t    machine_elf  = NULL;

/*
 * wsim run modes
 */
#define LIMIT_REALTIME 0x01
#define LIMIT_TIME     0x02
#define LIMIT_INSN     0x04

/*
 * realtime adjustment precision
 */
#define WSIM_REALTIME_PRECISION_SECONDS 0.05
#define WSIM_REALTIME_PRECISION         (WSIM_REALTIME_PRECISION_SECONDS * 1000 * 1000 * 1000)

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void     machine_framebuffer_allocate (void);
void     machine_framebuffer_free     (void);

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int machine_options_add()
{
  int res = 0;
  /* add options (MCU)                              */
  res += mcu_options_add();
  /* add all devices options = Board + peripherals) */
  res += devices_options_add();
  return res;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int machine_create()
{
  int res = 0;

#if defined(DEBUG_MEMFOOTPRINT)
  HW_DMSG_MISC("machine: internal memory size is %d bytes\n",(int)sizeof(struct machine_t));
  HW_DMSG_MISC("  machine_info_t : %d\n",(int)sizeof(struct machine_info_t));
  HW_DMSG_MISC("  device_t       : %d\n",(int)sizeof(struct device_t));
  HW_DMSG_MISC("  tracer_t       : %d\n",(int)sizeof(tracer_t));
  HW_DMSG_MISC("  ui_t           : %d\n",(int)sizeof(struct ui_t));
#endif

  /* zero memory */
  machine.state                       = NULL;
  machine.state_backup                = NULL;
  machine.nanotime_incr               = 0;
  machine.run_limit                   = 0;
  machine.run_time                    = 0;
  machine.run_insn                    = 0;
  machine.device_max                  = 0;
  memset(machine.device,      '\0',sizeof(struct device_t)*DEVICE_MAX);
  memset(machine.device_size, '\0',sizeof(int)*DEVICE_MAX);

  /* create tracer events */
  machine.backtrack                   = 0;
  machine.backtrack_trc               = MACHINE_TRC_BACKTRACK();
  machine.timestamp_trc               = MACHINE_TRC_TIMESTAMP();
  machine.realtime_trc                = MACHINE_TRC_REALTIME();
  machine.logwrite_trc                = MACHINE_TRC_LOGWRITE();

  machine.ui.framebuffer_background   = 0;

  /* create devices = mcu + peripherals */
  res += devices_create();
  machine_framebuffer_allocate();

  tracer_set_node_id(machine_get_node_id());
  return res;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int machine_delete()
{
  int ret = 0;

  ret +=  devices_delete();
  machine_state_free();
  machine_framebuffer_free();
  libelf_close(machine_elf);

  return ret;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int machine_reset()
{
  int res = 0;
  mcu_reset();
  res += devices_reset();
  return res;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int machine_dump_mem(FILE* out, uint8_t *ptr, uint32_t addr, uint32_t size)
{
  int i;
  uint32_t col;
  uint32_t laddr;

#define DUMP_COLS 8

  for(laddr=addr; laddr < (addr+size);  laddr+=(2 * DUMP_COLS))
    {
      fprintf(out,"%04ux  ",(unsigned)laddr & 0xffff);

      for(i=0; i<2; i++)
	{
	  for(col = 0; col < DUMP_COLS; col ++)
	    {
	      fprintf(out,"%02x ",ptr[laddr + i*DUMP_COLS + col]);
	    }
	  fprintf(out," ");
	}

      fprintf(out,"|");
      for(col = 0; col < 2*DUMP_COLS; col ++)
	{
	  char c = ptr[laddr + col];
	  fprintf(out,"%c",(isprint(c) ? c : '.'));
	}
      fprintf(out,"|\n");

    }

  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int machine_dump(const char *filename)
{
  FILE *file;

  HW_DMSG_MISC("machine: dump state to file %s\n",filename);
  if ((file = fopen(filename, "wb")) == NULL)
    {
      ERROR("machine: cannot dump state to file %s\n",filename);
      return 1;
    }

  fclose(file);
  return 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int machine_get_node_id(void)
{
  return worldsens_c_get_node_id();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void machine_exit_error()
{
  INFO("wsim:machine: exit error\n");
  app_exit_error();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static inline wsimtime_t system_gettime()
{
  struct timeval t;
  wsimtime_t ntime;
  gettimeofday(&t, NULL);
  ntime = t.tv_sec * 1000 * 1000 + t.tv_usec; // us
  return ntime;
}

static inline void system_waitmicro(int64_t micro)
{
  /*
  struct timespec t;
  t.tv_sec  = micro / 1000000;
  t.tv_nsec = micro * 1000;
  nanosleep(&t);
  */
  usleep(micro);
}

static inline uint32_t machine_run_internal(void)
{
  uint32_t sig;
  wsimtime_t realtime_last_wsim;
  wsimtime_t realtime_last_sys;
  
  mcu_system_clock_speed_tracer_update();
  machine_state_save();

  realtime_last_wsim = MACHINE_TIME_GET_NANO();
  realtime_last_sys  = system_gettime();

  do {
    /* run */
    mcu_run();         // MCU step and devices
    devices_update();  // call platform devices
    mcu_update_done(); // MCU step done
    MACHINE_TIME_CLR_INCR();
    sig = mcu_signal_get();

    /* memory access control MAC */
    if ( (sig & SIG_MAC) != 0 )
      {
	mcu_signal_remove(SIG_MAC);
	if ((sig & MAC_TO_SIG(MAC_MUST_WRITE_FIRST)) != 0)
	  {
	    WARNING("wsim: ========================================\n");
	    WARNING("wsim: Read before Write from PC=0x%04x - 0x%x - %s\n", 
		    mcu_get_pc(),sig,mcu_signal_str());
	    WARNING("wsim: ========================================\n");
	    mcu_signal_remove(MAC_TO_SIG(MAC_MUST_WRITE_FIRST));
	    sig = mcu_signal_get();
	  }

	if ((sig & MAC_TO_SIG(MAC_WATCH_READ)) != 0)
	  {
	    mcu_signal_remove(SIG_MAC | MAC_TO_SIG(MAC_WATCH_READ));
	    machine_monitor_add_trace(MAC_WATCH_READ);
	    sig = mcu_signal_get();
	  }

	if ((sig & MAC_TO_SIG(MAC_WATCH_WRITE)) != 0)
	  {
	    mcu_signal_remove(SIG_MAC | MAC_TO_SIG(MAC_WATCH_WRITE));
	    machine_monitor_add_trace(MAC_WATCH_WRITE);
	    sig = mcu_signal_get();
	  }
      }
WARNING("run\n");
    /* time and instruction counters */
    if (machine.run_limit != 0)
      {
	if ((machine.run_limit & LIMIT_INSN) && (mcu_get_cycles() >= machine.run_insn))
	  {
	    mcu_signal_add(SIG_RUN_INSN);
	    sig = mcu_signal_get();
	    return sig;
	  }

	if ((machine.run_limit & LIMIT_TIME) && (MACHINE_TIME_GET_NANO() >= machine.run_time))
	  {
	    mcu_signal_add(SIG_RUN_TIME);
	    sig = mcu_signal_get();
	    return sig;
	  }

	if ((machine.run_limit & LIMIT_REALTIME) && 
	    ((MACHINE_TIME_GET_NANO() - realtime_last_wsim) > WSIM_REALTIME_PRECISION))
	  {
	    int64_t delta;
	    wsimtime_t delta_wsim;
	    wsimtime_t delta_sys;
	    delta_wsim = MACHINE_TIME_GET_NANO() - realtime_last_wsim;
	    delta_sys  = system_gettime() - realtime_last_sys;
	    delta      = delta_wsim / 1000 - delta_sys;
	    if (delta > 0)
	      {
		/* catch up */
		system_waitmicro( delta ); // microseconds
	      }
	    else
	      {
		/* wsim is late */
		HW_DMSG_MISC("wsim: realtime mode behind schedule by %05d usec (%03d ms) at %"PRIu64"\n", 
		      -delta,  -delta / 1000, MACHINE_TIME_GET_NANO() / 1000000);
		
	      }
	    MACHINE_TRC_REALTIME_RECORD(delta);
	    realtime_last_wsim = MACHINE_TIME_GET_NANO();
	    realtime_last_sys  = system_gettime();
	  }
      }

    if ((MACHINE_TIME_GET_NANO() - machine.state->timestamp) > WSIM_RECORD_INTERNAL_TIME_PREC)
      {
	machine.state->timestamp = MACHINE_TIME_GET_NANO();
	MACHINE_TRC_TIMESTAMP_RECORD();
      }

  } while (sig == 0);

  return sig;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void machine_run(struct machine_opt_t *m)
{
  uint32_t UNUSED sig;

  machine.run_realtime = 0;
  machine.run_time     = 0;
  machine.run_insn     = 0;

  if (m != NULL)
    {
      if (m->realtime) 
	{
	  machine.run_limit   |= LIMIT_REALTIME;
	  HW_DMSG_MISC("machine: will run realtime\n");
	}
      
      if (m->insn > 0)
	{
	  machine.run_limit  |= LIMIT_INSN;
	  machine.run_insn    = m->insn;
	  HW_DMSG_MISC("machine: will run for %" PRIu64 " instructions\n",machine.run_insn);
	}
      
      if (m->time > 0)
	{
	  machine.run_limit  |= LIMIT_TIME;
	  machine.run_time    = m->time;
	  HW_DMSG_MISC("machine: will run for %" PRIu64 " nano seconds\n",machine.run_time);
	}
    }

  sig = machine_run_internal();
  
  HW_DMSG_MISC("machine:run: stopped at 0x%04x with signal 0x%x=%s\n",mcu_get_pc(),sig,mcu_signal_str());
  /*
   * Allowed outside tools signals
   *    SIG_GDB | SIG_CONSOLE | SIG_WORLDSENS_IO 
   */
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

int machine_load_elf(const char* filename, int verbose_level)
{
  machine_elf = libelf_load(filename, verbose_level);
  return machine_elf == NULL;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void machine_monitor(char* monitor, char *modify)
{
  if (machine_elf)
    {
      machine_monitor_init(machine_elf);
    }

  if (monitor)
    {
      machine_monitor_set(monitor, machine_elf);
    }

  if (modify)
    {
      machine_modify_set(modify, machine_elf);
    }

  machine_monitor_start();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void machine_print_description()
{
  OUTPUT_BOXS("");
  OUTPUT_BOXM("Machine description\n");
  mcu_print_description();
  devices_print();
  OUTPUT_BOXE("");
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

inline wsimtime_t machine_get_nanotime()
{
  return MACHINE_TIME_GET_NANO();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

uint8_t* machine_state_allocate(int devices_size)
{
  uint8_t *ptr;
  int machine_size = sizeof(struct machine_state_t);

  /* machine.state_size_devices must be assigned */
  machine.state_size = machine_size + devices_size;

  if ((machine.state        = (struct machine_state_t*)malloc(machine.state_size)) == NULL)
    {
      ERROR("** Could not allocate memory for devices states storage\n");
      return NULL;
    }

  if ((machine.state_backup = (struct machine_state_t*)malloc(machine.state_size)) == NULL)
    {
      machine_state_free();
      ERROR("** Could not allocate memory for devices states storage backup\n");
      return NULL;
    }

  memset(machine.state,        0, machine.state_size);
  memset(machine.state_backup, 0, machine.state_size);

  ptr = (uint8_t*)machine.state;
  return ptr + machine_size;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void machine_state_free(void)
{
  if (machine.state != NULL)
    {
      free(machine.state);
      machine.state = NULL;
    }
  if (machine.state_backup != NULL)
    {
      free(machine.state_backup);
      machine.state_backup = NULL;
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void machine_state_save()
{
  /* mcu              */
  mcu_state_save();
  /* devices          */
  memcpy(machine.state_backup, machine.state, machine.state_size);
  /* libselect        */
  libselect_state_save();
  /* event tracer     */
  tracer_state_save();
  /* esimu tracer     */
  etracer_state_save();
  /* pkt logger       */
  logpkt_state_save();
  /* libwsnet         */
  worldsens_c_state_save();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void machine_state_restore()
{
  /* mcu              */
  mcu_state_restore();
  /* devices          */
  memcpy(machine.state, machine.state_backup, machine.state_size);
  /* libselect        */
  libselect_state_restore();
  /* event tracer     */
  tracer_state_restore();
  /* esimu tracer     */
  etracer_state_restore();
  /* pkt logger       */
  logpkt_state_restore();
  /* libwsnet         */
  worldsens_c_state_restore();
  /* count backtracks */
  machine.backtrack ++;
  MACHINE_TRC_BACKTRACK_RECORD();
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void machine_framebuffer_allocate(void)
{
  int i,x,y,w,h;
  machine.ui.width  = 0;
  machine.ui.height = 0;
  machine.ui.bpp    = 3; // verif with ui.c
  
  for(i=0; i<machine.device_max; i++)
    {
      x = 0;
      y = 0;
      w = 0;
      h = 0;

      if (machine.device[i].ui_get_pos)
	{
	  machine.device[i].ui_get_pos (i,&x,&y);
	  if (machine.device[i].ui_get_size)
	    {
	      machine.device[i].ui_get_size(i,&w,&h);
	      machine.ui.height  = ((y+h) > machine.ui.height) ? (y+h) : machine.ui.height;
	      machine.ui.width   = ((x+w) > machine.ui.width)  ? (x+w) : machine.ui.width;
	    }
	}

    }

  machine.ui.framebuffer_size = machine.ui.width * machine.ui.height * machine.ui.bpp;
  //  VERBOSE(3,"machine:framebuffer: size %dx%dx%d\n",machine.ui.width, machine.ui.height, machine.ui.bpp);
  //  VERBOSE(3,"machine:framebuffer: size %d\n", machine.ui.framebuffer_size);
  //  VERBOSE(3,"machine:framebuffer: background 0x%08x\n",machine.ui.framebuffer_background);

  if (machine.ui.framebuffer_size > 0) 
    {
      uint8_t r,g,b;
      r = (machine.ui.framebuffer_background >> 16) & 0xff;
      g = (machine.ui.framebuffer_background >>  8) & 0xff;
      b = (machine.ui.framebuffer_background >>  0) & 0xff;
      //  VERBOSE(3,"machine:framebuffer:bg: 0x%02x%02x%02x\n",r,g,b);

      if ((machine.ui.framebuffer = (uint8_t*)malloc(machine.ui.framebuffer_size)) == NULL)
	{
	  HW_DMSG_DEV("Cannot allocate framebuffer\n");
	}
      for(i=0; i < (int)machine.ui.framebuffer_size; i+=machine.ui.bpp)
	{
	  setpixel(i, r,g,b);
	}
    }
}

/**************************************************/
/**************************************************/
/**************************************************/

void machine_framebuffer_free(void)
{
  if (machine.ui.framebuffer)
    {
      free(machine.ui.framebuffer);
    }
}

/**************************************************/
/**************************************************/
/**************************************************/

#define NANO  (1000*1000*1000)
#define MILLI (1000*1000)
#define MICRO (1000)

void machine_dump_stats(uint64_t user_nanotime)
{
  uint64_t s;
  uint64_t ms;
  uint64_t UNUSED us;
  float speedup;

  s         = MACHINE_TIME_GET_NANO() / NANO;
  ms        = ( MACHINE_TIME_GET_NANO() - (s * NANO)) / MILLI;
  us        = ( MACHINE_TIME_GET_NANO() - (s * NANO)) / MICRO;

  OUTPUT_STATS("Machine stats:\n");
  OUTPUT_STATS("--------------\n");
  OUTPUT_STATS("  simulated time                : %"PRId64" ns (%"PRId64".%03"PRId64" s)\n", 
              MACHINE_TIME_GET_NANO(),s,ms);
  if (user_nanotime > 0)
    {
      speedup = (float)MACHINE_TIME_GET_NANO() / (float)user_nanotime;
      OUTPUT_STATS("  simulation speedup            : %2.2f\n",speedup);
    }
  OUTPUT_STATS("  machine exit with signal      : %s\n",mcu_signal_str());
  OUTPUT_STATS("\n");
  OUTPUT_STATS("MCU:\n");
  OUTPUT_STATS("----\n");
  mcu_dump_stats(user_nanotime);
  OUTPUT_STATS("\n");
  OUTPUT_STATS("DEVICES:\n");
  OUTPUT_STATS("--------\n");
  devices_dump_stats(user_nanotime);
}

/**************************************************/
/**************************************************/
/**************************************************/
