# allows automatical multicore build sessions!
MAKEFLAGS+=-j$(shell getconf _NPROCESSORS_ONLN)
export MAKEFLAGS
# by default, we use a compiler dependent install place. carefull -- i ask the c++ compiler, not the c-compiler!
# additionally the environment variable CXX is asked, so not neccessarily the native compiler!
ARCH=$(shell ${CXX} -dumpmachine)

all: compile

# so we have "build" as a shorthand for creating a new build environment
build: build/$(ARCH)/Makefile

build/$(ARCH)/Makefile:
	mkdir -p build/$(ARCH);\
	cd build/$(ARCH);\
	cmake ../../ -DCMAKE_INSTALL_PREFIX=~/DFKI.install/$(ARCH)

compile: build/$(ARCH)/Makefile
	${MAKE}  -C build/$(ARCH)

install: compile
	${MAKE}  -C build/$(ARCH) install

clean:
	rm -rf build/$(ARCH)

.PHONY: compile
