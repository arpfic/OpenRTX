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

#include <stddef.h>
#include <slip.h>
#include <stdio.h>

#define END     0xC0
#define ESC     0xDB
#define ESC_END 0xDC
#define ESC_ESC 0xDD


size_t slip_encode(const void *src, void *dst, const size_t len, const bool start,
                   const bool end)
{
    if((src == NULL) || (dst == NULL))
        return 0;

    const uint8_t *input  = (uint8_t *) src;
    uint8_t       *output = (uint8_t *) dst;
    size_t         outIdx = 0;

    if(start == true)
    {
        // Prepend frame start
        output[0] = END;
        outIdx    = 1;
    }

    for(size_t i = 0; i < len; i++)
    {
        uint8_t curByte = input[i];
        switch(curByte)
        {
            case END:
                output[outIdx]     = ESC;
                output[outIdx + 1] = ESC_END;
                outIdx += 2;
                break;

            case ESC:
                output[outIdx]     = ESC;
                output[outIdx + 1] = ESC_ESC;
                outIdx += 2;
                break;

            default:
                output[outIdx] = curByte;
                outIdx += 1;
        }
    }

    if(end == true)
    {
        // Append frame terminator
        output[outIdx] = END;
        outIdx += 1;
    }

    return outIdx;
}

ssize_t slip_searchFrameEnd(const void *buf, const size_t len)
{
    if(len == 0)
        return 0;

    uint8_t *ptr = (uint8_t *) buf;
    for(ssize_t i = 0; i < (ssize_t) len; i++)
    {
        if(ptr[i] == END)
            return i;
    }

    return -1;
}

ssize_t slip_decodeBlock(const void *src, const void *dst, const size_t len)
{
    if((src == NULL) || (dst == NULL))
        return -1;

    size_t decodedBytes = 0;
    size_t decodedPos   = 0;
    uint8_t *input  = (uint8_t *) src;
    uint8_t *output = (uint8_t *) dst;
    uint8_t prev    = 0;

    for(size_t i = 0; i < len; i++)
    {
        uint8_t cur = input[i];
        switch(cur)
        {
            case END:
                return decodedBytes;
                break;

            case ESC:
                // Do nothing and advance
                break;

            case ESC_END:
            {
                if(prev == ESC)
                    output[decodedPos] = END;
                else
                    output[decodedPos] = cur;
                decodedPos   += 1;
                decodedBytes += 1;
            }
                break;

            case ESC_ESC:
            {
                if(prev == ESC)
                    output[decodedPos] = ESC;
                else
                    output[decodedPos] = cur;
                decodedPos   += 1;
                decodedBytes += 1;
            }
                break;

            default:
                output[decodedPos] = cur;
                decodedPos   += 1;
                decodedBytes += 1;
                break;
        }

        prev = cur;
    }

    return decodedBytes;
}
