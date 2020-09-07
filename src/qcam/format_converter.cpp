/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * format_convert.cpp - qcam - Convert buffer to RGB
 */

#include "format_converter.h"

#include <errno.h>

#include <QImage>
#include <QtDebug>

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
		params_.nv.horzSubSample = 2;
		params_.nv.vertSubSample = 2;
		params_.nv.nvSwap = false;
		break;
	case libcamera::formats::NV21:
		formatFamily_ = NV;
		params_.nv.horzSubSample = 2;
		params_.nv.vertSubSample = 2;
		params_.nv.nvSwap = true;
		break;
	case libcamera::formats::NV16:
		formatFamily_ = NV;
		params_.nv.horzSubSample = 2;
		params_.nv.vertSubSample = 1;
		params_.nv.nvSwap = false;
		break;
	case libcamera::formats::NV61:
		formatFamily_ = NV;
		params_.nv.horzSubSample = 2;
		params_.nv.vertSubSample = 1;
		params_.nv.nvSwap = true;
		break;
	case libcamera::formats::NV24:
		formatFamily_ = NV;
		params_.nv.horzSubSample = 1;
		params_.nv.vertSubSample = 1;
		params_.nv.nvSwap = false;
		break;
	case libcamera::formats::NV42:
		formatFamily_ = NV;
		params_.nv.horzSubSample = 1;
		params_.nv.vertSubSample = 1;
		params_.nv.nvSwap = true;
		break;

	case libcamera::formats::RGB888:
		formatFamily_ = RGB;
		params_.rgb.r_pos = 2;
		params_.rgb.g_pos = 1;
		params_.rgb.b_pos = 0;
		params_.rgb.bpp = 3;
		break;
	case libcamera::formats::BGR888:
		formatFamily_ = RGB;
		params_.rgb.r_pos = 0;
		params_.rgb.g_pos = 1;
		params_.rgb.b_pos = 2;
		params_.rgb.bpp = 3;
		break;
	case libcamera::formats::ARGB8888:
		formatFamily_ = RGB;
		params_.rgb.r_pos = 2;
		params_.rgb.g_pos = 1;
		params_.rgb.b_pos = 0;
		params_.rgb.bpp = 4;
		break;
	case libcamera::formats::RGBA8888:
		formatFamily_ = RGB;
		params_.rgb.r_pos = 3;
		params_.rgb.g_pos = 2;
		params_.rgb.b_pos = 1;
		params_.rgb.bpp = 4;
		break;
	case libcamera::formats::ABGR8888:
		formatFamily_ = RGB;
		params_.rgb.r_pos = 0;
		params_.rgb.g_pos = 1;
		params_.rgb.b_pos = 2;
		params_.rgb.bpp = 4;
		break;
	case libcamera::formats::BGRA8888:
		formatFamily_ = RGB;
		params_.rgb.r_pos = 1;
		params_.rgb.g_pos = 2;
		params_.rgb.b_pos = 3;
		params_.rgb.bpp = 4;
		break;

	case libcamera::formats::VYUY:
		formatFamily_ = YUV;
		params_.yuv.y_pos = 1;
		params_.yuv.cb_pos = 2;
		break;
	case libcamera::formats::YVYU:
		formatFamily_ = YUV;
		params_.yuv.y_pos = 0;
		params_.yuv.cb_pos = 3;
		break;
	case libcamera::formats::UYVY:
		formatFamily_ = YUV;
		params_.yuv.y_pos = 1;
		params_.yuv.cb_pos = 0;
		break;
	case libcamera::formats::YUYV:
		formatFamily_ = YUV;
		params_.yuv.y_pos = 0;
		params_.yuv.cb_pos = 1;
		break;

	case libcamera::formats::SRGGB10_CSI2P:
		formatFamily_ = RAW_CSI2P;
		params_.rawp.bpp_numer = 5;	/* 1.25 bytes per pixel */
		params_.rawp.bpp_denom = 4;
		params_.rawp.r_pos = 0;
		break;

	case libcamera::formats::SGRBG10_CSI2P:
		formatFamily_ = RAW_CSI2P;
		params_.rawp.bpp_numer = 5;
		params_.rawp.bpp_denom = 4;
		params_.rawp.r_pos = 1;
		break;

	case libcamera::formats::SGBRG10_CSI2P:
		formatFamily_ = RAW_CSI2P;
		params_.rawp.bpp_numer = 5;
		params_.rawp.bpp_denom = 4;
		params_.rawp.r_pos = 2;
		break;

	case libcamera::formats::SBGGR10_CSI2P:
		formatFamily_ = RAW_CSI2P;
		params_.rawp.bpp_numer = 5;
		params_.rawp.bpp_denom = 4;
		params_.rawp.r_pos = 3;
		break;

	case libcamera::formats::SRGGB12_CSI2P:
		formatFamily_ = RAW_CSI2P;
		params_.rawp.bpp_numer = 3;	/* 1.5 bytes per pixel */
		params_.rawp.bpp_denom = 2;
		params_.rawp.r_pos = 0;
		break;

	case libcamera::formats::SGRBG12_CSI2P:
		formatFamily_ = RAW_CSI2P;
		params_.rawp.bpp_numer = 3;
		params_.rawp.bpp_denom = 2;
		params_.rawp.r_pos = 1;
		break;

	case libcamera::formats::SGBRG12_CSI2P:
		formatFamily_ = RAW_CSI2P;
		params_.rawp.bpp_numer = 3;
		params_.rawp.bpp_denom = 2;
		params_.rawp.r_pos = 2;
		break;

	case libcamera::formats::SBGGR12_CSI2P:
		formatFamily_ = RAW_CSI2P;
		params_.rawp.bpp_numer = 3;
		params_.rawp.bpp_denom = 2;
		params_.rawp.r_pos = 3;
		break;

	case libcamera::formats::MJPEG:
		formatFamily_ = MJPEG;
		break;

	default:
		return -EINVAL;
	};

	format_ = format;
	width_ = size.width();
	height_ = size.height();

	if (formatFamily_ == RAW_CSI2P) {
		/*
		 * For RAW_CSI2P the assumption is that height_ and width_
		 * are even numbers.
		 */
		if ( width_ % 2 != 0 || height_ % 2 != 0 ) {
			qWarning() << "Image width or height isn't even number";
			return -EINVAL;
		}

		params_.rawp.srcLineLength = width_ * params_.rawp.bpp_numer /
					     params_.rawp.bpp_denom;

		/*
		 * Calculate [g1,g2,b]_pos based on r_pos.
		 * On entrance, r_pos is the position of red pixel in the
		 * 2-by-2 pixel Bayer pattern, and is from 0 to 3 range:
		 *    +---+---+
		 *    | 0 | 1 |
		 *    +---+---+
		 *    | 2 | 3 |
		 *    +---+---+
		 * At exit, [r,g1,g2,b]_pos are offsetts of the color
		 * values in the source buffer.
		 */
		if (params_.rawp.r_pos > 1) {
			params_.rawp.r_pos = params_.rawp.r_pos - 2 +
					     params_.rawp.srcLineLength;
			params_.rawp.b_pos = 3 - params_.rawp.r_pos;
		} else {
			params_.rawp.b_pos = 1 - params_.rawp.r_pos +
					     params_.rawp.srcLineLength;
		}
		params_.rawp.g1_pos = (params_.rawp.r_pos == 0 ||
				       params_.rawp.b_pos == 0) ? 1 : 0;
		params_.rawp.g2_pos = 1 - params_.rawp.g1_pos +
				      params_.rawp.srcLineLength;
	}

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
		convertRawCSI2P(src, dst->bits());
		break;
	};
}

void FormatConverter::convertRawCSI2P(const unsigned char *src,
				      unsigned char *dst)
{
	unsigned int g;
	static unsigned char dst_buf[8] = { 0, 0, 0, 0xff };
	unsigned int dstLineLength = width_ * 4;

	for (unsigned int y = 0; y < height_; y += 2) {
		for (unsigned x = 0; x < width_; x += params_.rawp.bpp_denom) {
			for (unsigned int i = 0; i < params_.rawp.bpp_denom ;
			     i += 2) {
				/*
				 * Process the current 2x2 group.
				 * Use the average of the two green pixels as
				 * the green value for all the pixels in the
				 * group.
				 */
				dst_buf[0] = src[params_.rawp.b_pos];
				g = src[params_.rawp.g1_pos];
				g = (g + src[params_.rawp.g2_pos]) >> 1;
				dst_buf[1] = (unsigned char)g;
				dst_buf[2] = src[params_.rawp.r_pos];
				src += 2;

				memcpy(dst_buf + 4, dst_buf, 4);
				memcpy(dst, dst_buf, 8);
				memcpy(dst + dstLineLength, dst_buf, 8);
				dst += 8;
			}
			src += params_.rawp.bpp_numer - params_.rawp.bpp_denom;
		}
		/* odd lines are copies of the even lines they follow: */
		memcpy(dst, dst-dstLineLength, dstLineLength);
		/* move to the next even line: */
		src += params_.rawp.srcLineLength;
		dst += dstLineLength;
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
	unsigned int c_stride = width_ * (2 / params_.nv.horzSubSample);
	unsigned int c_inc = params_.nv.horzSubSample == 1 ? 2 : 0;
	unsigned int cb_pos = params_.nv.nvSwap ? 1 : 0;
	unsigned int cr_pos = params_.nv.nvSwap ? 0 : 1;
	const unsigned char *src_c = src + width_ * height_;
	int r, g, b;

	for (unsigned int y = 0; y < height_; y++) {
		const unsigned char *src_y = src + y * width_;
		const unsigned char *src_cb = src_c +
					      (y / params_.nv.vertSubSample) *
					      c_stride + cb_pos;
		const unsigned char *src_cr = src_c +
					      (y / params_.nv.vertSubSample) *
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
			r = src[params_.rgb.bpp * x + params_.rgb.r_pos];
			g = src[params_.rgb.bpp * x + params_.rgb.g_pos];
			b = src[params_.rgb.bpp * x + params_.rgb.b_pos];

			dst[4 * x + 0] = b;
			dst[4 * x + 1] = g;
			dst[4 * x + 2] = r;
			dst[4 * x + 3] = 0xff;
		}

		src += width_ * params_.rgb.bpp;
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

	cr_pos = (params_.yuv.cb_pos + 2) % 4;
	src_stride = width_ * 2;
	dst_stride = width_ * 4;

	for (src_y = 0, dst_y = 0; dst_y < height_; src_y++, dst_y++) {
		for (src_x = 0, dst_x = 0; dst_x < width_; ) {
			cb = src[src_y * src_stride + src_x * 4 +
				 params_.yuv.cb_pos];
			cr = src[src_y * src_stride + src_x * 4 + cr_pos];

			y = src[src_y * src_stride + src_x * 4 +
				params_.yuv.y_pos];
			yuv_to_rgb(y, cb, cr, &r, &g, &b);
			dst[dst_y * dst_stride + 4 * dst_x + 0] = b;
			dst[dst_y * dst_stride + 4 * dst_x + 1] = g;
			dst[dst_y * dst_stride + 4 * dst_x + 2] = r;
			dst[dst_y * dst_stride + 4 * dst_x + 3] = 0xff;
			dst_x++;

			y = src[src_y * src_stride + src_x * 4 +
				params_.yuv.y_pos + 2];
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
