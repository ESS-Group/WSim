if BUILD_WSN430

bin_PROGRAMS=wsim-wsn430

AM_CPPFLAGS=-I$(top_srcdir)

WSN430_MCU= ../../arch/msp430/libmsp430f1611.a
WSN430_DEV= ../../devices/led/libled.a			\
		../../devices/ds2411/libds2411.a	\
		../../devices/m25p80/libm25p80.a	\
		../../devices/ptty/libptty.a		\
		../../devices/cc1100_2500/libcc1100.a   \
		../../devices/uigfx/libuigfx.a

## v1: capnet model
wsim_wsn430_CFLAGS=-DMSP430f1611 -DCC1100
wsim_wsn430_SOURCES=wsn430.c
wsim_wsn430_LDADD=${WSN430_DEV} ${WSIMADD} ${WSN430_MCU}
wsim_wsn430_DEPENDENCIES=${wsim_wsn430_LDADD}

endif
