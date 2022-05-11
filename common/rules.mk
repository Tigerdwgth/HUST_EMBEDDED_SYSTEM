
CC:=/PYNQ/gcc-armv7hf/bin/arm-linux-gnueabihf-gcc
STRIP:=/PYNQ/gcc-armv7hf/bin/arm-linux-gnueabihf-strip

CFLAGS:=-Wall
LDFLAGS:=-Wall

INCLUDE := -I../common/external/include
LIB := -L../common/external/lib -lturbojpeg -lfreetype -lpng12 -lz -lm -lsds_lib

EXESRCS := ../common/graphic.c ../common/touch.c ../common/image.c ../common/task.c ../common/board_pynq_z1.c $(EXESRCS)

EXEOBJS := $(patsubst %.c, %.o, $(EXESRCS))

$(EXENAME): $(EXEOBJS)
	$(CC) $(LDFLAGS) -o $(EXENAME) $(EXEOBJS) $(LIB)
	mv $(EXENAME) ../out/

clean:
	rm -f $(EXENAME) $(EXEOBJS)

%.o: %.c ../common/common.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

