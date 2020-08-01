default: ktimer

ktimer.o: ktimer.c
	arm-linux-gnueabihf-gcc -static -c ktimer.c -o ktimer.o
ktimer: ktimer.o
	arm-linux-gnueabihf-gcc -static ktimer.o -o ktimer
clean:
	rm -f ktimer.o ktimer