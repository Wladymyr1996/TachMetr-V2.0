all:
	avr-gcc -std=gnu99 -mmcu=atmega8 -I. -gdwarf-2 -Os -o main.o main.c
	avr-objcopy -j .text -j .data -O ihex main.o main.hex
