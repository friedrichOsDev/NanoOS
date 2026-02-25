/**
 * @file pic.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define PIC1_COMMAND 0x20 /**< IO base address for master PIC */
#define PIC1_DATA    0x21 /**< Data port for master PIC */
#define PIC2_COMMAND 0xA0 /**< IO base address for slave PIC */
#define PIC2_DATA    0xA1 /**< Data port for slave PIC */

#define ICW1_INIT    0x11 /**< Initialization - required! */
#define ICW4_8086    0x01 /**< 8086/88 (MCS-80/85) mode */

void pic_remap(void);