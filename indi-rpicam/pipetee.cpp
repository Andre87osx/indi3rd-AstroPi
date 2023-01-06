/*
 Raspberry Pi High Quality Camera CCD Driver for Indi.
 Copyright (C) 2020 Lars Berntzon (lars.berntzon@cecilia-data.se).
 All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include "pipetee.h"

PipeTee::PipeTee(const std::string &filename) : filename(filename)
{
    reset();
}

PipeTee::~PipeTee()
{
    if (fp) {
        fclose(fp);
        fp = nullptr;
    }
}

void PipeTee::data_received(uint8_t *data,  uint32_t length)
{
    fwrite(&data, 1, length, fp);
    forward(data, length);
}

void PipeTee::reset()
{
    if (fp) {
        fclose(fp);
        fp = nullptr;
    }

    fp = fopen(filename.c_str(), "w");
}
