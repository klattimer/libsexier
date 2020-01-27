lib_LTLIBRARIES = libsexier-0.1.la

INCLUDES = -DGNOMELOCALEDIR=\""$(datadir)/locale/"\" \
           -DG_LOG_DOMAIN=\"libsexier\" \
           -I$(top_srcdir) \
           -I. \
           $(LIBSEXIER_CFLAGS)


libsexier_0_1_la_SOURCES = fittsmenu.c
libsexier_0_1_la_LIBADD = $(LIBSEXIER_LIBS)

libsexier_0_1_includedir = $(includedir)/libsexier-0.1
libsexier_0_1_include_HEADERS = fittsmenu.h
