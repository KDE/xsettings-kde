bin_PROGRAMS = xsettings-kde

xsettings_kde_SOURCES = \
	xsettings-kde.c \
	xsettings-common.c \
	xsettings-manager.c \
	xsettings-common.h \
	xsettings-kde.h \
	xsettings-manager.h


xsettings_kde_CPPFLAGS = \
	-I$(top_srcdir)					\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

xsettings_kde_CFLAGS = \
	   $(XSETTINGS_KDE_CFLAGS)	\
	   $(WARN_CFLAGS)		\
	   $(AM_CFLAGS)

xsettings_kde_LDFLAGS = \
	$(AM_LDFLAGS)

xsettings_kde_LDADD = \
	$(XSETTINGS_KDE_LIBS)	\
 	$(INTLLIBS)

-include $(top_srcdir)/git.mk
