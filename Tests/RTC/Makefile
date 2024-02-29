#==============================================================================
# Settings

NAME = rtc
OBJECTS = main.o lcd.o
PROGRAMMER = usbasp

#==============================================================================
# Targets

all: $(NAME).hex

$(NAME).hex: $(NAME).elf
	rm -f $@
	avr-objcopy -j .text -j .data -O ihex $(NAME).elf $(NAME).hex

$(NAME).elf: $(OBJECTS)
	avr-gcc -Os -mmcu=atmega644 -o $(NAME).elf $(OBJECTS)

-include $(OBJECTS:.o=.d)

%.o: %.c
	avr-gcc -DF_CPU=20000000 -Os -mmcu=atmega644 -c $< -o $@
	avr-gcc -DF_CPU=20000000 -Os -mmcu=atmega644 -MM $< > $*.d

flash: $(NAME).hex
	avrdude -c $(PROGRAMMER) -p m644 -U flash:w:$(NAME).hex:i

clean:
	rm -rf $(NAME).hex $(NAME).elf *.o *.d

