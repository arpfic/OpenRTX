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
#include <string.h>
#include <state.h>
#include <cat.h>

enum errno
{
    OK       = 0,    // Success
    E2BIG    = 7,    // Argument list too long
    EBADR    = 53,   // Invalid request descriptor
    EBADRQC  = 56,   // Invalid request code
    EGENERIC = 255   // Generic error
};

enum pktId
{
    CAT_PKT_GET  = 0x47,
    CAT_PKT_SET  = 0x53,
    CAT_PKT_PEEK = 0x50,
    CAT_PKT_ACK  = 0x41,
    CAT_PKT_DATA = 0x44,
};

/**
 * \internal
 * Send a CAT ACK reply.
 *
 * @param status: status/error code.
 */
static inline void cat_sendAck(const uint8_t status)
{
    uint8_t reply[2];
    reply[0] = CAT_PKT_ACK;
    reply[1] = status;

    rtxlink_send(RTXLINK_FRAME_CAT, reply, 2);
}

/**
 * \internal
 * Handle the CAT "GET" command.
 *
 */
static void cat_cmdGet(const uint8_t *data, const size_t len)
{
    if(len < 3)
        cat_sendAck(EBADR);

    uint8_t reply[17] = {0};
    size_t  replyLen  = 0;
    reply[0]          = CAT_PKT_DATA;

    uint16_t id = (data[0] << 8) | data[1];
    switch(id)
    {
        case 0x494E:    // Radio name
        {
            const hwInfo_t *hwinfo = platform_getHwInfo();
            replyLen = sizeof(hwinfo->name);
            if(replyLen > 16) replyLen = 16;
            memcpy(&reply[1], hwinfo->name, replyLen);
            break;
        }
        case 0x5246:    // Receive Frequency
        {
            replyLen = 4;
            memcpy(&reply[1], &state.channel.rx_frequency, 4);
            break;
        }
        case 0x5446:    // Transmit Frequency
        {
            replyLen = 4;
            memcpy(&reply[1], &state.channel.tx_frequency, 4);
            break;
        }

        default:
            cat_sendAck(EBADR);
            break;
    }

    replyLen += 1;
    rtxlink_send(RTXLINK_FRAME_CAT, reply, replyLen);
}

/**
 * \internal
 * Handle the CAT "SET" command.
 *
 */
static void cat_cmdSet(const uint8_t *data, const size_t len)
{
    if(len < 3)
        cat_sendAck(EBADR);

    uint16_t id = (data[0] << 8) | data[1];
    switch(id)
    {
        case 0x5043:    // Reboot
            break;

        case 0x4654:    // File transfer mode
            break;

        case 0x5346:    // Set frequency
            break;

        default:
            cat_sendAck(EBADR);
            break;
    }

    cat_sendAck(OK);
}

/**
 * \internal
 * Handle the CAT "PEEK" command.
 *
 */
static void cat_cmdPeek(const uint8_t *data, const size_t len)
{
    if(len < 5)
        cat_sendAck(EBADR);

    uint8_t reply[8] = {0};
    reply[0]         = CAT_PKT_DATA;
    uint8_t  dlen = data[0];

    #if __x86_64__
    uint64_t addr = *((uint64_t *) (data + 1));
    #else
    uint32_t addr = *((uint32_t *) (data + 1));
    #endif

    if(dlen > 8)
        cat_sendAck(E2BIG);

    for(int i = 0; i < dlen; i++)
    {
        reply[i + 1] = ((uint8_t *) addr)[i];
    }

    rtxlink_send(RTXLINK_FRAME_CAT, reply, len + 1);
}

/**
 * \internal
 * CAT protocol handler for rtxlink.
 */
static void cat_protoCallback(const uint8_t *data, const size_t len)
{
    size_t         alen = len - 1;
    const uint8_t *args = data + 1;

    switch(data[0])
    {
        case CAT_PKT_GET:  cat_cmdGet (args, alen); break; // Get parameter
        case CAT_PKT_PEEK: cat_cmdPeek(args, alen); break; // Peek memory
        case CAT_PKT_SET:  cat_cmdSet (args, alen); break; // Set parameter
        default:           cat_sendAck(EBADRQC);    break; // Invalid opcode
    }
}


void cat_init()
{
    rtxlink_setProtcolHandler(RTXLINK_FRAME_CAT, cat_protoCallback);
}

void cat_terminate()
{
    rtxlink_removeProtocolHandler(RTXLINK_FRAME_CAT);
}
