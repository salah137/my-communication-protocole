CC = arm-none-eabi-gcc
CPU = cortex-m3
FLAGS = -mthumb -mcpu=$(CPU) -c -g3 -O0 -std=gnu11
LD = arm-none-eabi-ld
GDB = arm-none-eabi-gdb

LDFLAGS = -T map.ld
LDFLAGS2 = -nostdlib -T map.ld -Map=out.map

BOARD ?= stm32vldiscovery
PORT ?= 1234

PROJECT = mesh

.PHONY = qemu gdb

o_files/stm32_startup.o : stm32_startup.c 
	$(CC) $(FLAGS) -o $@ $^  

o_files/main.o : main.c 
	$(CC) $(FLAGS) -o $@ $^  

o_files/gpio_driver.o : gpio_driver.c  
	$(CC) $(FLAGS) -o $@ $^  

$(PROJECT).elf : o_files/stm32_startup.o o_files/main.o o_files/gpio_driver
	$(LD) $(LDFLAGS2) -o $@ $^  


qemu: $(PROJECT).elf
	arm-none-eabi-objdump -D -S $(PROJECT).elf > $(PROJECT).elf.lst
	arm-none-eabi-readelf -a $(PROJECT).elf > $(PROJECT).elf.debug
	qemu-system-arm -S -M $(BOARD) -cpu $(CPU) -nographic -kernel $(PROJECT).elf -gdb tcp::$(PORT)
	

gdb: $(PROJECT).elf
	$(GDB) -q $(PROJECT).elf -ex "target remote localhost:$(PORT)"

clean:
	rm -f *.o *.elf *.map *.lst *.debug o_files/*.o