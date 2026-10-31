CC = clang
LD = ld.lld
AS = nasm

KERNEL_BIN = iso/boot/kernel.bin
ISO_FILE = myos.iso
NVME_IMG = nvme.raw
NVME_SIZE = 64M

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

all: $(ISO_FILE) $(NVME_IMG)

$(NVME_IMG):
	qemu-img create -f raw $(NVME_IMG) $(NVME_SIZE)

run: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -m 256M -boot d \
	-drive file=$(NVME_IMG),if=none,id=nvm,format=raw \
	-device nvme,serial=Device001,drive=nvm

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

clean-all: clean
	rm -f $(NVME_IMG)

debug:
	qemu-system-x86_64 -cdrom $(ISO_FILE) -m 256M -boot d \
	-drive file=$(NVME_IMG),if=none,id=nvm,format=raw \
	-device nvme,serial=Device001,drive=nvm \
	-gdb tcp::1234 -S
