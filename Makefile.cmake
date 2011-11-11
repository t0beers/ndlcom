JOBS=$(shell getconf _NPROCESSORS_ONLN)

all: compile

build:
	mkdir -p build;\
	cd build;\
	cmake .. -DCMAKE_INSTALL_PREFIX=~/SeeGrip/SeeGrip.install

compile: build
	${MAKE} -C build -j$(JOBS)

install: build subdir
	${MAKE} -C build -j$(JOBS) install

clean:
	rm -rf build

.PHONY: subdir
