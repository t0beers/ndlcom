JOBS=$(shell getconf _NPROCESSORS_ONLN)

all: compile

build:
	mkdir -p build;\
	cd build;\
	cmake .. -DCMAKE_INSTALL_PREFIX=~/DFKI.install

compile: build
	${MAKE} -C build -j$(JOBS)

install: build
	${MAKE} -C build -j$(JOBS) install

clean:
	rm -rf build

.PHONY: compile
