.PHONY: all lib test

all: lib test

include memory.d
include util.d
include block.d
include taffy-cuckoo.d
include taffy-block.d
include minimal-taffy-cuckoo.d
include static.d

lib: libfilter.a libfilter.so Makefile

test: lib Makefile
	$(MAKE) -C test

# TODO: without c11 compiled in, aligned_alloc is never used. But if
# it is compiled in, how can systems without a libc that cupposts
# aligned alloc work?
#
# Same with gnu and mmap, although probably header, object, and
# program all have the same answer to that one. Still need a way to test

WARN=-W -Wall -Wextra
RELEASE=-fPIC -O3 -ggdb3
CFLAGS=-std=gnu11 -march=native -mtune=native $(INCLUDES) $(WARN) $(RELEASE)
LINKS=-lm

include $(DEFAULT_RECIPE)

libfilter.so: util.o memory.o block.o taffy-cuckoo.o taffy-block.o minimal-taffy-cuckoo.o static.o Makefile
	$(CC) -fPIC -shared -o libfilter.so util.o memory.o block.o taffy-cuckoo.o taffy-block.o minimal-taffy-cuckoo.o static.o

libfilter.a: util.o memory.o block.o taffy-cuckoo.o taffy-block.o minimal-taffy-cuckoo.o static.o Makefile
	ar rcs libfilter.a util.o memory.o block.o taffy-cuckoo.o taffy-block.o minimal-taffy-cuckoo.o static.o

clean:
	rm -f libfilter.so libfilter.a
	rm -f memory.o memory.d memory.d.new
	rm -f util.o util.d util.d.new
	rm -f block.o block.d block.d.new
	rm -f taffy-cuckoo.o taffy-cuckoo.d taffy-cuckoo.d.new
	rm -f taffy-block.o taffy-block.d taffy-block.d.new
	rm -f minimal-taffy-cuckoo.o minimal-taffy-cuckoo.d minimal-taffy-cuckoo.d.new
	rm -f static.o static.d static.d.new

