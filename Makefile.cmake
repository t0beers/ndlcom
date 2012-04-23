# 423d7ddbc -- unique string to find this makefile without sourcecontrol...
# allows automatical multicore build sessions!
JOBS=$(shell getconf _NPROCESSORS_ONLN)

# we wanna use absolute path' where possible
SRCDIR=$(shell pwd)

# by default, we use a compiler dependent build and install directory.
# carefull -- we ask the c++ compiler, not the c-compiler!
# additionally the environment variable CXX is asked, so not neccessarily the native compiler!
INSTALLDIR=$(shell readlink -m ~/DFKI.install/$(shell ${CXX} -dumpmachine))
BUILDDIR=$(shell readlink -m ./build/$(shell ${CXX} -dumpmachine))

# SILENCE!
MAKEFLAGS=--no-print-directory

# default:
all: compile

# so we have "build" as a shorthand for creating a new build environment
build: $(BUILDDIR)/Makefile

info:
	@echo "srcdir (here):"
	@echo $(SRCDIR)
	@echo "installdir:"
	@echo $(INSTALLDIR)
	@echo "builddir:"
	@echo $(BUILDDIR)

$(BUILDDIR)/Makefile:
	mkdir -p $(BUILDDIR);\
	sh -c "cd $(BUILDDIR); cmake $(SRCDIR) -DCMAKE_INSTALL_PREFIX=$(INSTALLDIR)"

dependencys: build
	mkdir -p $(BUILDDIR)
	sh -c "cd $(BUILDDIR); cmake $(SRCDIR) --graphviz=dependencys.dot"
	dot $(BUILDDIR)/dependencys.dot -Tpng > $(SRCDIR)/dependencys.png

compile: build
	${MAKE} -j$(JOBS) -C $(BUILDDIR)

test: compile
	${MAKE} -j$(JOBS) -C $(BUILDDIR) test

install: compile
	${MAKE} -j$(JOBS) -C $(BUILDDIR) install

clean:
	${MAKE} -C $(BUILDDIR) clean

distclean: clean
	rm -rf $(BUILDDIR)
	@rm -f dependencys.png

.PHONY: compile
