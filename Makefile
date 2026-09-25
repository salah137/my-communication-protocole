CC = arm-none-eabi-gcc
CPU = cortex-m3
FLAGS = -mthumb -mcpu=$(CPU) -c -g3 -O0 -std=gnu11
LD = arm-none-eabi-ld
GDB = arm-none-eabi-gdb

LDFLAGS = -T map.ld
LDFLAGS2 = -nostdlib -T map.ld -Map=out.map

BOARD ?= stm32vldiscovery
PORT_QEMU ?= 1234
PORT_RENODE ?= 3333

PROJECT = mesh
OBJ_DIR = o_files

# 1. Find all .c files recursively, excluding the o_files directory, and strip leading ./
C_FILES := $(shell find . -name "*.c" -not -path "./$(OBJ_DIR)/*" | sed 's|^\./||')

# 2. Map source files to corresponding object files inside o_files/
OBJS := $(C_FILES:%.c=$(OBJ_DIR)/%.o)

.PHONY: all qemu gdb clean

all: $(PROJECT).elf

# 3. Pattern rule to compile any .c file into its mirrored .o path, creating subdirs automatically
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) -c $< -o $@

# 4. Link all automatically gathered object files into the final ELF
$(PROJECT).elf: $(OBJS)
	$(LD) $(LDFLAGS2) -o $@ $^

qemu: $(PROJECT).elf
	arm-none-eabi-objdump -D -S $(PROJECT).elf > $(PROJECT).elf.lst
	arm-none-eabi-readelf -a $(PROJECT).elf > $(PROJECT).elf.debug
	qemu-system-arm -S -M $(BOARD) -cpu $(CPU) -nographic -kernel $(PROJECT).elf -gdb tcp::$(PORT)

renode: $(PROJECT).elf
	renode mesh.resc
gdb_renode: $(PROJECT).elf
	$(GDB) -q $(PROJECT).elf -ex "target remote localhost:$(PORT_RENODE)"

gdb_qemu: $(PROJECT).elf 
	$(GDB) -q $(PROJECT).elf -ex "target remote localhost:$(PORT_QEMU)"
	
clean:
	rm -rf $(OBJ_DIR) $(PROJECT).elf *.map *.lst *.debug