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

#ifndef SLIP_H
#define SLIP_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Encode a block of data according to the SLIP protocol.
 * The frame start and end markers are prepended/appended to the data block only
 * when the corresponding function parameters are set to true, allowing to
 * handle a big frame of data into smaller pieces.
 * Caller must ensure that destination buffer has enough capacity to store the
 * encoded data, which can be up to 2*len + 2 bytes.
 *
 * @param src: data to be encoded.
 * @param dst: destination buffer.
 * @param len: length of orginal data, in bytes.
 * @param start: set to true to prepend the frame start marker at the beginning
 *               of the block.
 * @param end: set to true to append the frame end marker at the end of the block.
 * @return final data length after the encoding.
 */
size_t slip_encode(const void *src, void *dst, const size_t len, const bool start,
                   const bool end);

/**
 * Search for the frame end terminator inside a data block.
 *
 * @param buf: pointer to a slip-encoded data block.
 * @param len: lenght of the slip-encoded data block.
 * @return position of the frame end as an offset from the beginning of the data
 * block or -1 if no end terminator was found.
 */
ssize_t slip_searchFrameEnd(const void *buf, const size_t len);

/**
 * Decode a block of data encoded following the SLIP protocol.
 *
 * @param src: data to be decoded.
 * @param dst: destination buffer.
 * @param len: length of orginal data, in bytes.
 * @return number of bytes decoded from the current data block or -1 in case of
 * errors.
 */
ssize_t slip_decodeBlock(const void *src, const void *dst, const size_t len);

#ifdef __cplusplus
}
#endif

#endif
