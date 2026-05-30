/**
 * @file ahci.c
 * @author friedrichOsDev
 */

#include <ahci.h>
#include <serial.h>
#include <memory.h>
#include <handler.h>
#include <string.h>
#include <print.h>

static HBA_mem_t* ahci_abar = NULL;
static HBA_cmd_header_t cmd_headers[32][32] __attribute__((aligned(1024)));
static HBA_fis_t        received_fis[32]    __attribute__((aligned(256)));
static HBA_cmd_tbl_t    cmd_tables[32][32]  __attribute__((aligned(128)));

void ahci_interrupt_handler(struct registers* regs) {
    (void)regs;
    serial_printf("AHCI: Interrupt received\n");
    // TODO: Real Handling (based on OsDev Wiki)
}

void ahci_init_device(pci_device_t* dev) {
    serial_printf("Initializing AHCI device at bus %d, device %d, function %d\n", dev->bus, dev->device, dev->function);
    pci_bar_t bar5 = pci_get_bar(dev->bus, dev->device, dev->function, 5);

    if (bar5.base_address == 0 || bar5.type != PCI_BAR_MEM) {
        serial_printf("AHCI: Invalid BAR5\n");
        return;
    }

    HBA_mem_t* abar = (HBA_mem_t*)io_map_permanent(bar5.base_address, bar5.size);

    if (!abar) {
        serial_printf("AHCI: Failed to map ABAR\n");
        return;
    }

    serial_printf("AHCI: ABAR mapped at virtual address %x\n", (uint32_t)abar);
    ahci_abar = abar;

    if (abar->cap2 & (1 << 0)) { // Check if BOHC is supported
        abar->bohc |= (1 << 1); // Set OS ownership request bit
        while (abar->bohc & (1 << 0)); // Wait until BIOS releases
    }

    // Reset AHCI controller
    abar->ghc |= (1 << 0); // Set HBA reset bit
    while (abar->ghc & (1 << 0)); // Wait until reset is
    serial_printf("AHCI: Controller reset complete\n");

    // Register IRQ handler
    uint32_t intr_reg = pci_config_read_dword(dev->bus, dev->device, dev->function, PCI_INTERRUPT_LINE);
    uint8_t irq = (uint8_t)(intr_reg & 0xFF);
    irq_install_handler(irq, (irq_handler_t)ahci_interrupt_handler);
    serial_printf("AHCI: IRQ handler installed for IRQ %d\n", irq);

    // Enable AHCI mode and global interrupts
    abar->ghc |= (1 << 31); // AE: AHCI Enable
    abar->ghc |= (1 << 1);  // IE: Interrupt Enable

    probe_port(abar);
    
}

static int check_type(HBA_port_t* port) {
	uint32_t ssts = port->ssts;

	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;

	if (det != HBA_PORT_DET_PRESENT)	// Check drive status
		return AHCI_DEV_NULL;
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEV_NULL;

	switch (port->sig) {
        case SATA_SIG_ATAPI:
            return AHCI_DEV_SATAPI;
        case SATA_SIG_SEMB:
            return AHCI_DEV_SEMB;
        case SATA_SIG_PM:
            return AHCI_DEV_PM;
        default:
            return AHCI_DEV_SATA;
	}
}

static int ahci_find_cmdslot(HBA_port_t* port) {
    uint32_t slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & (1 << i)) == 0) return i;
    }
    return -1; // No free slot
}

void start_cmd(HBA_port_t* port) {
    // 1. Wait until Command List DMA engine (CR) is idle
    while (port->cmd & (1 << 14));

    // 2. Set FRE (FIS Receive Enable)
    port->cmd |= (1 << 4);
    
    // 3. Set ST (Start)
    port->cmd |= (1 << 0);
}

void stop_cmd(HBA_port_t* port) {
    // 1. Clear ST (Start) and FRE (FIS Receive Enable)
    port->cmd &= ~(1 << 0);
    port->cmd &= ~(1 << 4);

    // 2. Wait until CR (Command List Running) and FR (FIS Receive Running) are cleared
    while (1) {
        if (port->cmd & (1 << 14)) continue;
        if (port->cmd & (1 << 15)) continue;
        break;
    }
}

void probe_port(HBA_mem_t* abar) {
    // Search disk in implemented ports
    uint32_t pi = abar->pi;
    int i = 0;
    while (i < 32) {
        if (pi & 1) {
            int dt = check_type(&abar->ports[i]);
            if (dt == AHCI_DEV_SATA) {
                serial_printf("AHCI: SATA drive found at port %d\n", i);
                port_rebase(&abar->ports[i], i);
                ahci_disk_t* ahci_disk = (ahci_disk_t*)kmalloc(sizeof(ahci_disk_t));
                memset(ahci_disk, 0, sizeof(ahci_disk_t));
                ahci_disk->hba_port = &abar->ports[i];
                ahci_disk->port_num = i;
                
                disk_t* new_disk = &ahci_disk->base;
                snprintf(new_disk->name, 8, "sd%c", 'a' + i);
                new_disk->type = TYPE_AHCI;
                
                ahci_identify(ahci_disk);
                
			} else if (dt == AHCI_DEV_SATAPI) {
                serial_printf("AHCI: SATAPI drive found at port %d\n", i);
                port_rebase(&abar->ports[i], i);
			} else if (dt == AHCI_DEV_SEMB) {
                serial_printf("AHCI: SEMB drive found at port %d\n", i);
                port_rebase(&abar->ports[i], i);
			} else if (dt == AHCI_DEV_PM) {
                serial_printf("AHCI: Port multiplier found at port %d\n", i);
                port_rebase(&abar->ports[i], i);
			} else {
                serial_printf("AHCI: No drive found at port %d\n", i);
			}
		}

		pi >>= 1;
		i ++;
    }
}

void port_rebase(HBA_port_t* port, int port_no) {
    stop_cmd(port);

    // Clear buffers
    memset((void*)&cmd_headers[port_no][0], 0, sizeof(cmd_headers[port_no]));
    memset((void*)&received_fis[port_no], 0, sizeof(received_fis[port_no]));
    memset((void*)&cmd_tables[port_no][0], 0, sizeof(cmd_tables[port_no]));

    uint32_t clb_phys = vmm_virtual_to_physical(vmm_get_page_directory(), (uint32_t)&cmd_headers[port_no][0]);
    uint32_t fb_phys = vmm_virtual_to_physical(vmm_get_page_directory(), (uint32_t)&received_fis[port_no]);

    port->clb = clb_phys;
    port->clbu = 0;
    port->fb = fb_phys;
    port->fbu = 0;

    for (int i = 0; i < 32; i++) {
        uint32_t ctba_phys = vmm_virtual_to_physical(vmm_get_page_directory(), (uint32_t)&cmd_tables[port_no][i]);
        cmd_headers[port_no][i].ctba = ctba_phys;
        cmd_headers[port_no][i].ctbau = 0;
        cmd_headers[port_no][i].prdtl = 1; // One PRDT entry per command table
    }
    
    // Reset Port
    port->serr = (uint32_t)-1; // Clear error register
    port->is = (uint32_t)-1;   // Clear interrupt status    

    start_cmd(port);
}

void ahci_identify(ahci_disk_t* disk) {
    HBA_port_t* port = disk->hba_port;

    port->is = (uint32_t)-1;

    int slot = ahci_find_cmdslot(port);
    if (slot == -1) {
        serial_printf("AHCI: No free command slots on port %d\n", disk->port_num);
        return;
    }
    
    HBA_cmd_header_t* cmdheader = &cmd_headers[disk->port_num][slot];
    cmdheader->cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
    cmdheader->w = 0;
    cmdheader->prdtl = 1;

    uint16_t identify_buf[256];
    memset(identify_buf, 0, sizeof(identify_buf));

    HBA_cmd_tbl_t* cmdtbl = &cmd_tables[disk->port_num][slot];
    memset(cmdtbl, 0, sizeof(HBA_cmd_tbl_t));

    uint32_t buf_phys = vmm_virtual_to_physical(vmm_get_page_directory(), (uint32_t)identify_buf);
    cmdtbl->prdt_entry[0].dba = buf_phys;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = 511;
    cmdtbl->prdt_entry[0].i = 1;

    fis_reg_h2d_t* cmdfis = (fis_reg_h2d_t*)(cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = ATA_CMD_IDENTIFY;
    cmdfis->device = 0;

    uint32_t timeout = 1000000;
    while ((port->tfd & (AHCI_DEV_BUSY | AHCI_DEV_DRQ)) && timeout) timeout--;

    if (timeout == 0) {
        serial_printf("AHCI: Port %d hung before IDENTIFY\n", disk->port_num);
        return;
    }

    port->ci = (1 << slot);

    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & HBA_PxIS_TFES) {
            serial_printf("AHCI: IDENTIFY failed on port %d\n", disk->port_num);
            return;
        }
    }

    disk->base.sector_size = 512;
    uint64_t sectors = 0;

    if (identify_buf[83] & (1 << 10)) {
        sectors = *((uint64_t*)&identify_buf[100]);
    } else {
        sectors = *((uint32_t*)&identify_buf[60]);
    }

    disk->base.total_sectors = sectors;

    char model[41];
    int char_idx = 0;
    for (int i = 27; i <= 46; i++) {
        model[char_idx++] = (char)(identify_buf[i] >> 8);
        model[char_idx++] = (char)(identify_buf[i] & 0xFF);
    }
    model[40] = '\0';

    for (int i = 39; i >= 0 && model[i] == ' '; i--) model[i] = '\0';

    serial_printf("AHCI: Drive '%s' registered: %llu sectors (~%llu GB)\n", model, sectors, (sectors * 512) / (1024 * 1024 * 1024));

    disk->base.read = ahci_read_sectors;   
    disk->base.write = ahci_write_sectors; 

    storage_register_disk(&disk->base);
    serial_printf("AHCI: Disk '%s' registered in storage\n", model);
}

uint8_t ahci_read_sectors(disk_t* self, uint64_t lba, uint32_t count, void* buffer) {
    ahci_disk_t* disk = (ahci_disk_t*)self;
    HBA_port_t* port = disk->hba_port;

    port->is = (uint32_t)-1;

    int slot = ahci_find_cmdslot(port);
    if (slot == -1) {
        serial_printf("AHCI: No free command slots on port %d\n", disk->port_num);
        return 1;
    }
     
    
    HBA_cmd_header_t* cmdheader = &cmd_headers[disk->port_num][slot];
    cmdheader->cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t); 
    cmdheader->w = 0; 
    cmdheader->prdtl = 1; 

    HBA_cmd_tbl_t* cmdtbl = &cmd_tables[disk->port_num][slot];
    memset(cmdtbl, 0, sizeof(HBA_cmd_tbl_t));

    uint32_t buffer_phys = vmm_virtual_to_physical(vmm_get_page_directory(), (uint32_t)buffer);
    
    cmdtbl->prdt_entry[0].dba = buffer_phys;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = (count * self->sector_size) - 1; 
    cmdtbl->prdt_entry[0].i = 1; 

    fis_reg_h2d_t* cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1; 
    cmdfis->command = ATA_CMD_READ_DMA_EX;

    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 1 << 6; 

    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);

    cmdfis->countl = (uint8_t)(count & 0xFF);
    cmdfis->counth = (uint8_t)((count >> 8) & 0xFF);

    uint32_t spin = 0;
    while ((port->tfd & (AHCI_DEV_BUSY | AHCI_DEV_DRQ)) && spin < 1000000) {
        spin++;
    }
    if (spin == 1000000) {
        serial_printf("AHCI: Port %d is hung before issuing read!\n", disk->port_num);
        return 1;
    }

    port->ci = (1 << slot);

    while (1) {
        if ((port->ci & (1 << slot)) == 0) {
            break;
        }
        
        if (port->is & (1 << 30)) {
            serial_printf("AHCI: Read disk error on port %d!\n", disk->port_num);
            return 1; 
        }
    }

    if (port->is & (1 << 30)) {
        serial_printf("AHCI: Read disk error on port %d!\n", disk->port_num);
        return 1;
    }

    return 0;
}

uint8_t ahci_write_sectors(disk_t* self, uint64_t lba, uint32_t count, const void* buffer) {
    ahci_disk_t* disk = (ahci_disk_t*)self;
    HBA_port_t* port = disk->hba_port;

    port->is = (uint32_t)-1;

    int slot = ahci_find_cmdslot(port);
    if (slot == -1) {
        serial_printf("AHCI: No free command slots on port %d\n", disk->port_num);
        return 1;
    }
     
    
    HBA_cmd_header_t* cmdheader = &cmd_headers[disk->port_num][slot];
    cmdheader->cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t); 
    cmdheader->w = 1; 
    cmdheader->prdtl = 1; 

    HBA_cmd_tbl_t* cmdtbl = &cmd_tables[disk->port_num][slot];
    memset(cmdtbl, 0, sizeof(HBA_cmd_tbl_t));

    uint32_t buffer_phys = vmm_virtual_to_physical(vmm_get_page_directory(), (uint32_t)buffer);
    
    cmdtbl->prdt_entry[0].dba = buffer_phys;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = (count * self->sector_size) - 1; 
    cmdtbl->prdt_entry[0].i = 1; 

    fis_reg_h2d_t* cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1; 
    cmdfis->command = ATA_CMD_WRITE_DMA_EX;

    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 1 << 6; 

    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);

    cmdfis->countl = (uint8_t)(count & 0xFF);
    cmdfis->counth = (uint8_t)((count >> 8) & 0xFF);

    uint32_t spin = 0;
    while ((port->tfd & (AHCI_DEV_BUSY | AHCI_DEV_DRQ)) && spin < 1000000) {
        spin++;
    }
    if (spin == 1000000) {
        serial_printf("AHCI: Port %d is hung before issuing write!\n", disk->port_num);
        return 1;
    }

    port->ci = (1 << slot);

    while (1) {
        if ((port->ci & (1 << slot)) == 0) {
            break;
        }
        
        if (port->is & (1 << 30)) {
            serial_printf("AHCI: Write disk error on port %d!\n", disk->port_num);
            return 1; 
        }
    }

    if (port->is & (1 << 30)) {
        serial_printf("AHCI: Write disk error on port %d!\n", disk->port_num);
        return 1;
    }

    return 0;
}