gs_libusb
=========

N64 GameShark Pro utilities using libusb.

This is based around the MosChip 7705 USB to Parallel Port bridge, which is
found in many low-cost adapters.

Subdirectories:
- comms: the main low-level interface
- gsuploader: homebrew loader based on ppcasm's "gsuploader"
- neon64: the "gsupload" NES emulator loader
