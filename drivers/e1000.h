#ifndef _E1000_H
#define _E1000_H

#include <stdint.h>

struct e1000_rx_desc {
    uint64_t addr;
    uint16_t special;
    uint8_t errors;
    uint8_t status;
    uint16_t checksum;
    uint16_t length;
};

struct e1000_tx_desc {
    uint64_t addr;
    uint16_t special;
    uint8_t css;
    uint8_t rsv_sta;
    uint8_t cmd;
    uint8_t cso;
    uint16_t length;
};

// Driver for Intel 82545EM-A (Copper, Server): 8086:100F
uint8_t identifyE1000();
uint8_t* E1000GetMACAddress();

#endif
