/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * bayer.vert - Vertex shader for raw Bayer to RGB conversion
 */

attribute vec4 vertexIn;
attribute vec2 textureIn;

uniform vec2 tex_size;
uniform vec2 tex_step;
uniform vec2 tex_bayer_first_red;

varying vec4 center;
varying vec2 xcoords;
varying vec2 ycoords;

void main(void)
{
	center.xy = textureIn;
	center.zw = textureIn * tex_size + tex_bayer_first_red;

	xcoords = center.x + vec2(-tex_step.x, tex_step.x);
	ycoords = center.y + vec2(-tex_step.y, tex_step.y);

	gl_Position = vertexIn;
}
