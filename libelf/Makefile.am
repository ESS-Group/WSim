

noinst_LIBRARIES = libelf.a

AM_CPPFLAGS = -I$(top_srcdir) 

if BUILD_GNUBFD
RDRSOURCES=libelf_bfd.c
else
RDRSOURCES=libelf_ntv.c
endif

libelf_a_SOURCES = 		\
        libelf.h libelf.c       \
	${RDRSOURCES}
