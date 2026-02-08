CC = clang
LD = ld.lld
AS = nasm

KERNEL_BIN = iso/boot/kernel.bin
ISO_FILE = myos.iso
SRCS_C = $(wildcard src/*.c)
SRCS_S = $(wildcard src/*.s)
OBJS = $(patsubst src/%.c, bins/%.o, $(SRCS_C)) \
       $(patsubst src/%.s, bins/%.o, $(SRCS_S))

CFLAGS = --target=x86_64-unknown-none-elf \
         -ffreestanding \
         -mno-red-zone \
				 -mno-mmx \
         -mno-sse \
         -mno-sse2 \
         -mcmodel=kernel \
         -Wall -g -O2


LDFLAGS = -T src/linker.ld -static -nostdlib

all: $(ISO_FILE)

run: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -m 256M -boot d

$(KERNEL_BIN): $(OBJS) src/linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(OBJS)

bins/%.o: src/%.c
	@mkdir -p bins
	$(CC) $(CFLAGS) -c $< -o $@

bins/%.o: src/%.s
	@mkdir -p bins
	nasm -f elf64 $< -o $@

$(ISO_FILE): $(KERNEL_BIN) iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_FILE) iso

clean:
	rm -rf bins $(KERNEL_BIN) $(ISO_FILE)

debug:
	qemu-system-x86_64 -cdrom $(ISO_FILE) -m 256M -boot d -gdb tcp::1234 -S
