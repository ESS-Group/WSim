
/**
 *  \file   main.c
 *  \brief  WSim simulator entry point
 *  \author Antoine Fraboulet
 *  \date   2005
 **/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "machine/machine.h"
#include "libconsole/console.h"
#include "libetrace/libetrace.h"
#include "libgdb/libgdb.h"
#include "libgui/ui.h"
#include "libwsnet/libwsnet.h"
#include "src/options.h"
#include "src/revision.h"

#if defined(FUNC_GETRUSAGE_DEFINED)
#include <sys/resource.h>
#endif

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

enum wsim_end_mode_t {
  WSIM_END_NORMAL,
  WSIM_END_SIGNAL,
  WSIM_END_ERROR
};

char* wsim_end_mode_str(enum wsim_end_mode_t mode)
{
  switch (mode)
    {
    case WSIM_END_NORMAL: return "normal";
    case WSIM_END_SIGNAL: return "signal";
    case WSIM_END_ERROR:  return "error";
    default: return "unknown";
    }
  return "unknown";
}

static struct options_t o;
static void main_end(enum wsim_end_mode_t mode);

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void signal_quit(int signum)
{
  INFO("\nwsim: received Unix signal %d (%s)\n",signum,host_signal_str(signum));
  mcu_signal_add(SIG_HOST | signum);
  main_end(WSIM_END_SIGNAL);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void main_dump_stats()
{
#define NANO  (1000*1000*1000)
#define MICRO (1000*1000)
#define MILLI (1000)

  int64_t unanotime = 0;
  /*  int64_t snanotime = -1; */

#if defined(FUNC_GETRUSAGE_DEFINED)
  struct rusage ru;
  getrusage(RUSAGE_SELF,&ru);
  /* explicit cast to prevent overflow */
  /* utime : user time                 */
  /* stime : system time               */
  unanotime = ((uint64_t)ru.ru_utime.tv_sec) * NANO + ((uint64_t)ru.ru_utime.tv_usec) * 1000;
#endif


  OUTPUT("\n");
  OUTPUT_STATS_START("");
  OUTPUT_STATS("WSim stats:\n");
  OUTPUT_STATS("-----------\n");
  if (unanotime > 0)
    {
      int64_t sec  = unanotime / NANO;
      int64_t msec = (unanotime - sec*1000000000) / 1000000;
      OUTPUT_STATS("  simulation user time          : %d.%03d s (%"PRId64" ns)\n", sec, msec, unanotime);
      /* 
       * OUTPUT_BOXM("  system simulation time        : %d.%03d s\n", 
       *	 ru.ru_stime.tv_sec, ru.ru_stime.tv_usec / 1000); 
       */
    }
  OUTPUT_STATS("  simulation backtracks         : %d\n",machine.backtrack); 

  OUTPUT_STATS("\n");
  machine_dump_stats(unanotime); /* system time */
  OUTPUT_STATS_STOP("");
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void  main_run_mode(struct options_t* o)
{
#if !defined(__MINGW32__)
  signal(SIGINT ,signal_quit);
  signal(SIGQUIT,signal_quit);
  signal(SIGUSR1,signal_quit);
  signal(SIGUSR2,signal_quit);
  signal(SIGPIPE,signal_quit); 
#endif
  
  struct machine_opt_t m;
  
  machine_reset();
  machine_state_save();
  
  /* so far so good, run() */
  switch (o->sim_mode)
    {
    case SIM_MODE_CONS:
      console_mode_main();
      break;
    case SIM_MODE_GDB:
      libgdb_target_mode_main(o->gdb_port);
      break;
     
    case SIM_MODE_RUN:
    case SIM_MODE_INSN:
    case SIM_MODE_TIME:
      m.insn     = o->sim_insn;
      m.time     = o->sim_time;
      m.realtime = o->realtime;
      machine_run(&m);
      break;

    default:
      ERROR("** Run mode error\n");
      break;
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void app_exit_error()
{
  INFO ("wsim: error, exit\n");
  ERROR("wsim: error, exit\n");
  exit( EXIT_FAILURE );
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void main_end(enum wsim_end_mode_t mode)
{
  OUTPUT("================\n");
  OUTPUT("== wsim stop  ==\n");
  OUTPUT("================\n");

  main_dump_stats();

  INFO("\n");
  INFO("wsim: end mode %s (%d)\n", wsim_end_mode_str(mode), mode);

  /* simulation done */
  if (o.do_dump)
    {
      INFO("wsim: dump machine state in [%s]\n",o.dumpfile);
      machine_dump(o.dumpfile);
    }

  /* finishing traces */
  if (o.do_trace)
    {
      INFO("wsim: finalize trace in [%s]\n",o.tracefile);
      tracer_close();
    }

  if (o.do_etrace)
    {
      INFO("wsim: finalize etrace in [%s]\n",o.etracefile);
      etracer_close();
    }

  machine_delete();
  worldsens_c_close();
  libselect_close();
  logger_close();
  logpkt_close();
  ui_delete();
  exit( EXIT_SUCCESS );
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#endif

/**
 * main : program entry point
 **/
int startWorker(int argc, char* argv[])
{

#ifdef _WIN32
	{
		WSADATA data;
		WSAStartup(MAKEWORD(2,0), &data);
	}
#endif
#undef ERROR
  /* options */
  options_start();
  ui_options_add();
  machine_options_add();
  options_read_cmdline(&o,&argc,argv);

  /* logger creation                              */
  /* do not use logger functions before that line */
  logger_init(o.logfilename,o.verbose);

  OUTPUT("%s\n",VERSION_STRING());
  OUTPUT("copyright 2005, 2006, 2007, 2008, 2009, 2010, 2011\n");
  OUTPUT("Citi Lab, INRIA, INSA de Lyon\n");
  OUTPUT("\n");
  OUTPUT("wsim: pid %d\n",getpid());

  switch (sizeof(long))
    {
    case 4:
      INFO("wsim: 32-bit edition\n");
      break;
    case 8:
      INFO("wsim: 64-bit edition\n");
      break;
    default:
      INFO("wsim: alien edition\n");
      break;
    }

  if (o.verbose > 1)
    {
      options_print_params(&o);
    }


  /* event tracer */
  tracer_init(o.tracefile, o.wsens_mode);

  /* packet logger */
  logpkt_init(o.do_logpkt, o.logpkt, o.logpktfilename);

  /* libselect init  */
  libselect_init(o.wsens_mode);

  /* etrace */
  etracer_init(o.etracefile, o.wsens_mode);

  /* worldsens initialize */
  worldsens_c_initialize(o.wsens_mode);

  /* machine creation */
  if (machine_create())
    {
      ERROR("\n");
      ERROR("wsim: ** error during machine creation **\n"); 
      ERROR("\n");
      return 1;
    }

  /* set timeref once the machine is created */
  tracer_set_timeref(machine_get_nanotime);

  /* worldsens connect to wsnet server */
  worldsens_c_connect(o.server_ip, o.server_port, o.multicast_ip, o.multicast_port, o.node_id);

  /* preload flash with file */
  if (o.do_preload)
    {
      INFO("wsim: preloading file %s\n",o.preload);
      mcu_hexfile_load(o.preload);
    }

  /* elf loading */
  if (strcmp(o.progname,"none") != 0)
    {
      INFO("wsim: loading file %s\n",o.progname);
      if (machine_load_elf(o.progname,o.verbose))
	{
	  ERROR("wsim: error while loading Elf file\n");
	  machine_delete();
	  return 1;
	}
    }
  else if (o.do_elfload == 0)
    {
      fprintf(stderr,"== Running without elf file \n");
    }
  else if (o.sim_mode == SIM_MODE_GDB)
    {
      fprintf(stderr,"== Running in GDB mode, not loading Elf file\n");
    }
  else 
    {
      ERROR("** Cannot load file, bailing out\n");
      machine_delete();
      machine_exit_error();
    }

  /* GUI */
  if (ui_create(machine.ui.width,machine.ui.height,machine_get_node_id()) != UI_OK)
    {
      ERROR("Cannot create display\n");
      return 3;
    }

  if (o.do_etrace_at_begin)
    {
      INFO("wsim: starting eSimu tracer at wsim start\n");
      etracer_start(); 
    }

  if (o.do_monitor || o.do_modify)
    {
      INFO("wsim: starting memory monitor\n");
      machine_monitor(o.monitor, o.modify);
    }

  /* tracer creation */
  if (o.do_trace)
    {
      INFO("wsim: starting tracer\n");
      tracer_start();
    }

  /* radios packets logger */
  if (o.do_logpkt)
    {
      INFO("wsim: starting packet logger\n");
    }

  /* what has been created so far */
  if (o.verbose > 1)
    {
      OUTPUT("\n");
      machine_print_description();
    }

  /* go */
  OUTPUT("\n");
  OUTPUT("================\n");
  OUTPUT("== wsim start ==\n");
  OUTPUT("================\n");
  main_run_mode(&o);

  main_end(WSIM_END_NORMAL);
  return 0;
}

/**************************************************/
/**************************************************/
/**************************************************/
