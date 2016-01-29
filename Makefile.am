SUBDIRS = src

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

NULL =

xdgautostartdir = $(sysconfdir)/xdg/autostart
xdgautostart_DATA = xsettings-kde.desktop

# @INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	autogen.sh \
	$(xdgautostart_DATA) \
	m4/ax_require_defined.m4

CLEANFILES = $(xdgautostart_DATA)

DISTCLEANFILES =

MAINTAINERCLEANFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL)

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

-include $(top_srcdir)/git.mk
