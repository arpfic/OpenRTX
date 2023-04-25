/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <rtxlink.h>
//#include <string.h>
//#include <state.h>
#include <fmp.h>

/**
 * Datastructure for describing a memory device over rtxlink
 */
struct __attribute__((packed)) meminfo_t {
    uint32_t size;
    char name[22];
    uint8_t index;
};
// sizeof(struct meminfo_t) == 32;

/**
 * FMP Protocol Opcodes
 */
enum opcode {
    FMP_OP_ACK     = 0x00,
    FMP_OP_MEMINFO = 0x01,
    FMP_OP_DUMP    = 0x02,
    FMP_OP_FLASH   = 0x03,
    FMP_OP_READ    = 0x04,
    FMP_OP_WRITE   = 0x05,
    FMP_OP_LIST    = 0x06,
    FMP_OP_MOVE    = 0x07,
    FMP_OP_COPY    = 0x08,
    FMP_OP_MKDIR   = 0x09,
    FMP_OP_RM      = 0x0a,
    FMP_OP_RESET   = 0xff,
};

static void fmp_sendAck(enum opcode opcode, const uint8_t status) {
    uint8_t reply[3];
    reply[0] = opcode; // Opcode byte
    reply[1] = status; // Status code
    reply[1] = 0;      // 0 Extra params

    rtxlink_send(RTXLINK_FRAME_FMP, reply, 3);
}

static void fmp_opMemInfo() {
    // TODO: Allocate memory for the whole frame
    uint8_t reply[3];
    reply[0] = FMP_OP_MEMINFO; // Opcode byte
    reply[1] = OK; // Status code
    reply[1] = 0;      // 0 Extra params

    rtxlink_send(RTXLINK_FRAME_FMP, reply, 3);
}

/**
 * \internal
 * FMP protocol handler for rtxlink.
 */
static void fmp_protoCallback(const uint8_t *data, const size_t len)
{
    size_t         alen = len - 1;
    const uint8_t *args = data + 1;

    switch(data[0])
    {
        case FMP_OP_MEMINFO:  fmp_opMemInfo(); break; // Request Memory Information
        default:              fmp_sendAck(FMP_OP_ACK, EBADRQC); break; // Invalid opcode
    }
}

void fmp_init()
{
    rtxlink_setProtocolHandler(RTXLINK_FRAME_FMP, fmp_protoCallback);
}

void fmp_terminate()
{
    rtxlink_removeProtocolHandler(RTXLINK_FRAME_FMP);
}
