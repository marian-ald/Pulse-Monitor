all: pulse.hex

pulse.hex: pulse.elf
	avr-objcopy  -j .text -j .data -O ihex $^ $@
	avr-size pulse.elf

pulse.elf: pulse.c usart.c lcd.c
	avr-g++ -mmcu=atmega324p -DF_CPU=16000000 -Os -Wall -o $@ $^

clean:
	rm -rf pulse.elf pulse.hex
