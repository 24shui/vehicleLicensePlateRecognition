CC = aarch64-linux-gnu-gcc

CPPFLAGS += --sysroot=/usr/local/arm/1808-sdk/host/aarch64-buildroot-linux-gnu/sysroot
CPPFLAGS += -I/usr/local/arm/1808-sdk/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/drm
CPPFLAGS += -I../inc

LDFLAGS  += -ldrm
LDFLAGS  += -Wl,-rpath=.

libDRMwrap.so:DRMwrap.o
	$(CC) $^ -o $@ -shared -fPIC 

DRMwrap.o:DRMwrap.c
	$(CC) $^ -o $@ -fPIC -c -I. $(CPPFLAGS) $(LDFLAGS)

clean:
	$(RM) DRMwrap.o

distclean:clean
	$(RM) libDRMwrap.so

.PHONY:clean distclean
