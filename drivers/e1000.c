#include "e1000.h"

#include "screen.h"
#include "../cpu/pci.h"
#include "../cpu/mmio.h"
#include "../cpu/timer.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../libc/function.h"
#include "../libc/mem.h"

#define E1000_MMIO_START    0x00400000
#define E1000_NUM_RX_DESC   16              //??
#define E1000_NUM_TX_DESC   16

// E82545EM-A (Copper, Server)
#define E82545EM_VENDORID   0x8086
#define E82545EM_DEVICEID   0x100F

#define E1000_REG_CTRL          0x00
#define E1000_REG_EECD          0x10
#define E1000_REG_EERD          0x14
#define E1000_REG_RCTL         0x100   // Receive Control
#define E1000_REG_TCTL         0x400   // Transmit Control
#define E1000_REG_TIPG         0x410   // IPG
#define E1000_REG_TDBAL            0x3800
#define E1000_REG_TDBAH            0x3804
#define E1000_REG_TDLEN            0x3808
#define E1000_REG_TDHEAD            0x3810
#define E1000_REG_TDTAIL            0x3818

#define E1000_REG_CTRL_RESET    (1 << 26)
#define E1000_REG_EECD_EE_PRES  (1 << 8)
#define E1000_REG_EECD_EE_REQ   (1 << 6)
#define E1000_REG_EECD_EE_GNT  (1 << 7)
#define E1000_REG_EERD_START    (1 << 0)
#define E1000_REG_EERD_DONE     (1 << 4)

static struct PCIAddressInfo _pci_address;

static struct e1000_tx_desc tx_descs[E1000_NUM_TX_DESC + 16];
static uint32_t tx_curr = 0;

static void E1000_handler(registers_t reg);
static void E1000txinit();

uint8_t identifyE1000() {
    // probe for E82545EM
    getDeviceInfo(E82545EM_VENDORID, E82545EM_DEVICEID, &_pci_address);
    if (_pci_address.vendor_id != 0xFFFF && _pci_address.device_id != 0xFFFF) {
        kprint("Detected E1000 Network Card (E82545EM-A, For servers, Copper Media)\n");
        kprintf("E1000 BAR0 (MMIO address): %x\n", _pci_address.BAR0);
        kprintf("E1000 BAR1 (I/O address): %x\n", _pci_address.BAR1);

        // Replace BAR0 address with ours
        // uint32_t address = (uint32_t)((_pci_address.bus << 16) | (_pci_address.device << 11) | ((uint32_t)0x80000000));
        // port_dword_out(0xCF8, (address | 0x10));
        // port_dword_out(0xCFC, E1000_MMIO_START);

        // kprint("Reconfigured E1000 address space\n");

        return 1;
    } else {
        kprint("No E1000 card was found\n");
    }

    return 0;
}

static uint16_t i8254x_read_eeprom(uint8_t addr) {
    uint32_t tmp;
    uint16_t data;
    uint32_t E1000BaseAddress = _pci_address.BAR0;

    if((mmio_dword_in(E1000BaseAddress + E1000_REG_EECD) & E1000_REG_EECD_EE_PRES) == 0) {
        kprint("EEPROM present bit is not set for i8254x\n");
        asm("hlt");
    }

    /* Tell the EEPROM to start reading */
    tmp = ((uint32_t)addr & 0xff) << 8;
    tmp |= E1000_REG_EERD_START;
    mmio_dword_out(E1000BaseAddress + E1000_REG_EERD, tmp);

    /* Wait until the read is finished - then the DONE bit is cleared */
    while ((mmio_dword_in(E1000BaseAddress + E1000_REG_EERD) & E1000_REG_EERD_DONE) == 0) {kprint("Waiting for EEPROM...\n");}

    /* Obtain the data */
    data = (uint16_t)(mmio_dword_in(E1000BaseAddress + E1000_REG_EERD) >> 16);

    /* Tell EEPROM to stop reading */
    tmp = mmio_dword_in(E1000BaseAddress + E1000_REG_EERD);
    tmp &= ~(uint32_t)E1000_REG_EERD_START;
    mmio_dword_out(E1000BaseAddress + E1000_REG_EERD, tmp);

    return data;
}

uint8_t initializeE1000() {
    uint32_t E1000BaseAddress = _pci_address.BAR0;

    // First check if PCI Bus Mastering is enabled
    uint16_t commandReg = pciConfigReadWord(_pci_address.bus, _pci_address.device, _pci_address.function, 4);
    uint8_t enabled = (uint8_t)(commandReg & 0x7);

    if (enabled != 7) {
        kprint("E1000 PCI Bus Mastering is DISABLED. Enabling now...\n");
        // we MUST enable PCI Bus Mastering
        uint8_t mask = commandReg |= 7;     // a fancier way of writing 4
        pciConfigWriteWord(_pci_address.bus, _pci_address.device, _pci_address.function, 4, mask);

        // check value
        commandReg = pciConfigReadWord(_pci_address.bus, _pci_address.device, _pci_address.function, 4);
        enabled = (uint8_t)(commandReg & 0x7);
    }

    kprintf("PCI Bus Mastering is %s\n", enabled == 7 ? "ENABLED" : "DISABLED");

    // Software reset
    uint32_t resetCommand;
    mmio_dword_out(E1000BaseAddress + E1000_REG_CTRL, E1000_REG_CTRL_RESET);
    while( ((resetCommand = mmio_dword_in(E1000BaseAddress + E1000_REG_CTRL)) & E1000_REG_CTRL_RESET) != 0) { kprintf("Reset Comm = %x\n", resetCommand); }

    kprint("E1000 Software Reset done!\n");

    E1000txinit();

    kprint("E1000 TX Initialized\n");

    // Enable IRQ callback
    kprintf("RTL IRQ Number: %u\n", _pci_address.irq);

    register_interrupt_handler(_pci_address.irq + 32, E1000_handler);   // our IRQ handlers are shifted by 32 positions (IRQ0=32), may be subject to change?

    return 0;
}

static void i8254x_lock_eeprom() {
    uint32_t tmp = mmio_dword_in(_pci_address.BAR0 + E1000_REG_EECD);

    mmio_dword_out(_pci_address.BAR0 + E1000_REG_EECD, (tmp | E1000_REG_EECD_EE_REQ));
    do {
        tmp = mmio_dword_in(_pci_address.BAR0 + E1000_REG_EECD);
    } while ((tmp & E1000_REG_EECD_EE_GNT) == 0);
}

static void i8254x_unlock_eeprom() {
    uint32_t tmp = mmio_dword_in(_pci_address.BAR0 + E1000_REG_EECD);

    mmio_dword_out(_pci_address.BAR0 + E1000_REG_EECD, (tmp & (~E1000_REG_EECD_EE_REQ)));
}

uint8_t* E1000GetMACAddress() {
    uint8_t* buff = kmalloc(6);

    i8254x_lock_eeprom();
    *((uint16_t *)buff) = i8254x_read_eeprom(0x00);
    *((uint16_t *)(buff + 2)) = i8254x_read_eeprom(0x01);
    *((uint16_t *)(buff + 4)) = i8254x_read_eeprom(0x02);
    i8254x_unlock_eeprom();

    return buff;
}

static void E1000_handler(registers_t reg) {
    kprint("Reached E1000 IRQ handler\n");

    UNUSED(reg);
}

// static void E1000rxinit()
// {
//     uint8_t * ptr;
//     struct e1000_rx_desc *descs;

//     // Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
//     // In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones
 
//     ptr = (uint8_t *)(kmalloc(sizeof(struct e1000_rx_desc)*E1000_NUM_RX_DESC + 16));

//     descs = (struct e1000_rx_desc *)ptr;
//     for(int i = 0; i < E1000_NUM_RX_DESC; i++)
//     {
//         rx_descs[i] = (struct e1000_rx_desc *)((uint8_t *)descs + i*16);
//         rx_descs[i]->addr = (uint64_t)(uint8_t *)(kmalloc(8192 + 16));
//         rx_descs[i]->status = 0;
//     }

//     mmio_dword_out(REG_TXDESCLO, (uint32_t)((uint64_t)ptr >> 32) );
//     mmio_dword_out(REG_TXDESCHI, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));

//     mmio_dword_out(REG_RXDESCLO, (uint64_t)ptr);
//     mmio_dword_out(REG_RXDESCHI, 0);

//     mmio_dword_out(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

//     mmio_dword_out(REG_RXDESCHEAD, 0);
//     mmio_dword_out(REG_RXDESCTAIL, E1000_NUM_RX_DESC-1);
//     rx_cur = 0;
//     mmio_dword_out(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_8192);
    
// }


static void E1000txinit()
{    
    uint32_t E1000BaseAddress = _pci_address.BAR0;

    for(int i = 0; i < E1000_NUM_TX_DESC; i++)
    {
        tx_descs[i].addr = 0;
        tx_descs[i].cmd = 0;
        tx_descs[i].rsv_sta = 1;   // DD
    }

    // All our addresses are 32 bit
    mmio_dword_out(E1000BaseAddress + E1000_REG_TDBAH, 0);
    mmio_dword_out(E1000BaseAddress + E1000_REG_TDBAL, (uintptr_t)tx_descs);


    //now setup total length of descriptors
    mmio_dword_out(E1000BaseAddress + E1000_REG_TDLEN, E1000_NUM_TX_DESC * 16);


    //setup numbers
    mmio_dword_out( E1000BaseAddress + E1000_REG_TDHEAD, 0);
    mmio_dword_out( E1000BaseAddress + E1000_REG_TDTAIL, 0);
    tx_curr = 0;
    mmio_dword_out(E1000BaseAddress + E1000_REG_TCTL,  (1 << 1) // EN
        | (1 << 3)  // PSP
        | (15 << 4) // CT
        | (64 << 12)   // Full Duplex Collision Distance
        | (1 << 24));

    // This line of code overrides the one before it but I left both to highlight that the previous one works with e1000 cards, but for the e1000e cards 
    // you should set the TCTRL register as follows. For detailed description of each bit, please refer to the Intel Manual.
    // In the case of I217 and 82577LM packets will not be sent if the TCTRL is not configured using the following bits.
    // mmio_dword_out(REG_TCTRL,  0b0110000000000111111000011111010);
    mmio_dword_out(E1000BaseAddress + E1000_REG_TIPG,  0x0060200A);

}