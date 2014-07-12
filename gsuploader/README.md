# gsuploader

This is a version of ppcasm's gsuploader, with some new features and a lot of
reorganization.

gsuploader is a code loader for the N64 using the GameShark Pro 3.3's
parallel port and "Code Generator" functionality. This version adds:

- support for USB to parallel adaptors using the MosChip MCS7705 bridge
- a 2x faster transfer mode

The support for linking has largely been stripped as it was incomplete.

## Example usage

Start a game with **Code Generator ON** and **Without Codes**. I have largely
tested with Super Mario 64, loading while at the Select File screen.

### Neon64

The included files neon64gs.bin and efp.nes are a NES emulator (Neon64)
and an NES game ROM (Escape from Pong) that can be loaded together with
the command:

    ./gsuploader ../examples/neon64gs.bin ../examples/efp.nes

First the loader will load the emulator and then, after a brief pause while it
unpacks itself, it will load the ROM. Neon64 can be started with any button
after the ROM is loaded.

###Flame Demo

The Flame Demo need to be loaded to 0x80000400, so change the #defines
UPLOAD_ADDR and ENTRYPOINT to 0x80000400UL. It can then be loaded with

    ./gsuploader ../examples/flames.bin




-hcs
