# Cross Compiling

The provided wrapper-Makefile understands the `ARCH` variable which shall point to a existing CMAKE toolchain file.
For more information on how to setup such a file see [this info](http://www.cmake.org/Wiki/CMake_Cross_Compiling#The_toolchain_file).
Thus, cross compiling can be as simple as:

```bash
make ARCH=arm-poky-linux-gnueabi
```
