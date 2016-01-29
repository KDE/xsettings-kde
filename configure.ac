AC_PREREQ(2.63)
AC_INIT([XSettings KDE],[0.12.3],[mailto:desktop-devel-list@gnome.org],[xsettings-kde])
AX_IS_RELEASE([git-directory])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.10 foreign dist-xz tar-ustar no-dist-gzip])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Check for programs
AX_CHECK_ENABLE_DEBUG
AC_PROG_CC

# Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT

dnl AX_COMPILER_FLAGS([WARN_CFLAGS],[WARN_LDFLAGS])

# ***************************
# Check for required packages
# ***************************

PKG_CHECK_MODULES([XSETTINGS_KDE],
   [x11 gio-2.0 gthread-2.0])

# *************
# Documentation
# *************

dnl AX_REQUIRE_DEFINED([YELP_HELP_INIT])
dnl YELP_HELP_INIT

# ********************
# Internationalisation
# ********************

dnl IT_PROG_INTLTOOL([0.40.0])

dnl AC_SUBST([GETTEXT_PACKAGE], [gnome-hello])
dnl AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], "$GETTEXT_PACKAGE", [Define to the gettext package name.])

# *************************************
# *************************************

AC_CONFIG_FILES([
Makefile
src/Makefile
])

AC_OUTPUT
