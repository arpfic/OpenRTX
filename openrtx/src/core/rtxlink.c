/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <interfaces/com_port.h>
#include <interfaces/platform.h>
#include <datatypes.h>
#include <rtxlink.h>
#include <slip.h>
#include <state.h>
#include <stdbool.h>
#include <string.h>

enum dataMode
{
    DATAMODE_CAT,
    DATAMODE_FILETRANSFER,
    DATAMODE_XMODEM
};

uint8_t       dataBuf[128];
size_t        dataBufLen = 0;
enum dataMode mode = DATAMODE_CAT;


/**
 * \internal
 * Pack data into a SLIP-encoded data frame and send it over the com port.
 *
 * @param data: payload data.
 * @param size: payload size.
 */
static inline void sendSlipFrame(const uint8_t *data, const size_t size)
{
    size_t len = slip_encode(data, dataBuf, size, true, true);
    com_writeBlock(dataBuf, len);
}

/**
 * \internal
 * Retrieve and decode a SLIP-encoded data frame from the com port.
 *
 * @return true when a data frame has been completely retrieved.
 */
static bool fetchSlipFrame()
{
    static uint8_t rxBuf[64];
    static ssize_t rxBufLen = 0;
    static ssize_t rxBufPos = 0;

    // Fetch data from com port, exit on error or no data received.
    if(rxBufLen <= 0)
    {
        rxBufPos = 0;
        rxBufLen = com_readBlock(&rxBuf, 64);
        if(rxBufLen <= 0)
            return false;
    }

    // Strip the leading END character(s) before start decoding a new frame
    if(dataBufLen == 0)
    {
        while(slip_searchFrameEnd(&rxBuf[rxBufPos], rxBufLen) == 0)
        {
            rxBufPos += 1;
            rxBufLen -= 1;
        }
    }

    // Decode SLIP-encoded data
    ssize_t end      = slip_searchFrameEnd(&rxBuf[rxBufPos], rxBufLen);
    size_t  toDecode = rxBufLen;
    if(end > 0) toDecode = end + 1;

    // Bad, too much bytes
    if((dataBufLen + toDecode) > 128)
    {
        dataBufLen = 0;
        rxBufLen  -= toDecode;
        return false;
    }

    dataBufLen += slip_decodeBlock(&rxBuf[rxBufPos], &dataBuf[dataBufLen], toDecode);
    rxBufLen   -= toDecode;
    rxBufPos   += toDecode;

    // Still some data to receive, wait for next round.
    if(end < 0)
        return false;

    return true;
}


void rtxlink_init()
{

}

void rtxlink_task()
{
    if(fetchSlipFrame() == false)
        return;

    // Handle data
    switch(mode)
    {
        case DATAMODE_CAT:
            break;

        case DATAMODE_FILETRANSFER: break;
        case DATAMODE_XMODEM:       break;
        default: break;
    }

    // Flush old data to start fetching a new frame
    dataBufLen = 0;
}

void rtxlink_terminate()
{

}
