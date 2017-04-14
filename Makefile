
CC=gcc
SRC=src
BUILD=build
INCLUDE=include
LIB=lib
LIB_ABSPATH=$(shell cd $(LIB) && pwd)
CFLAGS_PROTO=-L$(LIB) -I$(INCLUDE) -pthread
LD_FLAGS=-lpthread -lprotobuf-c -lprotobuf-c-rpc -Wl,-rpath=$(LIB_ABSPATH)
CFLAGS=$(CFLAGS_PROTO) -Ibuild
LIBRARY_PATH+=$(LIB)

PROTOC_C=protoc-c
PROTOBUF_SRC=$(SRC)/protobuf
PROTOBUF_BUILD=$(BUILD)/protobuf

PROTOBUF_FILES=$(wildcard $(PROTOBUF_SRC)/*.proto)
PROTOBUF_COMPILED=$(patsubst $(PROTOBUF_SRC)/%.proto, $(PROTOBUF_BUILD)/%.pb-c.o, $(PROTOBUF_FILES))

all: build-dirs client router server 

debug: CFLAGS_PROTO+=-g
debug: clean all

$(PROTOBUF_BUILD)/%.pb-c.o: $(PROTOBUF_BUILD)/%.pb-c.c
	$(CC) $(CFLAGS_PROTO) -c -o $@ $< $(LD_FLAGS)

$(PROTOBUF_BUILD)/%.pb-c.c: $(PROTOBUF_SRC)/%.proto
	mkdir -p $(PROTOBUF_BUILD)
	(cd $(PROTOBUF_SRC) && $(PROTOC_C) $(shell basename $^) --c_out=../../$(PROTOBUF_BUILD))

$(PROTOBUF_BUILD)/%.pb-c.h: $(PROTOBUF_BUILD)/%.pb-c.c



# Compiles all protobuf files with protoc-c
protobuf-all: $(PROTOBUF_COMPILED)

$(BUILD)/%.o: $(SRC)/%.c protobuf-all
	$(CC) $(CFLAGS) -c -o $@ $< $(LD_FLAGS)

build-dirs:
	mkdir -p $(PROTOBUF_BUILD)
	mkdir -p $(BUILD)/router $(BUILD)/server $(BUILD)/client 

router: $(PROTOBUF_COMPILED) $(BUILD)/router/router.o
	$(CC) $(CFLAGS) -o $@ $^ $(LD_FLAGS)

client: $(PROTOBUF_COMPILED) $(BUILD)/client/client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LD_FLAGS)

server: $(PROTOBUF_COMPILED) $(BUILD)/server/server.o
	$(CC) $(CFLAGS) -o $@ $^ $(LD_FLAGS)

hashtable: 
	$(CC) -o $@ $(SRC)/hashtable/hashtable.c

clean:
	rm -rf $(BUILD)
	rm -f client router server

dist:
	(cd .. && tar czf hw4.tar.gz hw4)
