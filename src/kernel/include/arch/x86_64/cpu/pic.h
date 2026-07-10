/**
 * @file pic.h
 * @brief PIC remapping (Header)
 * @author friedrichOsDev
 */

#pragma once

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define ICW1_INIT 0x11
#define ICW4_8086 0x01

void pic_remap();