DIRS = loader launcher test
BIN = bin

all:
	make -C loader
	make -C launcher
	make -C test
	mkdir -p bin
	mv launcher/launch bin/
	mv loader/simplerloader.so bin/



#Provide the command for cleanup
clean:
	for dir in $(DIRS); do \
	    $(MAKE) -C $$dir clean; \
	done
	-@rm -rf $(BIN)
