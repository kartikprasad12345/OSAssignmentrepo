#Compile the launch.c by linking it with the lib_simpleloader.so
CC=gcc
CFLAGS=-m32 -shared -fPIC
LIB=../loader/lib_simpleloader.so
SRC=launch.c
OUT=launch
all:$(OUT)
	$(CC) -o $(OUT) $(SRC) $(LIB) $(CFLAGS)
#Provide the command for cleanup
