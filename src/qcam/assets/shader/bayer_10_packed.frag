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
 * bayer_10_packed.frag - Fragment shader code for raw Bayer 10-bit packed
 * format
 */

#ifdef GL_ES
precision mediump float;
#endif

varying vec2 textureOut;

/* the texture size: tex_size.xy is in bytes, tex_size.zw is in pixels */
uniform vec4 tex_size;
uniform vec2 tex_step;
uniform vec2 tex_bayer_first_red;

uniform sampler2D tex_raw;

void main(void)
{
	vec3 rgb;

	/*
	 * center.xy holds the coordinates of the pixel being sampled
	 * on the [0, 1] range.
	 * center.zw holds the coordinates of the pixel being sampled
	 * on the [0, width/height-1] range.
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
	 * pixel.
	 */
	center.zw = floor(textureOut * tex_size.zw);
	center.y = center.w;
	/*
	 * Add a small number (a few mantissa's LSBs) to avoid float
	 * representation issues. Maybe paranoic.
	 */
	center.x = BPP_X * center.z + 0.02;

	const float threshold_l = 0.127 /* fract(BPP_X) * 0.5 + 0.02 */;
	const float threshold_h = 0.625 /* 1.0 - fract(BPP_X) * 1.5 */;

	float fract_x = fract(center.x);
	/*
	 * The below floor() call ensures that center.x points
	 * at one of the bytes representing the 8 higher bits of
	 * the pixel value, not at the byte containing the LS bits
	 * of the group of the pixels.
	 */
	center.x = floor(center.x);
	center.xy *= tex_step;

	xcoords = center.x + vec2(-tex_step.x, tex_step.x);
	ycoords = center.y + vec2(-tex_step.y, tex_step.y);
	/*
	 * If xcoords[0] points at the byte containing the LS bits
         * of the previous group of the pixels, move xcoords[0] one
	 * byte back.
	 */
	xcoords[0] += (fract_x < threshold_l) ? -tex_step.x : 0.0;
	/*
	 * If xcoords[1] points at the byte containing the LS bits
         * of the current group of the pixels, move xcoords[1] one
	 * byte forward.
	 */
	xcoords[1] += (fract_x > threshold_h) ? tex_step.x : 0.0;

	vec2 alternate = mod(center.zw + tex_bayer_first_red, 2.0);
	bool even_col = alternate.x < 1.0;
	bool even_raw = alternate.y < 1.0;

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
	 * In the below equations (0,-1).r means "r component of the texel
	 * shifted by -tex_step.y from the center.xy one" etc.
	 * In the even raw / even column (EE) case the colour values are:
	 *   R = C = (0,0).r,
	 *   G = (A0 + A1 + B0 + B1) / 4.0 =
	 *       ( (0,-1).r + (0,1).r + (-1,0).r + (1,0).r ) / 4.0,
	 *   B = (D0 + D1 + D2 + D3) / 4.0 =
	 *       ( (-1,-1).r + (1,-1).r + (-1,1).r + (1,1).r ) / 4.0
	 * For even raw / odd column (EO):
	 *   R = (B0 + B1) / 2.0 = ( (-1,0).r + (1,0).r ) / 2.0,
	 *   G = C = (0,0).r,
	 *   B = (A0 + A1) / 2.0 = ( (0,-1).r + (0,1).r ) / 2.0
	 * For odd raw / even column (OE):
	 *   R = (A0 + A1) / 2.0 = ( (0,-1).r + (0,1).r ) / 2.0,
	 *   G = C = (0,0).r,
	 *   B = (B0 + B1) / 2.0 = ( (-1,0).r + (1,0).r ) / 2.0
	 * For odd raw / odd column (OO):
	 *   R = (D0 + D1 + D2 + D3) / 4.0 =
	 *       ( (-1,-1).r + (1,-1).r + (-1,1).r + (1,1).r ) / 4.0,
	 *   G = (A0 + A1 + B0 + B1) / 4.0 =
	 *       ( (0,-1).r + (0,1).r + (-1,0).r + (1,0).r ) / 4.0,
	 *   B = C = (0,0).r
	 */

	/*
	 * Fetch the values and precalculate the terms:
	 *   patterns.x = (A0 + A1) / 2.0
	 *   patterns.y = (B0 + B1) / 2.0
	 *   patterns.z = (A0 + A1 + B0 + B1) / 4.0
	 *   patterns.w = (D0 + D1 + D2 + D3) / 4.0
	 */
	#define fetch(x, y) texture2D(tex_raw, vec2(x, y)).r

	float C = texture2D(tex_raw, center.xy).r;
	vec4 patterns = vec4(
		fetch(center.x, ycoords[0]),	/* A0: (0,-1) */
		fetch(xcoords[0], center.y),	/* B0: (-1,0) */
		fetch(xcoords[0], ycoords[0]),	/* D0: (-1,-1) */
		fetch(xcoords[1], ycoords[0]));	/* D1: (1,-1) */
	vec4 temp = vec4(
		fetch(center.x, ycoords[1]),	/* A1: (0,1) */
		fetch(xcoords[1], center.y),	/* B1: (1,0) */
		fetch(xcoords[1], ycoords[1]),	/* D3: (1,1) */
		fetch(xcoords[0], ycoords[1]));	/* D2: (-1,1) */
	patterns = (patterns + temp) * 0.5;
		/* .x = (A0 + A1) / 2.0, .y = (B0 + B1) / 2.0 */
		/* .z = (D0 + D3) / 2.0, .w = (D1 + D2) / 2.0 */
	patterns.w = (patterns.z + patterns.w) * 0.5;
	patterns.z = (patterns.x + patterns.y) * 0.5;

	rgb = (even_col) ?
		((even_raw) ?
			vec3(C, patterns.zw) :
			vec3(patterns.x, C, patterns.y)) :
		((even_raw) ?
			vec3(patterns.y, C, patterns.x) :
			vec3(patterns.wz, C));

	gl_FragColor = vec4(rgb, 1.0);
}
