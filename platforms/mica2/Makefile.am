if BUILD_MICA2

bin_PROGRAMS=wsim-mica2

if BUILD_PTTY
EXTRADEV= ../../devices/ptty/libptty.a
endif

AM_CPPFLAGS=-I$(top_srcdir)

MICA2MCU= ../../arch/atmega/libatmega128l.a
MICA2DEV= ../../devices/led/libled.a			\
		../../devices/ds2411/libds2411.a	\
		../../devices/m25p80/libm25p80.a	\
		${EXTRADEV}

wsim_mica2_CFLAGS=-DATMEGA128L
wsim_mica2_SOURCES=mica2.c
wsim_mica2_LDADD=${MICA2MCU} ${MICA2DEV} ${WSIMADD}  \
	../../machine/libmachine.a ${MICA2MCU} ../../devices/libdevices.a ../../libelf/libelf.a
wsim_mica2_DEPENDENCIES=${wsim_mica2_LDADD}

## WMICA2DEV=../../devices/cc1000/libcc1000.a 	\
##	../../devices/worldsens/libworldsens.a

endif
