CFLAGS=-std=gnu99 -Wall -pedantic -I gscomms/
LDFLAGS=-lusb-1.0
CC=gcc
COMMON_SRC=gscomms/gscomms.c
COMMON_DEPS=gscomms/gscomms.h $(COMMON_SRC)

all: gsuploader/gsuploader neon64/gsupload

.PHONY: clean

gsuploader/gsuploader: gsuploader/gsuploader.c gsuploader/mips.h $(COMMON_DEPS)
	$(CC) $(CFLAGS) gsuploader/gsuploader.c $(COMMON_SRC) -o gsuploader/gsuploader $(LDFLAGS)

neon64/gsupload: neon64/gsupload.c $(COMMON_DEPS)
	$(CC) $(CFLAGS) neon64/gsupload.c $(COMMON_SRC) -o neon64/gsupload $(LDFLAGS)

clean:
	rm -f gsuploader/gsuploader neon64/gsupload
