
/**
 *  \file   ui.cpp
 *  \brief  Graphical UI definition using the Qt Toolkit (version 4.7+)
 *  \author David Graeff, based on Antoine Fraboulet work
 *  \date   2005, 2013
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "arch/common/hardware.h"
#include "devices/devices.h"
#include "src/mgetopt.h"
#include "src/options.h"
#ifdef __cplusplus
}
#endif

#include "ui.h"
#include "mainwindow.h"

#define UNUSED __attribute__((unused))  

/**
 * global variables
 **/

#define GUI_DATA_MACHINE   machine.ui

static struct moption_t gui_opt = {
  "ui",
  no_argument,
  "enable GUI",
  0,
  0
};

static struct moption_t title_opt = {
  "ui_title",
  required_argument,
  "GUI window title",
  0, 0
};

int ui_options_add(void)
{
  options_add(& gui_opt            );
  options_add(& title_opt          );
  return 0;
}

int ui_create(int w, int h, int id)
{
  #define MAX_TITLE_SIZE 20
  char name[MAX_TITLE_SIZE];

  if (id != -1)
    {
      if (title_opt.isset && (title_opt.value != NULL))
	{
	  strncpyz(name, title_opt.value, MAX_TITLE_SIZE);
	}
      else
	{
	  snprintf(name,MAX_TITLE_SIZE,"WSim %d",id);
	}
    }
  else
    {
      strncpyz(name, "WSim", MAX_TITLE_SIZE);
    }


  simulationWorker->setWindowData(w, h, name, GUI_DATA_MACHINE.framebuffer_size);
  GUI_DATA_MACHINE.b_up   = 0;
  GUI_DATA_MACHINE.b_down = 0;
  return UI_OK;
}

void ui_delete(void)
{
}

int ui_refresh(int modified)
{
  uint8_t *fb;
  static int loop = 0;

  if (modified)
    {
		simulationWorker->copyBitmap(GUI_DATA_MACHINE.framebuffer);
    }
  return UI_OK;
}

int ui_getevent(void)
{
	if (!simulationWorker->buttonDown && !simulationWorker->buttonUp)
	  return UI_EVENT_NONE;
	else {
		GUI_DATA_MACHINE.b_up = simulationWorker->buttonUp;
		GUI_DATA_MACHINE.b_down = simulationWorker->buttonDown;
		simulationWorker->buttonUp = 0;
		simulationWorker->buttonDown = 0;
		return UI_EVENT_USER;
	}
}

void ui_default_input (const char *name)
{
  int ev;
  switch ((ev = ui_getevent()))
    {
    case UI_EVENT_USER:
      DMSG_LIB_UI("%s: UI event %04x %04x\n", name, machine.ui.b_up,machine.ui.b_down);
      break;

    case UI_EVENT_NONE:
      break;
	
    default:
      ERROR("%s: unknown ui event\n",name);
      break;
    }
}
