CC = aarch64-linux-gnu-g++ 

#指定编译库的头文件与库文件的路径
CPPFLAGS += --sysroot=/opt/rk1808-sdk/buildroot/output/rockchip_rk1808/host/aarch64-buildroot-linux-gnu/sysroot
CPPFLAGS += -I /usr/local/arm/1808-sdk/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/drm/
CPPFLAGS += -I inc/
CPPFLAGS += -I ./rockx-rk1808-Linux/include 
CPPFLAGS += -I /home/gec/sql_arm/include   

LDFLAGS += -L ./rockx-rk1808-Linux/lib64  -lrockx  -lrknn_api
LDFLAGS += -L ./lib/
LDFLAGS += -lDRMwrap
LDFLAGS += -ldrm
#LDFLAGS += -Wl,-rpath=.

SRC = $(wildcard *.cc)

main:$(SRC)
	$(CC) $(SRC) -o  main $(CPPFLAGS) $(LDFLAGS)

clean:
	$(RM) main

distclean:clean
	$(MAKE) -C lib/ distclean

.PHONY:clean distclean
