bin_PROGRAMS = shell

shell_SOURCES = config.c main.c shellConfig.h

AM_CPPFLAGS = -I$(top_srcdir)/include

shell_LDADD = -L../comLib -L../posterLib -L../portLib \
	-lcomLib -lposterLib -lportLib
