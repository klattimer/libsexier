EXTRA_DIST = \
  icon_cursor.svg icon_droplet.svg icon_freehand.svg icon_nodes.svg \
  icon_spiral.svg  icon_text.svg icon_calig.svg icon_curves.svg icon_ellipse.svg \
  icon_gradient.svg icon_rectangle.svg icon_star.svg

noinst_PROGRAMS = fittsmenu

fittsmenu_LDADD = $(LIBSEXIER_LIBS)

fittsmenu_CFLAGS = \
	-DEXAMPLES_DATA_PATH=\"$(abs_srcdir)/\" \
	-I$(top_srcdir)/libsexier \
	$(LIBSEXIER_CFLAGS) \
	$(top_builddir)/libsexier/libsexier-0.1.la
