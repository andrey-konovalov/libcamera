/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * raw12_packed.frag - Fragment shader code for raw Bayer 12-bit packed format
 */

#ifdef GL_ES
precision mediump float;
#endif

varying vec4 center;
varying vec2 xcoords;
varying vec2 ycoords;

uniform sampler2D tex_raw;

void main(void)
{
	vec4 pattern;
	vec3 rgb;

	vec2 alternate = mod(center.zw, 2.0);

	float c = texture2D(tex_raw, center.xy).r;

	vec4 vals = vec4 (
		texture2D(tex_raw, vec2(xcoords[0], ycoords[0])).r,	// D0
		texture2D(tex_raw, vec2(xcoords[0], ycoords[1])).r,	// D1
		texture2D(tex_raw, vec2(xcoords[0], center.y)).r,	// B0
		texture2D(tex_raw, vec2(center.x, ycoords[0])).r);	// A0

	vec4 vals2 = vec4 (
		texture2D(tex_raw, vec2(xcoords[1], ycoords[0])).r,	// D2
		texture2D(tex_raw, vec2(xcoords[1], ycoords[1])).r,	// D3
		texture2D(tex_raw, vec2(xcoords[1], center.y)).r,	// B1
		texture2D(tex_raw, vec2(center.x, ycoords[1])).r);	// A1

	vals += vals2;
	pattern.x = (vals.z + vals.w) / 4.0;
	pattern.y = (vals.x + vals.y) / 4.0;
	pattern.zw /= 2.0;
	/* pattern = vec4((A0+A1+B0+B1)/4, (D0+D1+D2+D3)/4, (B0+B1)/2, (A0+A1)/2) */

	rgb = (alternate.y < 1.0) ?
		((alternate.x < 1.0) ?
			vec3(c, pattern.xy) : vec3(pattern.z, c, pattern.w)) :
		((alternate.x < 1.0) ?
			vec3(pattern.w, c, pattern.z) : vec3(pattern.yx, c));
		
	gl_FragColor = vec4(rgb, 1.0);
}
