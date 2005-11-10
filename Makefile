loaps: loaps.c
	gcc -DDEBUG loaps.c -g -o loaps

loaps-normal: clean
	gcc loaps.c -o loaps

test: loaps
	./loaps ./knownError

debug: loaps
	gdb --args ./loaps ./knownError

clean:
	rm loaps