/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * format_convert.h - qcam - Convert buffer to RGB
 */
#ifndef __QCAM_FORMAT_CONVERTER_H__
#define __QCAM_FORMAT_CONVERTER_H__

#include <stddef.h>

#include <QSize>

#include <libcamera/pixel_format.h>

class QImage;

class FormatConverter
{
public:
	int configure(const libcamera::PixelFormat &format, const QSize &size);

	void convert(const unsigned char *src, size_t size, QImage *dst);

private:
	enum FormatFamily {
		MJPEG,
		NV,
		RGB,
		YUV,
	};

	void convertNV(const unsigned char *src, unsigned char *dst);
	void convertRGB(const unsigned char *src, unsigned char *dst);
	void convertYUV(const unsigned char *src, unsigned char *dst);

	libcamera::PixelFormat format_;
	unsigned int width_;
	unsigned int height_;

	enum FormatFamily formatFamily_;

	union {
		/* NV parameters */
		struct nv_params {
			unsigned int horzSubSample;
			unsigned int vertSubSample;
			bool nvSwap;
		} nv;

		/* RGB parameters */
		struct rgb_params {
			unsigned int bpp;
			unsigned int r_pos;
			unsigned int g_pos;
			unsigned int b_pos;
		} rgb;

		/* YUV parameters */
		struct yuv_params {
			unsigned int y_pos;
			unsigned int cb_pos;
		} yuv;
	} params_;

};

#endif /* __QCAM_FORMAT_CONVERTER_H__ */
