PROJECT_NAME=NDLCom

all: $(PROJECT_NAME)_sharedgui

build:
	mkdir -p build;\
	cd build;\
	cmake .. -DCMAKE_INSTALL_PREFIX=~/iStruct/iStruct.install

build/$(PROJECT_NAME)_sharedgui: build
	${MAKE} -C build

build/lib$(PROJECT_NAME).so: build
	${MAKE} -C build

$(PROJECT_NAME)_sharedgui: build/$(PROJECT_NAME)_sharedgui
	cp $< $@

install: build/lib$(PROJECT_NAME).so
	${MAKE} -C build install

clean:
	rm -rf build
	rm -f $(PROJECT_NAME)_sharedgui

.PHONY: build/$(PROJECT_NAME)_sharedgui
