CFLAGS=-std=gnu99 -Wall -pedantic -I gscomms/
LDFLAGS=-lusb-1.0
CC=gcc
COMMON_SRC=gscomms/gscomms.c
COMMON_DEPS=gscomms/gscomms.h $(COMMON_SRC)

all: gsuploader/gsuploader 

.PHONY: clean

gsuploader/gsuploader: gsuploader/gsuploader.c gsuploader/mips.h gsuploader/codegen.c gsuploader/codegen.h $(COMMON_DEPS)
	$(CC) $(CFLAGS) gsuploader/gsuploader.c gsuploader/codegen.c $(COMMON_SRC) -o gsuploader/gsuploader $(LDFLAGS)

clean:
	rm -f gsuploader/gsuploader 
