#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.

PLUGIN = webbridge

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

# thirdparty
U_SOCKETS_VERSION=0.8.8
U_WEBSOCKETS_VERSION=20.65.0
U_LIBSOCKET_VERSION=2.4

U_SOCKETS_DIR=thirdparty/uSockets-$(U_SOCKETS_VERSION)
U_SOCKETS_LIB=$(U_SOCKETS_DIR)/uSockets.a
U_SOCKETS_INCLUDE=$(U_SOCKETS_DIR)/src
U_WEBSOCKETS_INCLUDE=thirdparty/uWebSockets-$(U_WEBSOCKETS_VERSION)/src

U_LIBSOCKET_INCLUDE=thirdparty/libsocket-$(U_LIBSOCKET_VERSION)/headers
U_LIBSOCKET_OBJS_C=thirdparty/libsocket-$(U_LIBSOCKET_VERSION)/C/unix/libunixsocket.o thirdparty/libsocket-$(U_LIBSOCKET_VERSION)/C/inet/libinetsocket.o

U_TINY_PROCESS_LIBRARY_INCLUDE=thirdparty/tiny-process-library
U_TINY_PROCESS_LIBRARY_OBJ=thirdparty/tiny-process-library/process_unix.o thirdparty/tiny-process-library/process.o

# ffmpeg
FFMPEG_CFLAGS += $(shell pkg-config --cflags libavutil libavcodec libavdevice libavformat libswresample x264 x265 fdk-aac)
FFMPEG_LDFLAGS += $(shell pkg-config --libs libavutil libavcodec libavdevice libavformat libswresample x264 x265 fdk-aac)

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKG_CONFIG ?= pkg-config
PKGCFG = $(if $(VDRDIR),$(shell $(PKG_CONFIG) --variable=$(1) $(VDRDIR)/vdr.pc),$(shell PKG_CONFIG_PATH="$$PKG_CONFIG_PATH:../../.." $(PKG_CONFIG) --variable=$(1) vdr))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
RESDIR = $(call PKGCFG,resdir)
CFGDIR = $(call PKGCFG,configdir)
#
TMPDIR ?= /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags) -std=c++17 -fno-strict-aliasing -O3 -DFPNG_NO_SSE=1  #-DSSE=1 -DFPNG_NO_SSE=0 -msse4.1 -mpclmul

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):

INCLUDES += -I$(U_WEBSOCKETS_INCLUDE) -I$(U_SOCKETS_INCLUDE) -I$(U_LIBSOCKET_INCLUDE) -I $(U_TINY_PROCESS_LIBRARY_INCLUDE)

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### The object files (add further files here):

OBJS = $(PLUGIN).o config.o server.o fpng.o webdevice.o webosd.o webremote.o webstatus.o ffmpeghls.o $(U_LIBSOCKET_OBJS_C) $(U_TINY_PROCESS_LIBRARY_OBJ)

### The main target:

all: uSockets $(SOFILE) i18n

### Implicit rules:

%.o: %.cpp
	@echo CC $@
	$(Q)$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

%.o: %.c
	@echo CC $@
	$(Q)$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	@echo MO $@
	$(Q)msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.cpp)
	@echo GT $@
	$(Q)xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	@echo PO $@
	$(Q)msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

$(SOFILE): $(OBJS)
	@echo LD $@
	$(Q)$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(U_SOCKETS_LIB) -o $@

uSockets:
	$(MAKE) -C $(U_SOCKETS_DIR)

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install-static:
	@mkdir -p $(DESTDIR)$(CFGDIR)/plugins/$(PLUGIN)/live
	@cp -pn static-html/* $(DESTDIR)$(CFGDIR)/plugins/$(PLUGIN)/live

install: all install-lib install-i18n install-static

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~
	@$(MAKE) -C $(U_SOCKETS_DIR) clean
