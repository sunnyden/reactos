#
# Global makefile
#

PATH_TO_TOP = .

include $(PATH_TO_TOP)/rules.mak

#
# Required to run the system
#
COMPONENTS = iface_native iface_additional hallib ntoskrnl
HALS = halx86
BUS = acpi isapnp
DLLS = ntdll kernel32 advapi32 crtdll msvcrt fmifs gdi32 msafd \
	ole32 oleaut32 secur32 shell32 user32 ws2_32 version
SUBSYS = smss win32k csrss

#
# Select the server(s) you want to build
#
#SERVERS = posix linux os2
SERVERS = win32

#
# Select the loader(s) you want to build
#
#LOADERS = boot dos
LOADERS = dos

#
# Select the device drivers and filesystems you want
#
#DEVICE_DRIVERS = beep floppy mouse sound test parallel serial
DEVICE_DRIVERS = vidport vga blue ide null floppy

#INPUT_DRIVERS = keyboard
INPUT_DRIVERS = keyboard mouclass psaux

#FS_DRIVERS = vfat minix ext2 template
FS_DRIVERS = vfat ms np

#NET_DRIVERS = ndis tdi tcpip tditest wshtcpip afd
NET_DRIVERS = ndis tdi tcpip tditest wshtcpip afd

#NET_DEVICE_DRIVERS = ne2000
NET_DEVICE_DRIVERS = ne2000

#
# storage drivers (don't change the order)
#
STORAGE_DRIVERS = class2 scsiport disk

#
# system applications (required for startup)
#
#SYS_APPS = lsass services shell winlogon
SYS_APPS = services shell winlogon

APPS = args hello test cat bench apc shm lpc thread event file gditest \
       pteb consume dump_shared_data vmtest regtest alive mstest nptest \
       objdir atomtest

#NET_APPS = ping roshttpd telnet
NET_APPS = ping roshttpd


KERNEL_SERVICES = $(DEVICE_DRIVERS) $(INPUT_DRIVERS) $(FS_DRIVERS) \
	$(NET_DRIVERS) $(NET_DEVICE_DRIVERS) $(STORAGE_DRIVERS)

all: tools buildno dk $(COMPONENTS) $(HALS) $(BUS) $(DLLS) $(SUBSYS) \
     $(LOADERS) $(KERNEL_SERVICES) $(SYS_APPS) $(APPS) $(NET_APPS)

clean: buildno_clean dk_clean $(HALS:%=%_clean) \
       $(COMPONENTS:%=%_clean) $(BUS:%=%_clean) $(DLLS:%=%_clean) \
       $(LOADERS:%=%_clean) $(KERNEL_SERVICES:%=%_clean) $(SUBSYS:%=%_clean) \
       $(SYS_APPS:%=%_clean) $(APPS:%=%_clean) $(NET_APPS:%=%_clean) clean_after tools_clean

clean_after:
	$(RM) $(PATH_TO_TOP)/include/roscfg.h

install: tools buildno install_dirs install_before \
	       $(COMPONENTS:%=%_install) $(HALS:%=%_install) $(BUS:%=%_install) \
         $(DLLS:%=%_install) $(LOADERS:%=%_install) \
         $(KERNEL_SERVICES:%=%_install) $(SUBSYS:%=%_install) \
         $(SYS_APPS:%=%_install) $(APPS:%=%_install)

dist: $(TOOLS_PATH)/rcopy$(EXE_POSTFIX) dist_clean dist_dirs \
      $(HALS:%=%_dist) $(COMPONENTS:%=%_dist) $(BUS:%=%_dist) $(DLLS:%=%_dist) \
      $(LOADERS:%=%_dist) $(KERNEL_SERVICES:%=%_dist) $(SUBSYS:%=%_dist) \
      $(SYS_APPS:%=%_dist) $(APPS:%=%_dist) $(NET_APPS:%=%_dist)

.PHONY: all clean clean_before install dist

#
# Build number generator
#
buildno: include/reactos/version.h
	make -C apps/buildno

buildno_clean:
	make -C apps/buildno clean

buildno_dist:

buildno_install:

.PHONY: buildno buildno_clean buildno_dist buildno_install

#
# System Applications
#
$(SYS_APPS): %:
	make -C apps/system/$*

$(SYS_APPS:%=%_clean): %_clean:
	make -C apps/system/$* clean

$(SYS_APPS:%=%_dist): %_dist:
	make -C apps/system/$* dist

$(SYS_APPS:%=%_install): %_install:
	make -C apps/system/$* install

.PHONY: $(SYS_APPS) $(SYS_APPS:%=%_clean) $(SYS_APPS:%=%_install) $(SYS_APPS:%=%_dist)

#
# Applications
#
$(APPS): %:
	make -C apps/$*

$(APPS:%=%_clean): %_clean:
	make -C apps/$* clean

$(APPS:%=%_dist): %_dist:
	make -C apps/$* dist

$(APPS:%=%_install): %_install:
	make -C apps/$* install

.PHONY: $(APPS) $(APPS:%=%_clean) $(APPS:%=%_install) $(APPS:%=%_dist)

#
# Network applications
#
$(NET_APPS): %:
	make -C apps/net/$*

$(NET_APPS:%=%_clean): %_clean:
	make -C apps/net/$* clean

$(NET_APPS:%=%_dist): %_dist:
	make -C apps/net/$* dist

$(NET_APPS:%=%_install): %_install:
	make -C apps/net/$* install

.PHONY: $(NET_APPS) $(NET_APPS:%=%_clean) $(NET_APPS:%=%_install) $(NET_APPS:%=%_dist)


#
# Tools
#
tools:
	make -C tools

tools_clean:
	make -C tools clean

tools_install:

tools_dist:

.PHONY: tools tools_clean tools_install tools_dist


#
# Developer Kits
#
dk:
	$(RMKDIR) $(DK_PATH)
	$(RMKDIR) $(DDK_PATH)
	$(RMKDIR) $(DDK_PATH_LIB)
	$(RMKDIR) $(DDK_PATH_INC)
	$(RMKDIR) $(SDK_PATH)
	$(RMKDIR) $(SDK_PATH_LIB)
	$(RMKDIR) $(SDK_PATH_INC)
	$(RMKDIR) $(XDK_PATH)
	$(RMKDIR) $(XDK_PATH_LIB)
	$(RMKDIR) $(XDK_PATH_INC)

# WARNING! Be very sure that there are no important files
#          in these directories before cleaning them!!!
dk_clean:
	$(RM) $(DDK_PATH_LIB)/*.a
# $(RM) $(DDK_PATH_INC)/*.h
	$(RMDIR) $(DDK_PATH_LIB)
#	$(RMDIR) $(DDK_PATH_INC)
	$(RM) $(SDK_PATH_LIB)/*.a
# $(RM) $(SDK_PATH_INC)/*.h
	$(RMDIR) $(SDK_PATH_LIB)
#	$(RMDIR) $(SDK_PATH_INC)
	$(RM) $(XDK_PATH_LIB)/*.a
#	$(RM) $(XDK_PATH_INC)/*.h
	$(RMDIR) $(XDK_PATH_LIB)
#	$(RMDIR) $(XDK_PATH_INC)

dk_install:

dk_dist:

.PHONY: dk dk_clean dk_install dk_dist


#
# Interfaces
#
iface_native:
	make -C iface/native

iface_native_clean:
	make -C iface/native clean

iface_native_install:

iface_native_dist:

iface_additional:
	make -C iface/addsys

iface_additional_clean:
	make -C iface/addsys clean

iface_additional_install:

iface_additional_dist:

.PHONY: iface_native iface_native_clean iface_native_install \
        iface_native_dist \
        iface_additional iface_additional_clean iface_additional_install \
        iface_additional_dist

#
# Bus driver rules
#
$(BUS): %:
	make -C services/bus/$*

$(BUS:%=%_clean): %_clean:
	make -C services/bus/$* clean

$(BUS:%=%_install): %_install:
	make -C services/bus/$* install

$(BUS:%=%_dist): %_dist:
	make -C services/bus/$* dist

.PHONY: $(BUS) $(BUS:%=%_clean) \
        $(BUS:%=%_install) $(BUS:%=%_dist)

#
# Device driver rules
#
$(DEVICE_DRIVERS): %:
	make -C services/dd/$*

$(DEVICE_DRIVERS:%=%_clean): %_clean:
	make -C services/dd/$* clean

$(DEVICE_DRIVERS:%=%_install): %_install:
	make -C services/dd/$* install

$(DEVICE_DRIVERS:%=%_dist): %_dist:
	make -C services/dd/$* dist

.PHONY: $(DEVICE_DRIVERS) $(DEVICE_DRIVERS:%=%_clean) \
        $(DEVICE_DRIVERS:%=%_install) $(DEVICE_DRIVERS:%=%_dist)

#
# Input driver rules
#
$(INPUT_DRIVERS): %:
	make -C services/input/$*

$(INPUT_DRIVERS:%=%_clean): %_clean:
	make -C services/input/$* clean

$(INPUT_DRIVERS:%=%_install): %_install:
	make -C services/input/$* install

$(INPUT_DRIVERS:%=%_dist): %_dist:
	make -C services/input/$* dist

.PHONY: $(INPUT_DRIVERS) $(INPUT_DRIVERS:%=%_clean) \
        $(INPUT_DRIVERS:%=%_install) $(INPUT_DRIVERS:%=%_dist)

$(FS_DRIVERS): %:
	make -C services/fs/$*

$(FS_DRIVERS:%=%_clean): %_clean:
	make -C services/fs/$* clean

$(FS_DRIVERS:%=%_install): %_install:
	make -C services/fs/$* install

$(FS_DRIVERS:%=%_dist): %_dist:
	make -C services/fs/$* dist

.PHONY: $(FS_DRIVERS) $(FS_DRIVERS:%=%_clean) \
        $(FS_DRIVERS:%=%_install) $(FS_DRIVERS:%=%_dist)

#
# Network driver rules
#
$(NET_DRIVERS): %:
	make -C services/net/$*

$(NET_DRIVERS:%=%_clean): %_clean:
	make -C services/net/$* clean

$(NET_DRIVERS:%=%_install): %_install:
	make -C services/net/$* install

$(NET_DRIVERS:%=%_dist): %_dist:
	make -C services/net/$* dist

.PHONY: $(NET_DRIVERS) $(NET_DRIVERS:%=%_clean) \
        $(NET_DRIVERS:%=%_install) $(NET_DRIVERS:%=%_dist)

$(NET_DEVICE_DRIVERS): %:
	make -C services/net/dd/$*

$(NET_DEVICE_DRIVERS:%=%_clean): %_clean:
	make -C services/net/dd/$* clean

$(NET_DEVICE_DRIVERS:%=%_install): %_install:
	make -C services/net/dd/$* install

$(NET_DEVICE_DRIVERS:%=%_dist): %_dist:
	make -C services/net/dd/$* dist

.PHONY: $(NET_DEVICE_DRIVERS) $(NET_DEVICE_DRIVERS:%=%_clean) \
        $(NET_DEVICE_DRIVERS:%=%_install) $(NET_DEVICE_DRIVERS:%=%_dist)

#
# storage driver rules
#
$(STORAGE_DRIVERS): %:
	make -C services/storage/$*

$(STORAGE_DRIVERS:%=%_clean): %_clean:
	make -C services/storage/$* clean

$(STORAGE_DRIVERS:%=%_install): %_install:
	make -C services/storage/$* install

$(STORAGE_DRIVERS:%=%_dist): %_dist:
	make -C services/storage/$* dist

.PHONY: $(STORAGE_DRIVERS) $(STORAGE_DRIVERS:%=%_clean) \
        $(STORAGE_DRIVERS:%=%_install) $(STORAGE_DRIVERS:%=%_dist)

#
# Kernel loaders
#

$(LOADERS): %:
	make -C loaders/$*

$(LOADERS:%=%_clean): %_clean:
	make -C loaders/$* clean

$(LOADERS:%=%_install): %_install:
	make -C loaders/$* install

$(LOADERS:%=%_dist): %_dist:
	make -C loaders/$* dist

.PHONY: $(LOADERS) $(LOADERS:%=%_clean) $(LOADERS:%=%_install) \
        $(LOADERS:%=%_dist)

#
# Required system components
#

ntoskrnl:
	make -C ntoskrnl

ntoskrnl_clean:
	make -C ntoskrnl clean

ntoskrnl_install:
	make -C ntoskrnl install

ntoskrnl_dist:
	make -C ntoskrnl dist

.PHONY: ntoskrnl ntoskrnl_clean ntoskrnl_install ntoskrnl_dist

#
# Hardware Abstraction Layer import library
#

hallib:
	make -C hal/hal

hallib_clean:
	make -C hal/hal clean

hallib_install:
	make -C hal/hal install

hallib_dist:
	make -C hal/hal dist

.PHONY: hallib hallib_clean hallib_install hallib_dist

#
# Hardware Abstraction Layers
#

$(HALS): %:
	make -C hal/$*

$(HALS:%=%_clean): %_clean:
	make -C hal/$* clean

$(HALS:%=%_install): %_install:
	make -C hal/$* install

$(HALS:%=%_dist): %_dist:
	make -C hal/$* dist

.PHONY: $(HALS) $(HALS:%=%_clean) $(HALS:%=%_install) $(HALS:%=%_dist)

#
# Required DLLs
#

$(DLLS): %:
	make -C lib/$*

$(DLLS:%=%_clean): %_clean:
	make -C lib/$* clean

$(DLLS:%=%_install): %_install:
	make -C lib/$* install

$(DLLS:%=%_dist): %_dist:
	make -C lib/$* dist

.PHONY: $(DLLS) $(DLLS:%=%_clean) $(DLLS:%=%_install) $(DLLS:%=%_dist)

#
# Kernel Subsystems
#

$(SUBSYS): %:
	make -C subsys/$*

$(SUBSYS:%=%_clean): %_clean:
	make -C subsys/$* clean

$(SUBSYS:%=%_install): %_install:
	make -C subsys/$* install

$(SUBSYS:%=%_dist): %_dist:
	make -C subsys/$* dist

.PHONY: $(SUBSYS) $(SUBSYS:%=%_clean) $(SUBSYS:%=%_install) \
        $(SUBSYS:%=%_dist)

#
# Create an installation
#

install_clean:
	$(RM) $(INSTALL_DIR)/system32/drivers/*.*
	$(RM) $(INSTALL_DIR)/system32/config/*.*
	$(RM) $(INSTALL_DIR)/system32/*.*
	$(RM) $(INSTALL_DIR)/symbols/*.*
	$(RM) $(INSTALL_DIR)/media/fonts/*.*
	$(RM) $(INSTALL_DIR)/media/*.*
	$(RM) $(INSTALL_DIR)/bin/*.*
	$(RM) $(INSTALL_DIR)/*.com
	$(RM) $(INSTALL_DIR)/*.bat
	$(RMDIR) $(INSTALL_DIR)/system32/drivers
	$(RMDIR) $(INSTALL_DIR)/system32/config
	$(RMDIR) $(INSTALL_DIR)/system32
	$(RMDIR) $(INSTALL_DIR)/symbols
	$(RMDIR) $(INSTALL_DIR)/media/fonts
	$(RMDIR) $(INSTALL_DIR)/media
	$(RMDIR) $(INSTALL_DIR)/bin
	$(RMDIR) $(INSTALL_DIR)

install_dirs:
	$(RMKDIR) $(INSTALL_DIR)
	$(RMKDIR) $(INSTALL_DIR)/bin
	$(RMKDIR) $(INSTALL_DIR)/media
	$(RMKDIR) $(INSTALL_DIR)/media/fonts
	$(RMKDIR) $(INSTALL_DIR)/symbols
	$(RMKDIR) $(INSTALL_DIR)/system32
	$(RMKDIR) $(INSTALL_DIR)/system32/config
	$(RMKDIR) $(INSTALL_DIR)/system32/drivers

install_before:
	$(CP) boot.bat $(INSTALL_DIR)/boot.bat
	$(CP) media/fonts/helb____.ttf $(INSTALL_DIR)/media/fonts/helb____.ttf
	$(CP) media/fonts/timr____.ttf $(INSTALL_DIR)/media/fonts/timr____.ttf

.PHONY: install_clean install_dirs install_before


#
# Make a distribution saveset
#

dist_clean:
	$(RM) $(DIST_DIR)/symbols/*.sym
	$(RM) $(DIST_DIR)/drivers/*.sys
	$(RM) $(DIST_DIR)/subsys/*.exe
	$(RM) $(DIST_DIR)/dlls/*.dll
	$(RM) $(DIST_DIR)/apps/*.exe
	$(RM) $(DIST_DIR)/*.exe
	$(RMDIR) $(DIST_DIR)/symbols
	$(RMDIR) $(DIST_DIR)/subsys
	$(RMDIR) $(DIST_DIR)/drivers
	$(RMDIR) $(DIST_DIR)/dlls
	$(RMDIR) $(DIST_DIR)/apps
	$(RMDIR) $(DIST_DIR)

dist_dirs:
	$(RMKDIR) $(DIST_DIR)
	$(RMKDIR) $(DIST_DIR)/apps
	$(RMKDIR) $(DIST_DIR)/dlls
	$(RMKDIR) $(DIST_DIR)/drivers
	$(RMKDIR) $(DIST_DIR)/subsys
	$(RMKDIR) $(DIST_DIR)/symbols

.PHONY: dist_clean dist_dirs


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
