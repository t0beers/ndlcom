# NDLCom

Functions to encode/decode simple serial communication OSI-layer2 message
format. Named _ndlcom_ for _Node Data Link Communication_. Developed at DFKI in
the iStruct and SeeGrip projects.

To obtain something more profound see the
[documents](https://git.hb.dfki.de/istruct/documents/blob/master/README.md)
repository.

Comes with a cmake-based buildsystem and pkg-config files. Provides a simple
Makefile acting as a cmake-wrapper.

## structure

- src: Contains all source files (*.c)
- include/ndlcom: Contains all header files (*.h)
- build: The target directory for the build process, temporary content
- doc: some documentation, see [crc](doc/crc.md) for example.
