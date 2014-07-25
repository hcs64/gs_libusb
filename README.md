gs_libusb
=========

N64 GameShark Pro utilities using libusb.

This is based around the MosChip MCS7705 USB to Parallel Port bridge, which is
found in the [Tera Grand][teragrand] (aka [Viewcon][viewcon]) adapter,
available on [amazon][Amazon.com] and elsewhere.

Subdirectories:
- gsuploader: homebrew loader based on ppcasm's "gsuploader"
- gscomms: the low-level interface
- examples: a few example executables for use with gsuploader

[teragrand]: http://www.teragrand.com/product-p/usb-ve349.htm
[viewcon]: http://www.wiretek.com.cn/productshow.aspx?ProdId=921
[amazon]: http://amzn.com/B007MTQ2QS

# gsuploader

This is a version of ppcasm's gsuploader, with some new features and a lot of
reorganization.

gsuploader is a code loader for the N64 using the GameShark Pro 3.3's
parallel port and "Code Generator" functionality. This version adds:

- support for USB to parallel adaptors using the MosChip MCS7705 bridge
- a 2x speed transfer driver (supported by gscomms)
- a fast bulk transfer driver (supported by gscomms)

The support for linking has been stripped as it was incomplete.

## Example usage

Compile with

    make

in the root of the repository. I recommend running things from there as well.

On the N64 + GameShark, start a game with **Code Generator ON** and
**Without Codes**. I have largely tested with Super Mario 64, loading while at
the Select File screen.

### Neon64

The included files neon64gs.bin and efp.nes are a NES emulator (Neon64)
and an NES game ROM (Escape from Pong) that can be loaded together with
the command:

    ./gsuploader/gsuploader examples/neon64gs.bin examples/efp.nes

First the loader will load the emulator and then, after a brief pause while it
unpacks itself, it will load the NES ROM. Neon64 can be started with any button
after the ROM is loaded.

###Flame Demo

The Flame Demo needs to be loaded to 0x80000400, so change #define NEON64_MODE
to 0 before compiling. It can then be loaded with

    ./gsuploader/gsuploader examples/flames.bin

Hope it is useful for someone.

-hcs
