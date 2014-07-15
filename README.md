gs_libusb
=========

N64 GameShark Pro utilities using libusb.

This is based around the MosChip MCS7705 USB to Parallel Port bridge, which is
found in the [Tera Grand adapter][teragrand].

Subdirectories:
- comms: the main low-level interface
- gsuploader: homebrew loader based on ppcasm's "gsuploader"
- examples: a few example executables for use with gsuploader

[teragrand]: http://www.teragrand.com/product-p/usb-ve349.htm
