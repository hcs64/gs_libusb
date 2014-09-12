CFLAGS=-std=gnu99 -Wall -pedantic -I gscomms/
LDFLAGS=-lusb-1.0
CC=gcc
COMMON_SRC=gscomms/gscomms.c
COMMON_DEPS=gscomms/gscomms.h $(COMMON_SRC)

all: gsuploader/gsuploader gsdemo2/gsdemo2

.PHONY: clean

gsuploader/gsuploader: gsuploader/gsuploader.c gsuploader/mips.h gsuploader/codegen.c gsuploader/codegen.h $(COMMON_DEPS)
	$(CC) $(CFLAGS) gsuploader/gsuploader.c gsuploader/codegen.c $(COMMON_SRC) -o gsuploader/gsuploader $(LDFLAGS)

gsdemo2/gsdemo2: gsdemo2/GSDEMO2.c gsdemo2/TRAINER.H gsdemo2/COMMS.H $(COMMON_DEPS)
	$(CC) $(CFLAGS) gsdemo2/GSDEMO2.c $(COMMON_SRC) -o gsdemo2/gsdemo2 $(LDFLAGS)

clean:
	rm -f gsuploader/gsuploader gsdemo2/gsdemo2
