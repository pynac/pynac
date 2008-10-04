## Process this file with automake to produce Makefile.in
SUBDIRS = ginac 
DIST_SUBDIRS = ginac

# pkg-config metadata
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pynac.pc
$(pkgconfig_DATA): config.status

# All the rest of the distributed files
EXTRA_DIST = pynac.pc pynac.spec

# Rule to build tar-bzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpmbuild -ta --clean $(PACKAGE)-$(VERSION).tar.gz
