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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

static int ptyFd;

void com_init()
{
    ptyFd = posix_openpt(O_RDWR);
    if(ptyFd < 0)
    {
        fprintf(stderr, "Error %d on posix_openpt()\n", errno);
        exit(1);
    }

    int rc = grantpt(ptyFd);
    if (rc != 0)
    {
        fprintf(stderr, "Error %d on grantpt()\n", errno);
        exit(1);
    }

    rc = unlockpt(ptyFd);
    if (rc != 0)
    {
        fprintf(stderr, "Error %d on unlockpt()\n", errno);
        exit(1);
    }

    printf("Successfully open pseudoTTY on %s\n", ptsname(ptyFd));
}

void com_terminate()
{

}

ssize_t com_writeBlock(const void* buf, const size_t len)
{
    if(ptyFd < 0) return -1;
    return write(ptyFd, buf, len);
}

ssize_t com_readBlock(void* buf, const size_t len)
{
    if(ptyFd < 0) return -1;
    return read(ptyFd, buf, len);
}
