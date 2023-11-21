#ifndef MSLIP_H
#define MSLIP_H


#include <stdio.h>


#include "crc8.h"


#define SLIP_END     0xc0 /* indicates end of packet */
#define SLIP_ESC     0xdb /* indicates byte stuffing */
#define SLIP_ESC_END 0xdc /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC 0xdd /* ESC ESC_ESC means ESC data byte */

typedef enum{
    OK,
    FAIL
}slip_err_t;


enum{
    PRX_BLE_MESH_COPY=1,
};

#define MAX_PC_MSG_LENGTH 256

typedef struct __attribute__ ((packed)){
    uint16_t src_addr;
    uint16_t dst_addr;
    uint32_t opcode;
    uint16_t length; //length of msg array
    uint8_t msg[];
}ble_msg_copy_t;

typedef struct __attribute__ ((packed)){
    uint16_t length; //length of payload to be sent to PC (raspbery Pi). It include size of 'type' and some of the folLowing struct
    //Used for transmitter and internally while converting to SLIP type message but not transmitted itself//
    union {
        uint8_t raw_arr[MAX_PC_MSG_LENGTH];
        struct {//semi rough struct. Until we became know 'type' actual has to be mapped struct is unknown
            uint8_t type; //type of message to parse properly at reception
            union{
                ble_msg_copy_t msg_copy;
//                struct __attribute__ ((packed)) ble_msg_copy_t_{....
            };
        };
    };
}pc_transm_msg_t;

slip_err_t SLIP_prepare_packet(pc_transm_msg_t* pack) ;
slip_err_t SLIP_parse_packet(pc_transm_msg_t* pack) ;
#endif
