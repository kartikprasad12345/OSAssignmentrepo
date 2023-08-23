CC = gcc
CFLAGS = -m32 -shared -fPIC
SOURCES = loader.c
OUTPUT = simplerloader.so

all: $(OUTPUT)

$(OUTPUT): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-@rm -f $(OUTPUT)

