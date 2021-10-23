project5.elf : project5.c avr.c avr.h lcd.c lcd.h
	avr-gcc -mmcu=atmega32 -Wl,-u,vfprintf -lprintf_flt -lm -o project5.elf project5.c avr.c lcd.c

flash : project5.elf
	avrdude -p m32 -P /dev/cu.usbmodem14201 -c avrisp -b 19200 -U flash:w:project5.elf

clean :
	rm project5.elf
