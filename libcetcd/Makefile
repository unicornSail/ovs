CFLAGS= -Wall -Wextra -fPIC -I third-party/build/include
LDFLAGS=-lcurl -lpthread
prefix=/usr

release:CFLAGS += -O3
release:all

debug:CFLAGS += -g -O0
debug:all

all: libcetcd.so libcetcd.a

clean:
	rm -rf libcetcd.so libcetcd.a *.o sds/*.o third-party/build
	rm -rf ../ovn/libcetcd.a 
	rm -rf ../ovsdb/libcetcd.a
	rm -rf ../lib/libcetcd.a
distclean:
	rm -rf libcetcd.so *.o sds/*.o third-party/*
install:all
	install -D libcetcd.so $(prefix)/lib/libcetcd.so
	install -D cetcd.h $(prefix)/include/cetcd.h
	install -D cetcd_array.h $(prefix)/include/cetcd_array.h
	install -D sds/sds.h $(prefix)/include/sds/sds.h

libcetcd.so: cetcd_array.o sds/sds.o cetcd.o 
	$(CC) $(LDFLAGS) -shared -o libcetcd.so cetcd_array.o sds/sds.o cetcd.o third-party/build/*.o
libcetcd.a: cetcd_array.o sds/sds.o cetcd.o 
	ar rcs $@ libcetcd.a cetcd_array.o sds/sds.o cetcd.o third-party/build/*.o
	cp libcetcd.a ../ovn/
	cp libcetcd.a ../ovsdb/
	cp libcetcd.a ../lib/
sds/sds.o:sds/sds.c sds/sds.h
cetcd_array.o:cetcd_array.c cetcd_array.h
cetcd.o:cetcd.c cetcd.h cetcd_json_parser.h third-party/build/include/yajl/*.h third-party/build/*.o

# Processing third-party projects
yajl-2.1.0:
	cd yajl-2.1.0/ && ./configure
	cd yajl-2.1.0/ && make

third-party/build/*.o:yajl-2.1.0
	mkdir -p third-party/build
	cp yajl-2.1.0/build/src/CMakeFiles/yajl.dir/*.o third-party/build/
third-party/build/include/yajl/*.h:yajl-2.1.0
	mkdir -p third-party/build/include/yajl
	cp yajl-2.1.0/src/api/*.h third-party/build/include/yajl
