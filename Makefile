HIREDIS_LIBRARY=/usr/local/lib/libhiredis.a
LIBFFI_LIBRARY=/usr/local/lib/libffi.a
MSGPACK_LIBRARY=/usr/local/lib/libmsgpackc.a

BUILD_FOLDER=./build

CC=gcc
CFLAGS=-O0 -g -pthread -I. -I$(BUILD_FOLDER) -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -fPIC

OBJ=$(BUILD_FOLDER)/client.o $(BUILD_FOLDER)/server.o $(BUILD_FOLDER)/serializer.o $(BUILD_FOLDER)/dispatcher.o $(BUILD_FOLDER)/worker.o
LIB=$(HIREDIS_LIBRARY) $(LIBFFI_LIBRARY) $(MSGPACK_LIBRARY)

$(BUILD_FOLDER)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

build: awesome_farm.c $(OBJ) $(LIB)
	$(CC) $(CFLAGS) -shared awesome_farm.c $(OBJ) $(LIB) -o $(BUILD_FOLDER)/awesome-farm.so

test_client: build
	$(CC) $(CFLAGS) test/test_client.c test/test_workers.c $(BUILD_FOLDER)/awesome-farm.so -o $(BUILD_FOLDER)/test_client
	$(BUILD_FOLDER)/test_client

test_server: build
	$(CC) $(CFLAGS) test/test_server.c test/test_workers.c $(BUILD_FOLDER)/awesome-farm.so -o $(BUILD_FOLDER)/test_server
	$(BUILD_FOLDER)/test_server

.PHONY: clean

clean:
	rm -rf ./build/*
