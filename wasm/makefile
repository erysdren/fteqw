CC=clang
override CFLAGS+=--target=wasm32 -Os -nostdlib -mbulk-memory
override LDFLAGS+=-Wl,--no-entry

OBJ=client.o main.o world.o

all: clean progs.wasm

clean:
	$(RM) progs.wasm $(OBJ)

progs.wasm: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
