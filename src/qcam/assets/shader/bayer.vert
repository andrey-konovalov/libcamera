/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * bayer.vert - Vertex shader for raw Bayer to RGB conversion
 */

attribute vec4 vertexIn;
attribute vec2 textureIn;

uniform vec4 srcSize;
uniform vec2 firstRed;

varying vec4 center;
varying vec2 xcoords;
varying vec2 ycoords;

void main(void)
{
	center.xy = textureIn;
	center.zw = textureIn * srcSize.xy + firstRed;

	vec2 invSize = srcSize.zw;
	xcoords = center.x + vec2(-invSize.x, invSize.x);
	ycoords = center.y + vec2(-invSize.y, invSize.y);

	gl_Position = vertexIn;
}
