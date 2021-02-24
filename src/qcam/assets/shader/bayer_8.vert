/*
From http://jgt.akpeters.com/papers/McGuire08/

Efficient, High-Quality Bayer Demosaic Filtering on GPUs

Morgan McGuire

This paper appears in issue Volume 13, Number 4.
---------------------------------------------------------
Copyright (c) 2008, Morgan McGuire. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//Vertex Shader

attribute vec4 vertexIn;
attribute vec2 textureIn;

/* The texture size: tex_size.xy is in bytes, tex_size.zw is in pixels */
uniform vec4 tex_size;
uniform vec2 tex_step;

/** Pixel position of the first red pixel in the */
/**  Bayer pattern.  [{0,1}, {0, 1}]*/
uniform vec2            tex_bayer_first_red;

/** .xy = Pixel being sampled in the fragment shader on the range [0, 1]
    .zw = ...on the range [0, sourceSize], offset by firstRed */
varying vec4            center;

/** center.x + (-2/w, -1/w, 1/w, 2/w); These are the x-positions */
/** of the adjacent pixels.*/
varying vec4            xCoord;

/** center.y + (-2/h, -1/h, 1/h, 2/h); These are the y-positions */
/** of the adjacent pixels.*/
varying vec4            yCoord;

void main(void) {
    center.xy = textureIn;
    center.zw = textureIn * tex_size.zw + tex_bayer_first_red;

    xCoord = center.x + vec4(-2.0 * tex_step.x,
                             -tex_step.x, tex_step.x, 2.0 * tex_step.x);
    yCoord = center.y + vec4(-2.0 * tex_step.y,
                              -tex_step.y, tex_step.y, 2.0 * tex_step.y);

    gl_Position = vertexIn;
}
