// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// BlueKitchen BTStack config

#ifndef _PLATFORM_PRIME_HUB_SPIKE_RT_BTSTACK_CONFIG_H_
#define _PLATFORM_PRIME_HUB_SPIKE_RT_BTSTACK_CONFIG_H_

// BTstack features that can be enabled
#define ENABLE_BLE
// #define ENABLE_CC256X_BAUDRATE_CHANGE_FLOWCONTROL_BUG_WORKAROUND
#define ENABLE_LE_CENTRAL
#define ENABLE_LE_PERIPHERAL
#define ENABLE_PRINTF_HEXDUMP

// #define ENABLE_LOG_DEBUG
// #define ENABLE_LOG_ERROR
// #define ENABLE_LOG_INFO

// BTstack configuration. buffers, sizes, ...
#define HCI_ACL_PAYLOAD_SIZE (1691 + 4)
#define MAX_ATT_DB_SIZE 512
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES  0
#define MAX_NR_GATT_CLIENTS 1
#define MAX_NR_HCI_CONNECTIONS 2 // CC2564C can have up to 10 connections
#define MAX_NR_HFP_CONNECTIONS 0
#define MAX_NR_L2CAP_CHANNELS  0
#define MAX_NR_L2CAP_SERVICES  0
#define MAX_NR_RFCOMM_CHANNELS 0
#define MAX_NR_RFCOMM_MULTIPLEXERS 0
#define MAX_NR_RFCOMM_SERVICES 0
#define MAX_NR_SERVICE_RECORD_ITEMS 0
#define MAX_NR_SM_LOOKUP_ENTRIES 0
#define MAX_NR_WHITELIST_ENTRIES 0
#define MAX_NR_LE_DEVICE_DB_ENTRIES 1

// Link Key DB and LE Device DB using TLV on top of Flash Sector interface
// #define NVM_NUM_DEVICE_DB_ENTRIES 16
// #define NVM_NUM_LINK_KEYS 16

#endif // _PLATFORM_PRIME_HUB_SPIKE_RT_BTSTACK_CONFIG_H_
