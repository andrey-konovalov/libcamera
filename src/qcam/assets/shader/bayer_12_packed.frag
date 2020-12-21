/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
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

	vec4 center;
	vec2 xcoords;
	vec2 ycoords;

	center.xy = textureOut * tex_size; /* center.xy - coords in texels */
	center.z = floor(center.x * 2.0);  /* center.zw - coords in pixels */
	center.xy = floor(center.xy);	/* center.xy set to nearest texel */
	center.w = center.y;		/* center.zw set to nearest pixel */

	vec2 alternate = mod(center.zw + tex_bayer_first_red, 2.0);
	bool even_col = alternate.x < 1.0;
	bool even_raw = alternate.y < 1.0;

	center.xy *= tex_step;	/* center.xy converted to texture coordinates */
	xcoords = center.x + vec2(-tex_step.x, tex_step.x);
	ycoords = center.y + vec2(-tex_step.y, tex_step.y);

	/* .xy = (0,-1).rg, zw = (0, 1).rg */
	vec4 vals_AD = vec4(
		texture2D(tex_raw, vec2(center.x, ycoords[0])).rg,
		texture2D(tex_raw, vec2(center.x, ycoords[1])).rg);
	/* .xy = (0,0).rg, .z = (-1,0).g, .w = (1,0).r */
	vec4 vals_BCD = vec4(
		texture2D(tex_raw, center.xy).rg,
		texture2D(tex_raw, vec2(xcoords[0], center.y)).g,
		texture2D(tex_raw, vec2(xcoords[1], center.y)).r);
	/* .x = (-1,-1).g, .y = (-1,1).g, .z = (1,-1).r, .w = (1,1).r */
	vec4 vals_D = vec4(
		texture2D(tex_raw, vec2(xcoords[0], ycoords[0])).g,
		texture2D(tex_raw, vec2(xcoords[0], ycoords[1])).g,
		texture2D(tex_raw, vec2(xcoords[1], ycoords[0])).r,
		texture2D(tex_raw, vec2(xcoords[1], ycoords[1])).r);

	vec4 EFGH = vec4(
		vals_AD.x + vals_AD.z,		/* 2*E = (0,-1).r + (0, 1).r */
		vals_AD.y + vals_AD.w, 		/* 2*F = (0,-1).g + (0, 1).g */
		vals_BCD.y + vals_BCD.z,	/* 2*G = (0,0).g + (-1,0).g */
		vals_BCD.x + vals_BCD.w		/* 2*H = (0,0).r + (1,0).r */
		) / 2.0;
	vec2 JK = vec2(
		vals_D.x + vals_D.y,		/* 2*J = (-1,-1).g + (-1,1).g */
		vals_D.z + vals_D.w		/* 2*K = (1,-1).r + (1,1).r */
		) / 2.0;

	if (even_col) {
		rgb = (even_raw) ?
			vec3(vals_BCD.x, (EFGH.x + EFGH.z) / 2.0,
			     (EFGH.y + JK.x) / 2.0) :
			vec3(EFGH.x, vals_BCD.x, EFGH.z);
	} else {
		rgb = (even_raw) ?
			vec3(EFGH.w, vals_BCD.y, EFGH.y) :
			vec3((EFGH.x + JK.y) / 2.0,
			     (EFGH.y + EFGH.w) / 2.0,
			     vals_BCD.y);
	}
	gl_FragColor = vec4(rgb, 1.0);
}
