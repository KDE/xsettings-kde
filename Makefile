PACKAGE=xsettings-kde
VERSION=0.12.2
lib=lib
TAG := $(shell echo "V$(VERSION)_$(RELEASE)" | tr -- '-.' '__')
SVNROOT = svn+ssh://dmorgan@svn.mageia.org/svn/soft/theme/$(PACKAGE)

CFLAGS= -Wall -g
xsettings_kde_CFLAGS = $(shell pkg-config --cflags x11 gio-2.0 gthread-2.0)

xsettings_kde_LDFLAGS = $(shell pkg-config --libs x11 gio-2.0 gthread-2.0)

manager_objects_CFLAGS = $(shell pkg-config --cflags x11 gio-2.0 gthread-2.0)
manager_objects = xsettings-kde.o xsettings-common.o xsettings-manager.o

.c.o:
	$(CC) -c $(CFLAGS) $(xsettings_kde_CFLAGS) $< -o $@

all: xsettings-kde

xsettings-kde: $(manager_objects)
	$(CC) -o $@ $(manager_objects) $(LDFLAGS) $(xsettings_kde_LDFLAGS)

changelog:
	svn2cl --authors ../common/username.xml --accum
	rm -f ChangeLog.bak
	svn commit -m "Generated by svn2cl the `date '+%c'`" ChangeLog

clean:
	rm -f *.o xsettings-kde

checktag:
	@if [ "x$(VERSION)" == "x" -o "x$(RELEASE)" = "x" ]; then \
 	  echo usage is "make VERSION=version_number RELEASE=release_number dist" ; \
	  exit 1 ; \
	fi

dist: checktag changelog
	svn copy $(SVNROOT)/trunk $(SVNROOT)/tags/$(TAG) -m "$(TAG)"
	svn export $(SVNROOT)/tags/$(TAG) $(PACKAGE)-$(VERSION)
	tar cvf $(PACKAGE)-$(VERSION).tar $(PACKAGE)-$(VERSION)
	bzip2 -9vf $(PACKAGE)-$(VERSION).tar
	rm -rf $(PACKAGE)-$(VERSION)

distcheck: dist
	distdir=$(PACKAGE)-$(VERSION) ;		\
	tar xvfj $$distdir.tar.bz2 &&		\
	cd $$distdir &&				\
	make &&					\
	cd .. &&				\
	rm -rf $$distdir
