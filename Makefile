CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-ld
GDB     = arm-none-eabi-gdb
OBJDUMP = arm-none-eabi-objdump
READELF = arm-none-eabi-readelf

CPU     = cortex-m3
CFLAGS  = -mthumb -mcpu=$(CPU) -g3 -O0 -std=gnu11 -MMD -MP
LDFLAGS = -nostdlib -T map.ld -Map=out.map

PORT_OPENOCD ?= 3333
PORT_QEMU ?= 1234

PROJECT  = mesh
OBJ_DIR  = o_files

C_FILES := $(shell find . -name "*.c" -not -path "./$(OBJ_DIR)/*" -not -path "./testing/*" | sed 's|^\./||')

OBJS    := $(C_FILES:%.c=$(OBJ_DIR)/%.o)

BOARD ?= stm32vldiscovery


.PHONY: all flash openocd_server gdb lst clean

all: $(PROJECT).elf

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(PROJECT).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

lst: $(PROJECT).elf
	$(OBJDUMP) -D -S $< > $(PROJECT).elf.lst
	$(READELF) -a $< > $(PROJECT).elf.debug

qemu : 
	qemu-system-arm -S -M $(BOARD) -cpu $(CPU) -nographic -kernel $(PROJECT).elf -gdb tcp::1234
	
	
flash: $(PROJECT).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
	        -c "program $< verify reset exit"

openocd_server:
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg

gdb_openocd: $(PROJECT).elf
	$(GDB) -q $< -ex "target remote localhost:$(PORT_OPENOCD)"

gdb_qemu: $(PROJECT).elf
	$(GDB) -q $< -ex "target remote localhost:$(PORT_QEMU)"
	
clean:
	rm -rf $(OBJ_DIR) $(PROJECT).elf *.map *.lst *.debug

-include $(OBJS:.o=.d)