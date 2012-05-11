#Do not modufy CFLAGS in this Makefile, we are using the environment variable!

HOST := $(shell $(CC) -dumpmachine)

BUILDDIR ?= build/$(HOST)
LIBDIR ?= lib/$(HOST)
LIB := $(LIBDIR)/libNDLCom.a

OBJS=\
	$(BUILDDIR)/ProtocolEncode.o \
	$(BUILDDIR)/ProtocolParser.o \

INCLUDES = -Isrc -Iinclude

PROTOCOLCFLAGS:=-fPIC   #for use in a shared library

.PHONY: doc test

all: $(LIB) test

distclean:
clean:
	rm -f $(OBJS)
	rm -f $(LIB)
	rm -rf doc/html

doc:
	doxygen doc/Doxyfile

$(BUILDDIR)/%.o: src/%.c
	@mkdir -p "$(BUILDDIR)"
	$(CC) $(DEFINES) $(INCLUDES) $(CFLAGS) $(PROTOCOLCFLAGS) -c -o $@ $<

TEST_CFLAGS:=-Wall -g3 -O0
$(BUILDDIR)/test-decoder: test/test-decoder.c $(LIB)
	$(CC) $(DEFINES) $(INCLUDES) $(TEST_CFLAGS) $(CFLAGS) $< -L$(LIBDIR) -lNDLCom -o $@

#only run test on native platform
TEST_DEPENDS=
NATIVEHOST=$(shell gcc -dumpmachine)
ifeq ($(HOST),$(NATIVEHOST))
	TEST_DEPENDS+=$(BUILDDIR)/test-result
endif

test: $(TEST_DEPENDS)

build/${HOST}/test-result: $(BUILDDIR)/test-decoder
	@echo -n Running test-decoder...
	@$(BUILDDIR)/test-decoder && touch "$@"

	@echo " done."

$(LIB): $(OBJS)
	@mkdir -p "$(LIBDIR)"
	$(AR) rs "$@" $(OBJS)
