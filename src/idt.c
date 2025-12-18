#include "idt.h"

struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct   idt_ptr;

extern void isr_stub();

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].offset_low  = base & 0xFFFF;
    idt_entries[num].selector    = sel;
    idt_entries[num].ist         = 0;
    idt_entries[num].type_attr   = flags;
    idt_entries[num].offset_mid  = (base >> 16) & 0xFFFF;
    idt_entries[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt_entries[num].zero        = 0;
}

void init_idt(void) {
    __asm__ volatile (
        "xor %%rax, %%rax\n\t"
        "mov $0, %%rcx\n\t"
        "lea %0, %%rdi\n\t"
        "mov $4096, %%rdx\n\t"
        "rep stosb\n\t"
        : : "m"(idt_entries) : "rax", "rcx", "rdi", "rdx", "memory"
    );

    idt_ptr.limit = (uint16_t)(sizeof(idt_entries) - 1);
    idt_ptr.base  = (uint64_t)&idt_entries;

    idt_set_gate(0, (uint64_t)isr_stub, 0x08, 0x8E);

    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
}