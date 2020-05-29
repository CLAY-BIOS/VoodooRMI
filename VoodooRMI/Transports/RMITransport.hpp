/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2020 Avery Black
 * Ported to macOS from linux kernel, original source at
 * https://github.com/torvalds/linux/blob/master/drivers/input/rmi4/rmi_smbus.c
 * https://github.com/torvalds/linux/blob/master/drivers/input/rmi4/rmi_i2c.c
 *
 * Copyright (c) 2011-2016 Synaptics Incorporated
 * Copyright (c) 2011 Unixphere
 */

#ifndef RMITransport_H
#define RMITransport_H

#include <IOKit/IOService.h>
#include "../LinuxCompat.h"
#include "VoodooSMBus.hpp"

enum {
    kSmbusAlert = iokit_vendor_specific_msg(2046)
};

#define super IOService

class RMITransport : public IOService {
    OSDeclareDefaultStructors(RMITransport);
    
public:
    virtual int read(u16 addr, u8 *buf);
    // rmi_read_block
    virtual int readBlock(u16 rmiaddr, u8 *databuff, size_t len);
    // rmi_write
    virtual int write(u16 rmiaddr, u8 *buf);
    // rmi_block_write
    virtual int blockWrite(u16 rmiaddr, u8 *buf, size_t len);
    
    inline IOReturn message(UInt32 type, IOService *provider, void *argument = 0) override {
        IOService *client = getClient();
        if (!client) return kIOReturnError;
        
        switch (type) {
            case kSmbusAlert:
                return messageClient(kSmbusAlert, client);
            default:
                return super::message(type, provider, argument);
        }
    };
};

//OSDefineMetaClassAndAbstractStructors(RMITransport, IOService)

// VoodooSMBus/VoodooSMBusDeviceNub.hpp
#define I2C_CLIENT_HOST_NOTIFY          0x40    /* We want to use I2C host notify */
#define SMB_PROTOCOL_VERSION_ADDRESS    0xfd
#define SMB_MAX_COUNT                   32
#define RMI_SMB2_MAP_SIZE               8 /* 8 entry of 4 bytes each */
#define RMI_SMB2_MAP_FLAGS_WE           0x01

struct mapping_table_entry {
    __le16 rmiaddr;
    u8 readcount;
    u8 flags;
};

class RMISMBus : public RMITransport {
    OSDeclareDefaultStructors(RMISMBus);
    
public:
    bool init(OSDictionary *dictionary) override;
    RMISMBus *probe(IOService *provider, SInt32 *score) override;
    bool start(IOService *provider) override;
    void free() override;
    
    int read(u16 addr, u8 *buf) override;
    int readBlock(u16 rmiaddr, u8 *databuff, size_t len) override;
    int write(u16 rmiaddr, u8 *buf) override;
    int blockWrite(u16 rmiaddr, u8 *buf, size_t len) override;
private:
    VoodooSMBusDeviceNub *device_nub;
    IOLock *page_mutex;
    IOLock *mapping_table_mutex;
    
    struct mapping_table_entry mapping_table[RMI_SMB2_MAP_SIZE];
    u8 table_index;
    
    int rmi_smb_get_version();
    int rmi_smb_get_command_code(u16 rmiaddr, int bytecount,
                                 bool isread, u8 *commandcode);
};

class RMII2C : public RMITransport {
    OSDeclareDefaultStructors(RMII2C);
};

#endif // RMITransport_H