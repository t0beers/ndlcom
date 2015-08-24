![ndlcom_logo](doc/ndlcom_logo.png)

Functions to encode/decode simple serial communication OSI-layer2 message
format. Named _ndlcom_ for _Node Data Link Communication_. Developed at DFKI in
the iStruct and SeeGrip projects. See the [here](doc/NDLCom_short_en.md) for a
short explanation and [here](doc/NDLCom_long_en.md) for more details.

To obtain something more profound see the
[documents](https://git.hb.dfki.de/istruct/documents/blob/master/README.md)
repository.

Comes with a cmake-based buildsystem and pkg-config files. Provides a simple
Makefile acting as a cmake-wrapper.

## Structure

- `src` Contains all source files of the C-language
- `test` some crudge limited programs used for testing and benchmarking, called using `make test`
- `include/ndlcom` Contains all header files used by the library
- `build/...` The target directory used during the build process, temporary content
- `doc` some documentation
- `tools` contain usefull compiled tools
- `scripts`: probably the same category than tools...
