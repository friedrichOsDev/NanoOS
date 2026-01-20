/*
 * @file idt.h
 * @brief Header file for Interrupt Descriptor Table (IDT) implementation
 * @author friedrichOsDev
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

/*
 * Structure representing an IDT entry
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
struct idt_entry {
    uint16_t base_low; 
    uint16_t selector; 
    uint8_t  zero;     
    uint8_t  flags;    
    uint16_t base_high;
} __attribute__((packed));

/*
 * Structure representing a pointer to the IDT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
struct idt_ptr {
    uint16_t limit;       
    uint32_t base;        
} __attribute__((packed));

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif // IDT_H
