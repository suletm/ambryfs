all: fs
vrball: verbosefs

fs:
	gcc -DVERBOSE=0 ambryfs.c -o ambryfs -I/usr/include/fuse -D_FILE_OFFSET_BITS=64 -lfuse -pthread -lcurl
verbosefs:
	gcc -g -DVERBOSE=1 -DFUSE_VERBOSE=1 -g ambryfs.c -o ambryfs -I/usr/include/fuse -D_FILE_OFFSET_BITS=64 -lfuse -pthread -lcurl
