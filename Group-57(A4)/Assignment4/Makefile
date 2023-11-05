all:
	gcc  -m32 -no-pie -nostdlib -o sum sum.c
	gcc -m32 -o simplesmartloader simplesmartloaderloader.c
	./simplesmartloader sum
clean:
	-@rm -f sum simplesmartloader
