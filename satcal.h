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

const struct {
	float d[4][3];
	struct {
		float vc, A, B;
	} rad[3];
	struct {
		float Ns;
		float b[3];
	} cor[3]; 
/* Calibration corficcients taken from the NOAA KLM satellite user guide
 * https://web.archive.org/web/20141220021557/https://www.ncdc.noaa.gov/oa/pod-guide/ncdc/docs/klm/tables.htm 
 */
} satcal[] = {
{ // NOAA 15
	{ // PRT coefficient d0, d1, d2
		{276.60157, 0.051045, 1.36328E-06},
		{276.62531, 0.050909, 1.47266E-06},
		{276.67413, 0.050907, 1.47656E-06},
		{276.59258, 0.050966, 1.47656E-06}
	},
	{ // Channel radiance coefficient vc, A, B
		{925.4075, 0.337810, 0.998719}, // Channel 4
		{839.8979, 0.304558, 0.999024}, // Channel 5
		{2695.9743, 1.621256, 0.998015} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-4.50, {4.76, -0.0932, 0.0004524}}, // Channel 4
		{-3.61, {3.83, -0.0659, 0.0002811}}, // Channel 5
		{0.0, {0.0, 0.0, 0.0}} // Channel 3B
	}
},
{ // NOAA 16
	{ // PRT coeff d0, d1, d2
		{276.355, 5.562E-02, -1.590E-05},
		{276.142, 5.605E-02, -1.707E-05},
		{275.996, 5.486E-02, -1.223E-05},
		{276.132, 5.494E-02, -1.344E-05}
	},
	{ // Channel radiance coefficient vc, A, B
		{917.2289, 0.332380, 0.998522}, // Channel 4
		{838.1255, 0.674623, 0.998363}, // Channel 5
		{2700.1148, 1.592459, 0.998147} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-2.467, {2.96, -0.05411, 0.00024532}}, // Channel 4
		{-2.009, {2.25, -0.03665, 0.00014854}}, // Channel 5
		{0.0, {0.0, 0.0, 0.0}}  // Channel 3B
	}
},
{ // NOAA 17
	{ // PRT coefficient d0, d1, d2
		{276.628, 0.05098, 1.371e-06},
		{276.538, 0.05098, 1.371e-06},
		{276.761, 0.05097, 1.369e-06},
		{276.660, 0.05100, 1.348e-06}
	},
	{ // Channel radiance coefficient vc, A, B
		{926.2947, 0.271683, 0.998794}, // Channel 4
		{839.8246, 0.309180, 0.999012}, // Channel 5
		{2669.3554, 1.702380, 0.997378} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-8.55, {8.22, -0.15795, 0.00075579}}, // Channel 4
		{-3.97, {4.31, -0.07318, 0.00030976}}, // Channel 5
		{0.0, {0.0, 0.0, 0.0}} // Channel 3B
	}
},
{ // NOAA 18
	{ // PRT coefficient d0, d1, d2
		{276.601, 0.05090, 1.657e-06},
		{276.683, 0.05101, 1.482e-06},
		{276.565, 0.05117, 1.313e-06},
		{276.615, 0.05103, 1.484e-06}
	},
	{ // Channel radiance coefficient vc, A, B
		{928.1460, 0.436645, 0.998607}, // Channel 4
		{833.2532, 0.253179, 0.999057}, // Channel 5
		{2659.7952, 1.698704, 0.996960} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-5.53, {5.82, -0.11069, 0.00052337}}, // Channel 4
		{-2.22, {2.67, -0.04360, 0.00017715}}, // Channel 5
		{0.0, {0.0, 0.0, 0.0}} // Channel 3B
	}
},
{ // NOAA 19
	{ // PRT coefficient d0, d1, d2
		{276.6067, 0.051111, 1.405783E-06},
		{276.6119, 0.051090, 1.496037E-06},
		{276.6311, 0.051033, 1.496990E-06},
		{276.6268, 0.051058, 1.493110E-06}
	},
	{ // Channel radiance coefficient vc, A, B
		{928.9, 0.53959, 0.998534}, // Channel 4
		{831.9, 0.36064, 0.998913}, // Channel 5
		{2670.0, 1.67396, 0.997364} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-5.49, {5.70 	-0.11187, 0.00054668}}, // Channel 4
		{-3.39, {3.58 	-0.05991, 0.00024985}}, // Channel 5
		{0.0, {0.0, 0.0, 0.0}} // Channel 3B
	}
}};

const float c1 = 1.1910427e-5;
const float c2 = 1.4387752;