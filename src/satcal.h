/* 
 *  This file is part of Aptdec.
 *  Copyright (c) 2004-2009 Thierry Leconte (F4DWV), Xerbo (xerbo@protonmail.com) 2019-20222
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
		{276.60157f, 0.051045f, 1.36328E-06f},
		{276.62531f, 0.050909f, 1.47266E-06f},
		{276.67413f, 0.050907f, 1.47656E-06f},
		{276.59258f, 0.050966f, 1.47656E-06f}
	},
	{ // Channel radiance coefficient vc, A, B
		{925.4075f, 0.337810f, 0.998719f}, // Channel 4
		{839.8979f, 0.304558f, 0.999024f}, // Channel 5
		{2695.9743f, 1.621256f, 0.998015f} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-4.50f, {4.76f, -0.0932f, 0.0004524f}}, // Channel 4
		{-3.61f, {3.83f, -0.0659f, 0.0002811f}}, // Channel 5
		{0.0f, {0.0f, 0.0f, 0.0f}} // Channel 3B
	}
},
{ // NOAA 16
	{ // PRT coeff d0, d1, d2
		{276.355f, 5.562E-02f, -1.590E-05f},
		{276.142f, 5.605E-02f, -1.707E-05f},
		{275.996f, 5.486E-02f, -1.223E-05f},
		{276.132f, 5.494E-02f, -1.344E-05f}
	},
	{ // Channel radiance coefficient vc, A, B
		{917.2289f, 0.332380f, 0.998522f}, // Channel 4
		{838.1255f, 0.674623f, 0.998363f}, // Channel 5
		{2700.1148f, 1.592459f, 0.998147f} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-2.467f, {2.96f, -0.05411f, 0.00024532f}}, // Channel 4
		{-2.009f, {2.25f, -0.03665f, 0.00014854f}}, // Channel 5
		{0.0f, {0.0f, 0.0f, 0.0f}}  // Channel 3B
	}
},
{ // NOAA 17
	{ // PRT coefficient d0, d1, d2
		{276.628f, 0.05098f, 1.371e-06f},
		{276.538f, 0.05098f, 1.371e-06f},
		{276.761f, 0.05097f, 1.369e-06f},
		{276.660f, 0.05100f, 1.348e-06f}
	},
	{ // Channel radiance coefficient vc, A, B
		{926.2947f, 0.271683f, 0.998794f}, // Channel 4
		{839.8246f, 0.309180f, 0.999012f}, // Channel 5
		{2669.3554f, 1.702380f, 0.997378f} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-8.55f, {8.22f, -0.15795f, 0.00075579f}}, // Channel 4
		{-3.97f, {4.31f, -0.07318f, 0.00030976f}}, // Channel 5
		{0.0f, {0.0f, 0.0f, 0.0f}} // Channel 3B
	}
},
{ // NOAA 18
	{ // PRT coefficient d0, d1, d2
		{276.601f, 0.05090f, 1.657e-06f},
		{276.683f, 0.05101f, 1.482e-06f},
		{276.565f, 0.05117f, 1.313e-06f},
		{276.615f, 0.05103f, 1.484e-06f}
	},
	{ // Channel radiance coefficient vc, A, B
		{928.1460f, 0.436645f, 0.998607f}, // Channel 4
		{833.2532f, 0.253179f, 0.999057f}, // Channel 5
		{2659.7952f, 1.698704f, 0.996960f} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-5.53f, {5.82f, -0.11069f, 0.00052337f}}, // Channel 4
		{-2.22f, {2.67f, -0.04360f, 0.00017715f}}, // Channel 5
		{0.0f, {0.0f, 0.0f, 0.0f}} // Channel 3B
	}
},
{ // NOAA 19
	{ // PRT coefficient d0, d1, d2
		{276.6067f, 0.051111f, 1.405783E-06f},
		{276.6119f, 0.051090f, 1.496037E-06f},
		{276.6311f, 0.051033f, 1.496990E-06f},
		{276.6268f, 0.051058f, 1.493110E-06f}
	},
	{ // Channel radiance coefficient vc, A, B
		{928.9f, 0.53959f, 0.998534f}, // Channel 4
		{831.9f, 0.36064f, 0.998913f}, // Channel 5
		{2670.0f, 1.67396f, 0.997364f} // Channel 3B
	},
	{ // Nonlinear radiance correction Ns, b0, b1, b2
		{-5.49f, {5.70f, -0.11187f, 0.00054668f}}, // Channel 4
		{-3.39f, {3.58f, -0.05991f, 0.00024985f}}, // Channel 5
		{0.0f, {0.0f, 0.0f, 0.0f}} // Channel 3B
	}
}};

const float c1 = 1.1910427e-5f;
const float c2 = 1.4387752f;
