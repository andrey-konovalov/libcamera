/* SPDX-License-Identifier: BSD-2-Clause */
/*
From http://jgt.akpeters.com/papers/McGuire08/

Efficient, High-Quality Bayer Demosaic Filtering on GPUs

Morgan McGuire

This paper appears in issue Volume 13, Number 4.
---------------------------------------------------------
Copyright (c) 2008, Morgan McGuire. All rights reserved.


Modified by Linaro Ltd to integrate it into libcamera, and to
fix the artifacts due to pixel coordinates interpolation.
Copyright (C) 2021, Linaro
*/

//Pixel Shader

varying vec2 textureOut;

uniform vec2 tex_size; /* The texture size in pixels */
uniform vec2 tex_step;

/** Pixel position of the first red pixel in the */
/**  Bayer pattern.  [{0,1}, {0, 1}]*/
uniform vec2            tex_bayer_first_red;

/** Monochrome RGBA or GL_LUMINANCE Bayer encoded texture.*/
uniform sampler2D       tex_y;

void main(void) {
    #define fetch(x, y) texture2D(tex_y, vec2(x, y)).r

    /** .xy = Pixel being sampled in the fragment shader on the range [0, 1]
        .zw = ...on the range [0, sourceSize], offset by firstRed */
    vec4            center;

    /** center.x + (-2/w, -1/w, 1/w, 2/w); These are the x-positions */
    /** of the adjacent pixels.*/
    vec4            xCoord;

    /** center.y + (-2/h, -1/h, 1/h, 2/h); These are the y-positions */
    /** of the adjacent pixels.*/
    vec4            yCoord;

    /* Align the center coordinates to the nearest pixel */
    center.zw = floor(textureOut * tex_size);
    center.xy = center.zw * tex_step;
    center.zw += tex_bayer_first_red;

    xCoord = center.x + vec4(-2.0 * tex_step.x,
                             -tex_step.x, tex_step.x, 2.0 * tex_step.x);
    yCoord = center.y + vec4(-2.0 * tex_step.y,
                              -tex_step.y, tex_step.y, 2.0 * tex_step.y);

    float C = texture2D(tex_y, center.xy).r; // ( 0, 0)
    const vec4 kC = vec4( 4.0,  6.0,  5.0,  5.0) / 8.0;

    // Determine which of four types of pixels we are on.
    vec2 alternate = mod(floor(center.zw), 2.0);

    vec4 Dvec = vec4(
        fetch(xCoord[1], yCoord[1]),  // (-1,-1)
        fetch(xCoord[1], yCoord[2]),  // (-1, 1)
        fetch(xCoord[2], yCoord[1]),  // ( 1,-1)
        fetch(xCoord[2], yCoord[2])); // ( 1, 1)

    vec4 PATTERN = (kC.xyz * C).xyzz;

    // Can also be a dot product with (1,1,1,1) on hardware where that is
    // specially optimized.
    // Equivalent to: D = Dvec[0] + Dvec[1] + Dvec[2] + Dvec[3];
    Dvec.xy += Dvec.zw;
    Dvec.x  += Dvec.y;

    vec4 value = vec4(
        fetch(center.x, yCoord[0]),   // ( 0,-2)
        fetch(center.x, yCoord[1]),   // ( 0,-1)
        fetch(xCoord[0], center.y),   // (-2, 0)
        fetch(xCoord[1], center.y));  // (-1, 0)

    vec4 temp = vec4(
        fetch(center.x, yCoord[3]),   // ( 0, 2)
        fetch(center.x, yCoord[2]),   // ( 0, 1)
        fetch(xCoord[3], center.y),   // ( 2, 0)
        fetch(xCoord[2], center.y));  // ( 1, 0)

    // Even the simplest compilers should be able to constant-fold these to
    // avoid the division.
    // Note that on scalar processors these constants force computation of some
    // identical products twice.
    const vec4 kA = vec4(-1.0, -1.5,  0.5, -1.0) / 8.0;
    const vec4 kB = vec4( 2.0,  0.0,  0.0,  4.0) / 8.0;
    const vec4 kD = vec4( 0.0,  2.0, -1.0, -1.0) / 8.0;

    // Conserve constant registers and take advantage of free swizzle on load
    #define kE (kA.xywz)
    #define kF (kB.xywz)

    value += temp;

    // There are five filter patterns (identity, cross, checker,
    // theta, phi).  Precompute the terms from all of them and then
    // use swizzles to assign to color channels.
    //
    // Channel   Matches
    //   x       cross   (e.g., EE G)
    //   y       checker (e.g., EE B)
    //   z       theta   (e.g., EO R)
    //   w       phi     (e.g., EO R)
    #define A (value[0])
    #define B (value[1])
    #define D (Dvec.x)
    #define E (value[2])
    #define F (value[3])

    // Avoid zero elements. On a scalar processor this saves two MADDs
    // and it has no effect on a vector processor.
    PATTERN.yzw += (kD.yz * D).xyy;

    PATTERN += (kA.xyz * A).xyzx + (kE.xyw * E).xyxz;
    PATTERN.xw  += kB.xw * B;
    PATTERN.xz  += kF.xz * F;

    gl_FragColor.rgb = (alternate.y == 0.0) ?
        ((alternate.x == 0.0) ?
            vec3(C, PATTERN.xy) :
            vec3(PATTERN.z, C, PATTERN.w)) :
        ((alternate.x == 0.0) ?
            vec3(PATTERN.w, C, PATTERN.z) :
            vec3(PATTERN.yx, C));
}
