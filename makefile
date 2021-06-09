
export CFLAGS = -I. -Iinclude -Llib

debug: .always 
	make libmlisp.a libmlisp.so mlisp.exe CFLAGS="$(CFLAGS) -O0 -g3 -Wall"

release: .always 
	make libmlisp.a libmlisp.so mlisp.exe CFLAGS="$(CFLAGS) -O3 -Wall"

test: .always
	make debug 
	make test.exe CFLAGS="$(CFLAGS) -O0 -g3 -Wall"
	gdb test.exe 
	#./test.exe

setup: .always
	git clone https://github.com/tikubonn/bitarray
	git clone https://github.com/tikubonn/memnode
	git clone https://github.com/tikubonn/cgcmemnode
	git clone https://github.com/tikubonn/hashtable
	git clone https://github.com/tikubonn/hashset
	make release -C bitarray
	cp bitarray/bitarray.h include/bitarray.h
	cp bitarray/libbitarray.so lib/libbitarray.so
	cp bitarray/libbitarray.a lib/libbitarray.a
	make setup release -C memnode
	cp memnode/memnode.h include/memnode.h
	cp memnode/libmemnode.so lib/libmemnode.so
	cp memnode/libmemnode.a lib/libmemnode.a
	make setup release -C cgcmemnode
	cp cgcmemnode/cgcmemnode.h include/cgcmemnode.h
	cp cgcmemnode/libcgcmemnode.so lib/libcgcmemnode.so
	cp cgcmemnode/libcgcmemnode.a lib/libcgcmemnode.a
	make release -C hashtable
	cp hashtable/hashtable.h include/hashtable.h
	cp hashtable/libhashtable.so lib/libhashtable.so
	cp hashtable/libhashtable.a lib/libhashtable.a
	make release -C hashset
	cp hashset/hashset.h include/hashset.h
	cp hashset/libhashset.so lib/libhashset.so
	cp hashset/libhashset.a lib/libhashset.a

dist: .always 
	make mlisp.tar.gz

clean: .always
	rm -f mlisp.o

# libhashset.so: lib/libhashset.so
# 	cp lib/libhashset.so libhashset.so

# libhashtable.so: lib/libhashtable.so
# 	cp lib/libhashtable.so libhashtable.so

# libbitarray.so: lib/libbitarray.so
# 	cp lib/libbitarray.so libbitarray.so

# libmemnode.so: lib/libmemnode.so
# 	cp lib/libmemnode.so libmemnode.so

# libcgcmemnode.so: lib/libcgcmemnode.so
# 	cp lib/libcgcmemnode.so libcgcmemnode.so

# test.exe: test.c libmlisp.so mlisp.h libcgcmemnode.so libmemnode.so libbitarray.so libhashtable.so libhashset.so
# 	gcc $(CFLAGS) test.c libmlisp.so libcgcmemnode.so libmemnode.so libbitarray.so libhashtable.so libhashset.so -o test.exe 

test.exe: test.c libmlisp.so mlisp.h
	gcc $(CFLAGS) test.c libmlisp.so -o test.exe 

.always:

mlisp.o: mlisp.c mlisp.h 
	gcc $(CFLAGS) -c mlisp.c -o mlisp.o

# libmlisp.so: mlisp.o lib/libcgcmemnode.so lib/libbitarray.so lib/libmemnode.so lib/libhashtable.so lib/libhashset.so
# 	gcc $(CFLAGS) -shared mlisp.o lib/libcgcmemnode.so lib/libbitarray.so lib/libmemnode.so lib/libhashtable.so lib/libhashset.so -o libmlisp.so

libmlisp.so: mlisp.o lib/libcgcmemnode.a lib/libbitarray.a lib/libmemnode.a lib/libhashtable.a lib/libhashset.a
	gcc $(CFLAGS) -shared mlisp.o lib/libcgcmemnode.a lib/libbitarray.a lib/libmemnode.a lib/libhashtable.a lib/libhashset.a -o libmlisp.so

libmlisp.a: mlisp.o 
	ar r libmlisp.a mlisp.o

mlisp.exe: main.c libmlisp.so 
	gcc $(CFLAGS) main.c libmlisp.so -o mlisp.exe

mlisp.tar.gz: mlisp.exe libmlisp.so README.md LICENSE
	tar cvfz mlisp.tar.gz mlisp.exe libmlisp.so README.md LICENSE
