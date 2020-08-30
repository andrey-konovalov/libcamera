/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * format_convert.cpp - qcam - Convert buffer to RGB
 */

#include "format_converter.h"

#include <errno.h>

#include <QImage>

#include <libcamera/formats.h>

#define RGBSHIFT		8
#ifndef MAX
#define MAX(a,b)		((a)>(b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b)		((a)<(b)?(a):(b))
#endif
#ifndef CLAMP
#define CLAMP(a,low,high)	MAX((low),MIN((high),(a)))
#endif
#ifndef CLIP
#define CLIP(x)			CLAMP(x,0,255)
#endif

int FormatConverter::configure(const libcamera::PixelFormat &format,
			       const QSize &size)
{
	switch (format) {
	case libcamera::formats::NV12:
		formatFamily_ = NV;
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		nvSwap_ = false;
		break;
	case libcamera::formats::NV21:
		formatFamily_ = NV;
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		nvSwap_ = true;
		break;
	case libcamera::formats::NV16:
		formatFamily_ = NV;
		horzSubSample_ = 2;
		vertSubSample_ = 1;
		nvSwap_ = false;
		break;
	case libcamera::formats::NV61:
		formatFamily_ = NV;
		horzSubSample_ = 2;
		vertSubSample_ = 1;
		nvSwap_ = true;
		break;
	case libcamera::formats::NV24:
		formatFamily_ = NV;
		horzSubSample_ = 1;
		vertSubSample_ = 1;
		nvSwap_ = false;
		break;
	case libcamera::formats::NV42:
		formatFamily_ = NV;
		horzSubSample_ = 1;
		vertSubSample_ = 1;
		nvSwap_ = true;
		break;

	case libcamera::formats::RGB888:
		formatFamily_ = RGB;
		r_pos_ = 2;
		g_pos_ = 1;
		b_pos_ = 0;
		bpp_ = 3;
		break;
	case libcamera::formats::BGR888:
		formatFamily_ = RGB;
		r_pos_ = 0;
		g_pos_ = 1;
		b_pos_ = 2;
		bpp_ = 3;
		break;
	case libcamera::formats::ARGB8888:
		formatFamily_ = RGB;
		r_pos_ = 2;
		g_pos_ = 1;
		b_pos_ = 0;
		bpp_ = 4;
		break;
	case libcamera::formats::RGBA8888:
		formatFamily_ = RGB;
		r_pos_ = 3;
		g_pos_ = 2;
		b_pos_ = 1;
		bpp_ = 4;
		break;
	case libcamera::formats::ABGR8888:
		formatFamily_ = RGB;
		r_pos_ = 0;
		g_pos_ = 1;
		b_pos_ = 2;
		bpp_ = 4;
		break;
	case libcamera::formats::BGRA8888:
		formatFamily_ = RGB;
		r_pos_ = 1;
		g_pos_ = 2;
		b_pos_ = 3;
		bpp_ = 4;
		break;

	case libcamera::formats::VYUY:
		formatFamily_ = YUV;
		y_pos_ = 1;
		cb_pos_ = 2;
		break;
	case libcamera::formats::YVYU:
		formatFamily_ = YUV;
		y_pos_ = 0;
		cb_pos_ = 3;
		break;
	case libcamera::formats::UYVY:
		formatFamily_ = YUV;
		y_pos_ = 1;
		cb_pos_ = 0;
		break;
	case libcamera::formats::YUYV:
		formatFamily_ = YUV;
		y_pos_ = 0;
		cb_pos_ = 1;
		break;

	case libcamera::formats::MJPEG:
		formatFamily_ = MJPEG;
		break;

	/* TBD: add more raw packed formats */
	case libcamera::formats::SRGGB12_CSI2P:
		formatFamily_ = RAW_CSI2P;
		r_pos_ = 0;
		break;

	case libcamera::formats::SGRBG12_CSI2P:
		formatFamily_ = RAW_CSI2P;
		r_pos_ = 1;
		break;

	case libcamera::formats::SGBRG12_CSI2P:
		formatFamily_ = RAW_CSI2P;
		r_pos_ = 2;
		break;

	case libcamera::formats::SBGGR12_CSI2P:
		formatFamily_ = RAW_CSI2P;
		r_pos_ = 3;
		break;

	default:
		return -EINVAL;
	};

	format_ = format;
	width_ = size.width();
	height_ = size.height();

	return 0;
}

void FormatConverter::convert(const unsigned char *src, size_t size,
			      QImage *dst)
{
	switch (formatFamily_) {
	case MJPEG:
		dst->loadFromData(src, size, "JPEG");
		break;
	case YUV:
		convertYUV(src, dst->bits());
		break;
	case RGB:
		convertRGB(src, dst->bits());
		break;
	case NV:
		convertNV(src, dst->bits());
		break;
	case RAW_CSI2P:
		convertRAW_CSI2P(src, dst->bits());
		break;
	};
}

/*
 * The pixels are processed in groups of 4 (2 by 2 squares), and the
 * assumption is that height_ and width_ are even numbers.
 */
void FormatConverter::convertRAW_CSI2P(const unsigned char *src,
				       unsigned char *dst)
{
	unsigned int r_pos, b_pos, g1_pos, g2_pos;
	unsigned char r, g1, g2, b;
	unsigned int s_inc = 3; // RAW12P: bpp = 3/2 = 3 bytes per 2 pixels
	unsigned int s_linelen = width_ * s_inc /  2; // width * bpp

	/*
	 * Calculate the offsets of the colour values in the src buffer.
	 * g1 is green value from the even (upper) line, g2 is the green
	 * value from the odd (lower) line.
	 */
	if ( r_pos_ > 1) {
		r_pos = r_pos_ - 2 + s_linelen;
		b_pos = 3 - r_pos_;
	} else {
		r_pos = r_pos_;
		b_pos = 1 - r_pos_ + s_linelen;
	}
	g1_pos = (r_pos == 0 || b_pos == 0) ? 1 : 0;
	g2_pos = 1 - g1_pos + s_linelen;

	for (unsigned int y = 0; y < height_; y += 2) {
		for (unsigned x =0; x < width_; x += 2) {
			// read the color values for the current 2x2 group:
			r = src[r_pos];
			g1 = src[g1_pos];
			g2 = src[g2_pos];
			b = src[b_pos];
			src += s_inc;
			// two left pixels of the four:
			dst[0] = dst[0 + 4 * width_] = b;
			dst[1] = g1;
			dst[1 + 4 * width_] = g2;
			dst[2] = dst[2 + 4 * width_] = r;
			dst[3] = dst[3 + 4 * width_] = 0xff;
			dst += 4;
			// two right pixels of the four:
			dst[0] = dst[0 + 4 * width_] = b;
			dst[1] = g1;
			dst[1 + 4 * width_] = g2;
			dst[2] = dst[2 + 4 * width_] = r;
			dst[3] = dst[3 + 4 * width_] = 0xff;
			dst += 4;
		}
		// move to the next even line:
		src += s_linelen;
		dst += 4 * width_;
	}
}

static void yuv_to_rgb(int y, int u, int v, int *r, int *g, int *b)
{
	int c = y - 16;
	int d = u - 128;
	int e = v - 128;
	*r = CLIP(( 298 * c           + 409 * e + 128) >> RGBSHIFT);
	*g = CLIP(( 298 * c - 100 * d - 208 * e + 128) >> RGBSHIFT);
	*b = CLIP(( 298 * c + 516 * d           + 128) >> RGBSHIFT);
}

void FormatConverter::convertNV(const unsigned char *src, unsigned char *dst)
{
	unsigned int c_stride = width_ * (2 / horzSubSample_);
	unsigned int c_inc = horzSubSample_ == 1 ? 2 : 0;
	unsigned int cb_pos = nvSwap_ ? 1 : 0;
	unsigned int cr_pos = nvSwap_ ? 0 : 1;
	const unsigned char *src_c = src + width_ * height_;
	int r, g, b;

	for (unsigned int y = 0; y < height_; y++) {
		const unsigned char *src_y = src + y * width_;
		const unsigned char *src_cb = src_c + (y / vertSubSample_) *
					      c_stride + cb_pos;
		const unsigned char *src_cr = src_c + (y / vertSubSample_) *
					      c_stride + cr_pos;

		for (unsigned int x = 0; x < width_; x += 2) {
			yuv_to_rgb(*src_y, *src_cb, *src_cr, &r, &g, &b);
			dst[0] = b;
			dst[1] = g;
			dst[2] = r;
			dst[3] = 0xff;
			src_y++;
			src_cb += c_inc;
			src_cr += c_inc;
			dst += 4;

			yuv_to_rgb(*src_y, *src_cb, *src_cr, &r, &g, &b);
			dst[0] = b;
			dst[1] = g;
			dst[2] = r;
			dst[3] = 0xff;
			src_y++;
			src_cb += 2;
			src_cr += 2;
			dst += 4;
		}
	}
}

void FormatConverter::convertRGB(const unsigned char *src, unsigned char *dst)
{
	unsigned int x, y;
	int r, g, b;

	for (y = 0; y < height_; y++) {
		for (x = 0; x < width_; x++) {
			r = src[bpp_ * x + r_pos_];
			g = src[bpp_ * x + g_pos_];
			b = src[bpp_ * x + b_pos_];

			dst[4 * x + 0] = b;
			dst[4 * x + 1] = g;
			dst[4 * x + 2] = r;
			dst[4 * x + 3] = 0xff;
		}

		src += width_ * bpp_;
		dst += width_ * 4;
	}
}

void FormatConverter::convertYUV(const unsigned char *src, unsigned char *dst)
{
	unsigned int src_x, src_y, dst_x, dst_y;
	unsigned int src_stride;
	unsigned int dst_stride;
	unsigned int cr_pos;
	int r, g, b, y, cr, cb;

	cr_pos = (cb_pos_ + 2) % 4;
	src_stride = width_ * 2;
	dst_stride = width_ * 4;

	for (src_y = 0, dst_y = 0; dst_y < height_; src_y++, dst_y++) {
		for (src_x = 0, dst_x = 0; dst_x < width_; ) {
			cb = src[src_y * src_stride + src_x * 4 + cb_pos_];
			cr = src[src_y * src_stride + src_x * 4 + cr_pos];

			y = src[src_y * src_stride + src_x * 4 + y_pos_];
			yuv_to_rgb(y, cb, cr, &r, &g, &b);
			dst[dst_y * dst_stride + 4 * dst_x + 0] = b;
			dst[dst_y * dst_stride + 4 * dst_x + 1] = g;
			dst[dst_y * dst_stride + 4 * dst_x + 2] = r;
			dst[dst_y * dst_stride + 4 * dst_x + 3] = 0xff;
			dst_x++;

			y = src[src_y * src_stride + src_x * 4 + y_pos_ + 2];
			yuv_to_rgb(y, cb, cr, &r, &g, &b);
			dst[dst_y * dst_stride + 4 * dst_x + 0] = b;
			dst[dst_y * dst_stride + 4 * dst_x + 1] = g;
			dst[dst_y * dst_stride + 4 * dst_x + 2] = r;
			dst[dst_y * dst_stride + 4 * dst_x + 3] = 0xff;
			dst_x++;

			src_x++;
		}
	}
}
