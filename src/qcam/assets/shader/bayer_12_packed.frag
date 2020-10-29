/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Based on the code from http://jgt.akpeters.com/papers/McGuire08/
 *
 * Efficient, High-Quality Bayer Demosaic Filtering on GPUs
 *
 * Morgan McGuire
 *
 * This paper appears in issue Volume 13, Number 4.
 * ---------------------------------------------------------
 * Copyright (c) 2008, Morgan McGuire. All rights reserved.
 *
 *
 * Modified by Linaro Ltd for 12-bit packed vs 8-bit raw Bayer format,
 * and for simpler demosaic algorithm.
 * Copyright (C) 2020, Linaro
 *
 * bayer_12_packed.frag - Fragment shader code for raw Bayer 12-bit packed
 * format
 */

#ifdef GL_ES
precision mediump float;
#endif

varying vec2 textureOut;

uniform vec2 tex_size;
uniform vec2 tex_step;
uniform vec2 tex_bayer_first_red;

uniform sampler2D tex_raw;

void main(void)
{
	vec3 rgb;

	/*
	 * center holds the coordinates of the pixel being sampled.
	 * center.xy are on the [0, 1] range,
	 * center.zw are in pixels: [0, frame width - 1] and
	 * [0, frame height - 1] respectively.
	 */
	vec4 center;
	/*
	 * x-positions of the adjacent pixels on the [0, 1] range.
	 */
	vec2 xcoords;
	/*
	 * y-positions of the adjacent pixels on the [0, 1] range.
	 */
	vec2 ycoords;

	/*
	 * The coordinates passed to the shader in textureOut may point
	 * to a place in between the pixels if the viewfinder window is scaled
	 * from the original captured frame size. Align them to the nearest
	 * texel (center.xy) and pixel (center.zw).
	 */
	center.xy = textureOut * tex_size;	/* [0, texture size) */
	center.z = floor(center.x * 2.0);
	center.xy = floor(center.xy);
	center.w = center.y;
	center.xy *= tex_step;	/* on the [0, 1] range */

	vec2 alternate = mod(center.zw + tex_bayer_first_red, 2.0);
	bool even_col = alternate.x < 1.0;
	bool even_raw = alternate.y < 1.0;

	xcoords = center.x + vec2(-tex_step.x, tex_step.x);
	ycoords = center.y + vec2(-tex_step.y, tex_step.y);

	/*
	 * We need to sample the central pixel and the ones with offset
	 * of -1 to +1 pixel in both X and Y directions. Let's name these
	 * pixels as below, where C is the central pixel:
	 *   +----+----+----+----+
	 *   | \ x|    |    |    |
	 *   |y \ | -1 |  0 | +1 | 
	 *   +----+----+----+----+
	 *   | +1 | D2 | A1 | D3 |
	 *   +----+----+----+----+
	 *   |  0 | B0 |  C | B1 |
	 *   +----+----+----+----+
	 *   | -1 | D0 | A0 | D1 |
	 *   +----+----+----+----+
	 * As we use RGB texture, and take only 8 MS bits of every pixel value,
	 * the blue component is never used.
	 * In the below equations (0,-1).r means "r component of the texel
	 * shifted by -tex_step.y from the center.xy one" etc.
	 * In the even raw / even column (EE) case the colour values are:
	 *   R = C = (0,0).r,
	 *   G = (A0 + A1 + B0 + B1) / 4.0 =
	 *       ( (0,-1).r + (0,1).r + (-1,0).g + (0,0).g ) / 4.0,
	 *   B = (D0 + D1 + D2 + D3) / 4.0 =
	 *       ( (-1,-1).g + (0,-1).g + (-1,1).g + (0,1).g ) / 4.0
	 * For even raw / odd column (EO):
	 *   R = (B0 + B1) / 2.0 = ( (0,0).r + (1,0).r ) / 2.0,
	 *   G = C = (0,0).g,
	 *   B = (A0 + A1) / 2.0 = ( (0,-1).g + (0,1).g ) / 2.0
	 * For odd raw / even column (OE):
	 *   R = (A0 + A1) / 2.0 = ( (0,-1).r + (0,1).r ) / 2.0,
	 *   G = C = (0,0).r,
	 *   B = (B0 + B1) / 2.0 = ( (-1,0).g + (0,0).g ) / 2.0
	 * For odd raw / odd column (OO):
	 *   R = (D0 + D1 + D2 + D3) / 4.0 =
	 *       ( (0,-1).r + (1,-1).r + (0,1).r + (1,1).r ) / 4.0,
	 *   G = (A0 + A1 + B0 + B1) / 4.0 =
	 *       ( (0,-1).g + (0,1).g + (0,0).r + (1,0).r ) / 4.0,
	 *   B = C = (0,0).g
	 */

	/* Fetch all the values we might need */

	/* .xy = (0,-1).rg, zw = (0, 1).rg - all Ax, and some of Dx values */
	vec4 vals_AD = vec4(
		texture2D(tex_raw, vec2(center.x, ycoords[0])).rg,
		texture2D(tex_raw, vec2(center.x, ycoords[1])).rg);
	/*
	 * .xy = (0,0).rg, .z = (-1,0).g, .w = (1,0).r
	 * - all Bx and C, some of Dx values
	 */
	vec4 vals_BCD = vec4(
		texture2D(tex_raw, center.xy).rg,
		texture2D(tex_raw, vec2(xcoords[0], center.y)).g,
		texture2D(tex_raw, vec2(xcoords[1], center.y)).r);
	/*
	 * .x = (-1,-1).g, .y = (-1,1).g, .z = (1,-1).r, .w = (1,1).r
	 * - the remaining Dx values
	 */
	vec4 vals_D = vec4(
		texture2D(tex_raw, vec2(xcoords[0], ycoords[0])).g,
		texture2D(tex_raw, vec2(xcoords[0], ycoords[1])).g,
		texture2D(tex_raw, vec2(xcoords[1], ycoords[0])).r,
		texture2D(tex_raw, vec2(xcoords[1], ycoords[1])).r);

	/*
	 * Let:
	 *   E = ( (0,-1).r + (0, 1).r ) / 2.0
	 *   F = ( (0,-1).g + (0, 1).g ) / 2.0
	 *   G = ( (0,0).g + (-1,0).g ) / 2.0
	 *   H = ( (0,0).r + (1,0).r ) / 2.0
	 *   J = ( (-1,-1).g + (-1,1).g ) / 2.0
	 *   K = ( (1,-1).r + (1,1).r ) / 2.0
	 * Then:
	 *   Even raw, even column case (EE):
	 *     Red = (0,0).r,
	 *     Green = (E + G) / 2.0,
	 *     Blue = (F + J) / 2.0
	 *   EO:
	 *     Red = H,
	 *     Green = (0,0).g,
	 *     Blue = F
	 *   OE:
	 *     Red = E,
	 *     Green = (0,0).r,
	 *     Blue = G
	 *   OO:
	 *     R = (E + K) / 2.0,
	 *     G = (F + H ) / 2.0,
	 *     B = (0,0).g
	 * Then if we let:
	 *   L = (E + G) / 2.0
	 *   M = (F + H) / 2.0
	 * and modify J and K like this:
	 *   J = (F + J) / 2.0
	 *   K = (E + K) / 2.0
	 * the Red/Green/Blue output values can be obtained from EFGH and JKLM
	 * with swizzles thus eliminating the branches.
	 */

	vec4 EFGH = vec4(
		vals_AD.x + vals_AD.z,
		vals_AD.y + vals_AD.w,
		vals_BCD.y + vals_BCD.z,
		vals_BCD.x + vals_BCD.w
		) / 2.0;
	vec4 JKLM = vec4(
		vals_D.x + vals_D.y,
		vals_D.z + vals_D.w,
		EFGH.x + EFGH.z,	/* E + G */
		EFGH.y + EFGH.w		/* F + H */
		) / 2.0;

	JKLM.xy = (JKLM.xy + EFGH.yx) / 2.0;

	rgb = (even_col) ?
		((even_raw) ?
			vec3(vals_BCD.x, JKLM.z, JKLM.x) :
			vec3(EFGH.x, vals_BCD.x, EFGH.z)) :
		((even_raw) ?
			vec3(EFGH.w, vals_BCD.y, EFGH.y) :
			vec3(JKLM.y, JKLM.w, vals_BCD.y));

	gl_FragColor = vec4(rgb, 1.0);
}
