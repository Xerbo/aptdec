/*
 *  Atpdec
 *  Copyright (c) 2003 by Thierry Leconte (F4DWV)
 *
 *      $Id$
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define IQFilterLen 32
const float iqfilter[IQFilterLen] = {
-0.0205361, -0.0219524, -0.0235785, -0.0254648,
-0.0276791, -0.0303152, -0.0335063, -0.0374482,
-0.0424413, -0.0489708, -0.0578745, -0.0707355,
-0.0909457, -0.127324, -0.212207, -0.63662,
0.63662, 0.212207, 0.127324, 0.0909457,
0.0707355, 0.0578745, 0.0489708, 0.0424413,
0.0374482, 0.0335063, 0.0303152, 0.0276791,
0.0254648, 0.0235785, 0.0219524, 0.0205361
};

#define SyncFilterLen 32
const float Sync[SyncFilterLen]={
 -14,-14,-14,
 18,18,-14,-14,18,18,-14,-14,18,18,-14,-14,
 18,18,-14,-14,18,18,-14,-14,18,18,-14,-14,
 18,18,-14,-14,-14
};


#define RSFilterLen 75
const float rsfilter[RSFilterLen] = {
0.000684467, 0.000686301, 0.000680827, 0.000657135, 0.000598639, 0.000484572, 0.000292259, -4.07622e-19, 
-0.000409636, -0.000946387, -0.00160904, -0.00238265, -0.00323652, -0.00412324, -0.00497894, -0.00572484, 
-0.00627025, -0.00651673, -0.00636341, -0.00571307, -0.00447871, -0.00259011, 1.72476e-18, 0.00331064, 
0.00732849, 0.0120054, 0.017258, 0.0229688, 0.0289897, 0.0351475, 0.0412507, 0.0470982, 
0.052488, 0.0572273, 0.0611415, 0.0640834, 0.0659403, 0.0666398, 0.0661536, 0.0644993, 
0.061739, 0.0579765, 0.0533519, 0.0480347, 0.0422149, 0.0360944, 0.0298768, 0.023758, 
0.0179179, 0.0125126, 0.00766851, 0.00347853, 1.81998e-18, -0.00274526, -0.00476897, -0.00611273, 
-0.00684295, -0.00704483, -0.00681579, -0.00625878, -0.00547594, -0.00456294, -0.00360441, -0.00267047, 
-0.00181474, -0.00107366, -0.000467084, -4.66468e-19, 0.000334865, 0.000553877, 0.000679047, 0.000734606, 
0.000743914, 0.000726905, 0.00069827 };
