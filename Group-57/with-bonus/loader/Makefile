COMPILER = gcc
FLAGS = -m32 -shared -fPIC -c
SOURCES = loader.c
FILE = simplerloader.so

all: $(FILE)

$(FILE): $(SOURCES)
	$(COMPILER) $(FLAGS) -o $@ $^

clean:
	-@rm -f $(FILE)
