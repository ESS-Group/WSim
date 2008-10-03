
/**
 *  \file   tracer.c
 *  \brief  Simulator activity tracer
 *  \author Antoine Fraboulet
 *  \date   2006
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>   /* basename */
#include <inttypes.h>
#include <errno.h>
#include <inttypes.h>

#include "arch/common/hardware.h"
#include "liblogger/logger.h"
#include "tracer.h"

/****************************************
 * TRACER_USE_BACKTRACK is used to select
 * data log file dump behavior
 *
 * if defined(TRACER_USE_BACKTRACK) 
 *   we can save at each backtrack
 *   this is minimized by recording to memory 
 *   and save only when threshold is reached and
 *   a backtrack occurs
 * else
 *   we can save whenever the threshold is reached
 * 
 ****************************************/
#if defined(WORLDSENS)
#define TRACER_USE_BACKTRACK
#endif

/****************************************
 * DEBUG
 * 
 * DMSG is used for general tracer messages
 * DMSG_TRACER is used for event recording messages
 * ERROR is used for ... errors
 ****************************************/

#if defined(DEBUG)
#undef DEBUG
#endif

#if defined(DEBUG)
#define DMSG(x...) HW_DMSG(x)
#else
#define DMSG(x...) do {} while(0)
#endif

#if defined(DEBUG)
#define DMSG_TRACER(x...) DMSG(x)
#else
#define DMSG_TRACER(x...) do { } while (0)
#endif

/****************************************
 * For performance purpose and because this
 * is an early version most of the tracer
 * dimensions are fixed
 ****************************************/

#define TRACER_MAX_ID                30
#define TRACER_MAX_NAME_LENGTH       30

/*
 *  64ksamples = 750kB
 * 128ksamples = 1.5MB
 * 256ksamples = 3MB
 *
 * recording blocks are   256ksamples * 12bytes = 3MB
 * recording threshold is 128ksamples * 12bytes = 1.5MB
 */

#define TRACER_BLOCK_EV              (256*1024)
#define TRACER_BLOCK_THRESHOLD       (128*1024)

/****************************************
 * struct _sample_t is the sample type
 * that is recorded. Its size should be 
 * 12 bytes  (96 bits)
 ****************************************/

#define PACKED __attribute__((packed))

struct PACKED _tracer_sample_t {
  tracer_id_t    id;   /* 32 Bits */
  tracer_time_t  time; /* 64 bits */
  tracer_val_t   val;  /* 64 bits */
};
typedef struct _tracer_sample_t tracer_sample_t;

/****************************************
 * tracer datafile id and version
 * information
 ****************************************/

#define TRACER_MAGIC        "Worldsens tracer datafile"
#define TRACER_MAGIC_SIZE   26
/* version 0: 
      use unpacked struct
      magic_size == 27
   version 1:
      packed struct
      magic_size == 26
*/
#define TRACER_VERSION      2

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

struct _tracer_state_t {
  /* events : these values are updated during record                     */
  /* these values are for the current block                              */
  tracer_ev_t      ev_count;                   /* event counter (index)  */
  tracer_val_t     id_val    [TRACER_MAX_ID];  /* current id value       */
  /* these value are for the complete trace                              */
  tracer_ev_t      ev_count_total;             /* global counter         */
  tracer_ev_t      id_count  [TRACER_MAX_ID];  /* count for each id      */
  tracer_val_t     id_val_min[TRACER_MAX_ID];  /* id_min                 */
  tracer_val_t     id_val_max[TRACER_MAX_ID];  /* id max                 */
};
typedef struct _tracer_state_t tracer_state_t;

/* tracer state and its backup */
static tracer_state_t tracer_current;
static tracer_state_t tracer_saved;

#define EVENT_TRACER tracer_current

/* values that are not saved during a state_save */
/* these values will be used to build the header */
static int32_t                 tracer_node_id      = 0xFFFF;
static unsigned char           tracer_max_id       = TRACER_MAX_ID;
static char                   *tracer_filename     = NULL;
static char                    tracer_id_name        [TRACER_MAX_ID][TRACER_MAX_NAME_LENGTH];
static char                    tracer_id_module      [TRACER_MAX_ID][TRACER_MAX_NAME_LENGTH];
static uint8_t                 tracer_width          [TRACER_MAX_ID];
static get_nanotime_function_t tracer_get_nanotime = NULL;
static int                     tracer_init_done    = 0;
static FILE*                   tracer_datafile     = NULL;
static tracer_sample_t         tracer_buffer[TRACER_BLOCK_EV];

/* block access macro */
#define tracer_end_of_block(e)   ((e & TRACER_BLOCK_EV) == TRACER_BLOCK_EV)

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/* start/stop recording event */
void  (*tracer_event_record_ptr) (tracer_id_t id, tracer_val_t val);
static void tracer_event_record_active(tracer_id_t id, tracer_val_t val);
static void tracer_event_record_time_nocheck(tracer_id_t id, tracer_val_t val, tracer_time_t time);

static void tracer_dump_data(void);
static void tracer_dump_header(void);

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void
tracer_dump_header()
{
  char      v,e;
  uint32_t  i = 0;
  tracer_time_t t = 0;
  uint64_t  cycles;
  uint64_t  time;
  uint64_t  insn;
  uint32_t  size;


  rewind(tracer_datafile);
  
  v = TRACER_VERSION;
#if defined(WORDS_BIGENDIAN)
  e = 0;
#else
  e = 1;
#endif

  cycles = mcu_get_cycles();
  time   = MACHINE_TIME_GET_NANO();
  insn   = mcu_get_insn();

  /* magic */
  size = sizeof(TRACER_MAGIC);
  i   += fwrite(TRACER_MAGIC,1,size,tracer_datafile);
  DMSG("tracer:hdr:magic    : %s (%d)\n",TRACER_MAGIC,size);

  /* version */
  i += fwrite(&v,1,1,tracer_datafile);
  DMSG("tracer:hdr:version  : %d\n",v);

  /* endianess */
  i += fwrite(&e,1,1,tracer_datafile);
  DMSG("tracer:hdr:endian   : %d\n",e);

  /* backtracks uint32_t */
  size = sizeof(machine.backtrack);
  i   += fwrite(&machine.backtrack,size,1,tracer_datafile);
  DMSG("tracer:hdr:backtrack: %d\n",machine.backtrack);

  /* simulated cycles */
  size = sizeof(cycles);
  i   += fwrite(&cycles,size,1,tracer_datafile);
  DMSG("tracer:hdr:cycles   : %"PRId64"\n",cycles);

  /* simulated nanoseconds */
  size = sizeof(time);
  i   += fwrite(&time,size,1,tracer_datafile);
  DMSG("tracer:hdr:nano     : %"PRId64"\n",time);
  
  /* simulated instructions */
  size = sizeof(insn);
  i   += fwrite(&insn,size,1,tracer_datafile);
  DMSG("tracer:hdr:insn     : %"PRId64"\n",insn);
  
  /* tracer node id   */
  size = sizeof(tracer_node_id);
  i   += fwrite(&tracer_node_id,size,1,tracer_datafile);
  DMSG("tracer:hdr:node id  : %d\n",tracer_node_id);

  /* max number of id */
  size = sizeof(tracer_max_id);
  i   += fwrite(&tracer_max_id,1,size,tracer_datafile);
  DMSG("tracer:hdr:max id   : %d (%d)\n",tracer_max_id,size);

  /* number of events */
  size = sizeof(EVENT_TRACER.ev_count_total);
  i   += fwrite(&EVENT_TRACER.ev_count_total,1,size,tracer_datafile);
  DMSG("tracer:hdr:number   : %d (%d)\n",EVENT_TRACER.ev_count_total,size);

  /* end simulation time */
  if (tracer_get_nanotime != NULL)
    {
      t = tracer_get_nanotime();
    }
  else
    {
      t = 0;
    }

  size = sizeof(tracer_time_t);
  i   += fwrite(&t,1,size,tracer_datafile);
  DMSG("tracer:hdr:nano     : %" PRId64 " (%d)\n",t,size);

  /* events name */
  size = sizeof(tracer_id_name);
  i   += fwrite(tracer_id_name,1,size,tracer_datafile);
  DMSG("tracer:hdr:events   : name %d bytes\n",size);

  /* events width */
  size = sizeof(tracer_width);
  i   += fwrite(tracer_width, 1, size, tracer_datafile);
  DMSG("tracer:hdr:events   : width %d bytes\n",size);

  /* id_count, id_min, id_max */
  size = sizeof(tracer_ev_t)  * TRACER_MAX_ID;
  i   += fwrite(EVENT_TRACER.id_count  , 1, size, tracer_datafile);
  size = sizeof(tracer_val_t) * TRACER_MAX_ID;
  i   += fwrite(EVENT_TRACER.id_val_min, 1, size, tracer_datafile);
  size = sizeof(tracer_val_t) * TRACER_MAX_ID;
  i   += fwrite(EVENT_TRACER.id_val_max, 1, size, tracer_datafile);

  DMSG("tracer:header: total %d bytes\n",i);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void
tracer_init(char *filename, get_nanotime_function_t f)
{
  int id;
  uint32_t size;

  if (filename == NULL)
    return ;
    
  tracer_get_nanotime = f;
  tracer_filename     = strdup(filename);
  for(id=0; id < TRACER_MAX_ID; id++)
    {
      tracer_id_name   [id][0]    = '\0';
      tracer_id_module [id][0]    = '\0';
      EVENT_TRACER.id_count[id]   = 0;
      EVENT_TRACER.id_val_min[id] = 0;
      EVENT_TRACER.id_val_max[id] = 0;
    }
  EVENT_TRACER.ev_count       = 0;
  EVENT_TRACER.ev_count_total = 0;;
  tracer_stop();

  if ((tracer_datafile = fopen(filename,"w")) == NULL)
    {
      ERROR("tracer: ***********************************\n");
      ERROR("tracer: %s\n",strerror(errno));
      ERROR("tracer: ***********************************\n");
      return;
    }
  /* write an empty header that will be fixed later */
  tracer_dump_header();

  tracer_init_done = 1;
  size = sizeof(tracer_sample_t);
  DMSG("tracer: init ok, sample size = %d\n",size);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void
tracer_close(void)
{
  if (tracer_filename == NULL)
    return;
  if (tracer_datafile == NULL)
    return;
  /* tracer_dump_data */
  tracer_dump_data();
  tracer_dump_header();
  fclose(tracer_datafile);
  DMSG("tracer: close ok\n");
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void
tracer_start(void)
{
  if (tracer_init_done == 1)
    {
      tracer_event_record_ptr = tracer_event_record_active;
    }
  else
    {
      ERROR("tracer: attempt to start the tracer before init\n");
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void tracer_stop(void)
{
  tracer_event_record_ptr = NULL;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void
tracer_event_add_id(tracer_id_t id, int width, char* label, char* module)
{
  if (id >= (TRACER_MAX_ID - 1))
    {
      ERROR("tracer: max event recording reached, could not register [%s] = %d\n",label,id);
      machine_exit(1);
    }

  if ((label == NULL) || (strlen(label) == 0))
    {
      ERROR("tracer: event id %d must have a valid name (non null)\n",id);
      machine_exit(1);
    }

  if ((width < 1) || (width > 64))
    {
      ERROR("tracer: event id %d must have 0 < width < 65\n",id);
    }

  if (tracer_id_name[id][0] != 0)
    {
      ERROR("tracer: event id %d for %s is already used by event %s\n",id,label,tracer_id_name[id]);
      machine_exit(1);
    }
  
  strncpy(tracer_id_name   [id], label,  TRACER_MAX_NAME_LENGTH);
  strncpy(tracer_id_module [id], module, TRACER_MAX_NAME_LENGTH);
  tracer_width[id] = width;

  DMSG_TRACER("tracer:add:id: %s=%d module=%s\n",label,id,module);
  tracer_event_record_time_nocheck(id,0,0);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static inline void 
tracer_set_event(tracer_ev_t e, tracer_id_t id, tracer_time_t time, uint64_t val)
{
  tracer_buffer[e].id   = id;
  tracer_buffer[e].time = time;
  tracer_buffer[e].val  = val;
} 

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void 
tracer_dump_data()
{
  int written;
  int32_t size;

  size    = sizeof(tracer_sample_t)*EVENT_TRACER.ev_count;
  written = fwrite(tracer_buffer, 1, size, tracer_datafile);
  assert(written == size);
  DMSG("tracer:data:dump: write %d ev = %d bytes (%d requested)\n",EVENT_TRACER.ev_count,written,size);
  EVENT_TRACER.ev_count = 0;
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#define min(a,b)  ((a < b) ? a : b)
#define max(a,b)  ((a < b) ? b : a)

static void
tracer_event_record_time_nocheck(tracer_id_t id, tracer_val_t val, tracer_time_t time)
{
  if (! tracer_end_of_block(EVENT_TRACER.ev_count))
    {
      tracer_set_event(EVENT_TRACER.ev_count,id,time,val);

      EVENT_TRACER.ev_count       += 1;
      EVENT_TRACER.id_val[id]      = val;

      EVENT_TRACER.ev_count_total += 1;
      EVENT_TRACER.id_count[id]   += 1;
      EVENT_TRACER.id_val_min[id]  = min(EVENT_TRACER.id_val_min[id],val);
      EVENT_TRACER.id_val_max[id]  = max(EVENT_TRACER.id_val_max[id],val);

      if (tracer_end_of_block(EVENT_TRACER.ev_count))
	{
	  ERROR("tracer:error: max event reached\n");
	}
    }
  DMSG_TRACER("tracer:add:event: [%s] = (%" PRId64 ",%" PRId64 ")\n",tracer_id_name[id],time,val);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void 
tracer_event_record_active(tracer_id_t id, tracer_val_t val)
{
  /* we record only value change to limit trace size */
  if (val != EVENT_TRACER.id_val[id])
    {
      uint64_t time = tracer_get_nanotime();
      tracer_event_record_time_nocheck(id,val,time);
    }
#if !defined(TRACER_USE_BACKTRACK)
  if (EVENT_TRACER.ev_count > TRACER_BLOCK_THRESHOLD)
    {
      tracer_dump_data();
    }
#endif
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void
tracer_state_save(void)
{
  if (EVENT_TRACER.ev_count > TRACER_BLOCK_THRESHOLD)
    {
      tracer_dump_data();
    }
  memcpy(&tracer_saved, &tracer_current, sizeof(tracer_state_t));
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void
tracer_state_restore(void)
{
  memcpy(&tracer_current, &tracer_saved, sizeof(tracer_state_t));
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void tracer_set_node_id(int id)
{
  tracer_node_id = id;
  DMSG_TRACER("tracer:data: node_id to %d (0x%04x)\n",tracer_node_id,tracer_node_id);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */