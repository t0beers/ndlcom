# Cross Compiling

The provided wrapper-Makefile understands the `ARCH` variable which shall point to a existing [toolchain files][1]. Thus, cross compiling can be as simple as:

```bash
make ARCH=arm-poky-linux-gnueabi
```

[1]: https://git.hb.dfki.de/istruct/cmakemodules
