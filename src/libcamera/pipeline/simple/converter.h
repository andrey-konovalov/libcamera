/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Laurent Pinchart
 *
 * converter.h - Format converter for simple pipeline handler
 */

#ifndef __LIBCAMERA_PIPELINE_SIMPLE_CONVERTER_H__
#define __LIBCAMERA_PIPELINE_SIMPLE_CONVERTER_H__

#include <memory>
#include <tuple>
#include <vector>

#include <libcamera/pixel_format.h>
#include <libcamera/signal.h>

namespace libcamera {

class FrameBuffer;
class MediaDevice;
class Size;
class SizeRange;
struct StreamConfiguration;
class V4L2M2MDevice;

class SimpleConverter
{
public:
	SimpleConverter(MediaDevice *media);

	bool isValid() const { return m2m_ != nullptr; }

	std::vector<PixelFormat> formats(PixelFormat input);
	SizeRange sizes(const Size &input);
	std::tuple<unsigned int, unsigned int>
	strideAndFrameSize(const PixelFormat &pixelFormat, const Size &size);

	int configure(const StreamConfiguration &inputCfg,
		      const StreamConfiguration &outputCfg);
	int exportBuffers(unsigned int count,
			  std::vector<std::unique_ptr<FrameBuffer>> *buffers);

	int start();
	void stop();

	int queueBuffers(FrameBuffer *input, FrameBuffer *output);

	Signal<FrameBuffer *> inputBufferReady;
	Signal<FrameBuffer *> outputBufferReady;

private:
	void m2mInputBufferReady(FrameBuffer *buffer);
	void m2mOutputBufferReady(FrameBuffer *buffer);

	std::unique_ptr<V4L2M2MDevice> m2m_;

	unsigned int inputBufferCount_;
	unsigned int outputBufferCount_;
};

} /* namespace libcamera */

#endif /* __LIBCAMERA_PIPELINE_SIMPLE_CONVERTER_H__ */
