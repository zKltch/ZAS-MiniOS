# compile and tools
CC = x86_64-elf-gcc
AS = nasm  
LD = x86_64-elf-ld

# compile parameters
# -ffreestanding: raw machine
# -fno-pie: disable randomize address
CFLAGS = -Wall -Wextra -std=c11 -ffreestanding -fno-pie -fno-stack-protector -I. -g
ASFLAGS = -f elf64

# project file
SRCS_C = $(wildcard src/*.c)
SRCS_S = $(wildcard src/*.s)
OBJS   = $(patsubst src/%.c, bins/%.o, $(SRCS_C)) \
          $(patsubst src/%.s, bins/%.o, $(SRCS_S))
KERNEL = iso/boot/kernel.bin
ISO = ZAS-MiniOS.iso

.PHONY: all run clean

# rules
all: $(ISO)

run: $(ISO)
	qemu-system-x86_64 \
    -cdrom ZAS-MiniOS.iso \
    -m 256M \
    -boot d \

debug: $(iso)
	qemu-system-x86_64 \
    -cdrom ZAS-MiniOS.iso \
    -m 256M \
    -boot d \
    -s \
    -S

$(ISO): $(KERNEL) iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso

$(KERNEL): $(OBJS) src/linker.ld
	@echo "linking"
	@echo $(OBJS)
	$(CC) -T src/linker.ld -o $(KERNEL) $(OBJS) -ffreestanding -O2 -nostdlib -lgcc

# compile rule
bins/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

bins/%.o: src/%.s
	$(AS) $(ASFLAGS) $< -o $@

# clean rule
clean:
	rm -rf $(OBJS) $(KERNEL) $(ISO)
