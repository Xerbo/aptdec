/* 
 *  This file is part of Aptdec.
 *  Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019-2020
 *
 *  Aptdec is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#define SYNC_WIDTH 39
#define SPC_WIDTH  47
#define TELE_WIDTH 45
#define CH_WIDTH   909
#define FRAME_LEN  128
#define CH_OFFSET  (SYNC_WIDTH+SPC_WIDTH+CH_WIDTH+TELE_WIDTH)
#define IMG_WIDTH  2080
#define CHA_OFFSET (SYNC_WIDTH+SPC_WIDTH)
#define CHB_OFFSET (SYNC_WIDTH+SPC_WIDTH+CH_WIDTH+TELE_WIDTH+SYNC_WIDTH+SPC_WIDTH)
#define TOTAL_TELE (SYNC_WIDTH+SPC_WIDTH+TELE_WIDTH+SYNC_WIDTH+SPC_WIDTH+TELE_WIDTH)
