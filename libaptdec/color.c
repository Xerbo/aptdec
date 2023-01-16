/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2019-2023 Xerbo (xerbo@protonmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <aptdec.h>
#include "util.h"

#define MCOMPOSITE(m1, a1, m2, a2) (m1 * a1 + m2 * a2 * (1 - a1))

// clang-format off

apt_rgb_t apt_gradient(const uint32_t *gradient, uint8_t val) {
    return (apt_rgb_t) {
        (gradient[val] & 0x00FF0000) >> 16,
        (gradient[val] & 0x0000FF00) >> 8,
        (gradient[val] & 0x000000FF)
    };
}

apt_rgb_t apt_composite_rgb(apt_rgb_t top, float top_alpha, apt_rgb_t bottom, float bottom_alpha) {
    return (apt_rgb_t) {
        MCOMPOSITE(top.r, top_alpha, bottom.r, bottom_alpha),
        MCOMPOSITE(top.g, top_alpha, bottom.g, bottom_alpha),
        MCOMPOSITE(top.b, top_alpha, bottom.b, bottom_alpha)
    };
}

// Taken from WXtoImg
const uint32_t temperature_gradient[256] = {
    0x45008F, 0x460091, 0x470092, 0x480094, 0x490096, 0x4A0098, 0x4B009B, 
    0x4D009D, 0x4E00A0, 0x5000A2, 0x5100A5, 0x5200A7, 0x5400AA, 0x5600AE, 
    0x5700B1, 0x5800B4, 0x5A00B7, 0x5C00BA, 0x5E00BD, 0x5F00C0, 0x6100C4, 
    0x6400C8, 0x6600CB, 0x6800CE, 0x6900D1, 0x6800D4, 0x6500D7, 0x6300DA, 
    0x6100DD, 0x5D00E1, 0x5B00E4, 0x5900E6, 0x5600E9, 0x5300EB, 0x5000EE, 
    0x4D00F0, 0x4900F3, 0x4700FC, 0x4300FA, 0x3100BF, 0x200089, 0x200092, 
    0x1E0095, 0x1B0097, 0x19009A, 0x17009C, 0x15009E, 0x1200A0, 0x0F00A3, 
    0x0F02A5, 0x0E06A8, 0x0E0AAB, 0x0E0DAD, 0x0E11B1, 0x0D15B4, 0x0D18B7, 
    0x0D1CBA, 0x0B21BD, 0x0A25C0, 0x0A29C3, 0x092DC6, 0x0833CA, 0x0736CD, 
    0x073BD0, 0x0741D3, 0x0545D6, 0x044BD9, 0x0450DC, 0x0355DE, 0x025DE2, 
    0x0161E5, 0x0066E7, 0x006CEA, 0x0072EC, 0x0078EE, 0x007DF0, 0x0082F3, 
    0x008DFC, 0x0090FA, 0x0071BF, 0x005489, 0x005C91, 0x006194, 0x006496, 
    0x006897, 0x006D99, 0x00719B, 0x00759D, 0x00799F, 0x007EA0, 0x0082A2, 
    0x0087A4, 0x008CA6, 0x0092AA, 0x0096AC, 0x009CAE, 0x00A2B1, 0x00A6B3, 
    0x00AAB5, 0x00ADB7, 0x00B1BA, 0x00B6BE, 0x00BAC0, 0x00BEC2, 0x00C2C5, 
    0x00C6C6, 0x00CAC9, 0x00CCCA, 0x00CFCB, 0x00D2CC, 0x00D4CC, 0x00D6CC, 
    0x00D9CB, 0x00DBCB, 0x00DECB, 0x00E0CB, 0x00E2CC, 0x00EAD2, 0x00EACF, 
    0x00B9A4, 0x008E7A, 0x01947C, 0x049779, 0x079975, 0x099B71, 0x0D9D6B, 
    0x109F67, 0x12A163, 0x15A35F, 0x17A559, 0x1AA855, 0x1DAA50, 0x20AC4B, 
    0x24AF45, 0x28B241, 0x2BB53B, 0x2EB835, 0x31BA30, 0x34BD2B, 0x39BF24, 
    0x3FC117, 0x49C508, 0x4FC801, 0x4FCA00, 0x4ECD00, 0x4ECF00, 0x4FD200, 
    0x54D500, 0x5DD800, 0x68DB00, 0x6EDD00, 0x74DF00, 0x7AE200, 0x7FE400, 
    0x85E700, 0x8BE900, 0x8FEB00, 0x9BF300, 0x9EF200, 0x7EBB00, 0x608A00, 
    0x689200, 0x6D9500, 0x719600, 0x759800, 0x7B9A00, 0x7F9D00, 0x839F00, 
    0x87A100, 0x8CA200, 0x8FA500, 0x92A700, 0x96A900, 0x9AAD00, 0x9DB000, 
    0xA1B200, 0xA5B500, 0xA9B700, 0xADBA00, 0xB2BD00, 0xB6BF00, 0xBBC300, 
    0xBFC600, 0xC3C800, 0xC8CB00, 0xCCCE00, 0xD0D100, 0xD3D200, 0xD5D400, 
    0xD9D400, 0xDCD400, 0xDED500, 0xE1D500, 0xE3D500, 0xE6D400, 0xE8D100, 
    0xEACE00, 0xF2CF00, 0xF2CA00, 0xBB9900, 0x8A6E00, 0x927200, 0x957200, 
    0x977100, 0x9A7000, 0x9C6E00, 0x9E6D00, 0xA06B00, 0xA36A00, 0xA56800, 
    0xA86700, 0xAB6600, 0xAE6500, 0xB26300, 0xB46100, 0xB75F00, 0xBA5D00, 
    0xBD5C00, 0xC05900, 0xC35700, 0xC65400, 0xCA5000, 0xCD4D00, 0xD04A00, 
    0xD34700, 0xD64300, 0xD94000, 0xDC3D00, 0xDE3900, 0xE23300, 0xE52F00, 
    0xE72C00, 0xEA2800, 0xEC2300, 0xEF1F00, 0xF11A00, 0xF31400, 0xFB0F00, 
    0xFA0D00, 0xC10500, 0x8E0000, 0x970000, 0x9B0000, 0x9E0000, 0xA10000, 
    0xA50000, 0xA90000, 0xAD0000, 0xB10000, 0xB60000, 0xBA0000, 0xBD0000, 
    0xC20000, 0xC80000, 0xCC0000, 0xCC0000
};

// Taken from WXtoImg
const uint32_t precipitation_gradient[58] = {
    0x088941, 0x00C544, 0x00D12C, 0x00E31C, 0x00F906, 0x14FF00, 0x3EFF00, 
    0x5DFF00, 0x80FF00, 0xABFF00, 0xCDFE00, 0xF8FF00, 0xFFE600, 0xFFB800, 
    0xFF9800, 0xFF7500, 0xFF4900, 0xFE2600, 0xFF0400, 0xDF0000, 0xA80000, 
    0x870000, 0x5A0000, 0x390000, 0x110000, 0x0E1010, 0x232222, 0x333333, 
    0x414141, 0x535353, 0x606060, 0x6E6E6E, 0x808080, 0x8E8E8E, 0xA0A0A0, 
    0xAEAEAE, 0xC0C0C0, 0xCECECE, 0xDCDCDC, 0xEFEFEF, 0xFAFAFA, 0xFFFFFF, 
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 
    0xFFFFFF, 0xFFFFFF
};
