/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Laurent Pinchart
 *
 * converter.cpp - Format converter for simple pipeline handler
 */

#include "converter.h"

#include <algorithm>

#include <libcamera/buffer.h>
#include <libcamera/geometry.h>
#include <libcamera/signal.h>

#include "log.h"
#include "media_device.h"
#include "v4l2_videodevice.h"

namespace libcamera {

LOG_DECLARE_CATEGORY(SimplePipeline);

SimpleConverter::SimpleConverter(MediaDevice *media)
	: m2m_(nullptr)
{
	/*
	 * Locate the video node. There's no need to validate the pipeline
	 * further, the caller guarantees that this is a V4L2 mem2mem device.
	 */
	const std::vector<MediaEntity *> &entities = media->entities();
	auto it = std::find_if(entities.begin(), entities.end(),
			       [](MediaEntity *entity) {
				       return entity->function() == MEDIA_ENT_F_IO_V4L;
			       });
	if (it == entities.end())
		return;

	m2m_ = new V4L2M2MDevice((*it)->deviceNode());

	m2m_->output()->bufferReady.connect(this, &SimpleConverter::outputBufferReady);
	m2m_->capture()->bufferReady.connect(this, &SimpleConverter::captureBufferReady);
}

SimpleConverter::~SimpleConverter()
{
	delete m2m_;
}

int SimpleConverter::open()
{
	if (!m2m_)
		return -ENODEV;

	return m2m_->open();
}

void SimpleConverter::close()
{
	if (m2m_)
		m2m_->close();
}

std::vector<PixelFormat> SimpleConverter::formats(PixelFormat input)
{
	if (!m2m_)
		return {};

	V4L2DeviceFormat format;
	format.fourcc = m2m_->output()->toV4L2PixelFormat(input);
	format.size = { 1, 1 };

	int ret = m2m_->output()->setFormat(&format);
	if (ret < 0) {
		LOG(SimplePipeline, Error)
			<< "Failed to set format: " << strerror(-ret);
		return {};
	}

	std::vector<PixelFormat> pixelFormats;

	for (const auto &format : m2m_->capture()->formats()) {
		PixelFormat pixelFormat = m2m_->capture()->toPixelFormat(format.first);
		if (pixelFormat)
			pixelFormats.push_back(pixelFormat);
	}

	return pixelFormats;
}

int SimpleConverter::configure(PixelFormat inputFormat,
			       PixelFormat outputFormat, const Size &size)
{
	V4L2DeviceFormat format;
	int ret;

	V4L2PixelFormat videoFormat = m2m_->output()->toV4L2PixelFormat(inputFormat);
	format.fourcc = videoFormat;
	format.size = size;

	ret = m2m_->output()->setFormat(&format);
	if (ret < 0) {
		LOG(SimplePipeline, Error)
			<< "Failed to set input format: " << strerror(-ret);
		return ret;
	}

	if (format.fourcc != videoFormat || format.size != size) {
		LOG(SimplePipeline, Error)
			<< "Input format not supported";
		return -EINVAL;
	}

	videoFormat = m2m_->capture()->toV4L2PixelFormat(outputFormat);
	format.fourcc = videoFormat;

	ret = m2m_->capture()->setFormat(&format);
	if (ret < 0) {
		LOG(SimplePipeline, Error)
			<< "Failed to set output format: " << strerror(-ret);
		return ret;
	}

	if (format.fourcc != videoFormat || format.size != size) {
		LOG(SimplePipeline, Error)
			<< "Output format not supported";
		return -EINVAL;
	}

	return 0;
}

int SimpleConverter::exportBuffers(unsigned int count,
				   std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	return m2m_->capture()->exportBuffers(count, buffers);
}

int SimpleConverter::start(unsigned int count)
{
	int ret = m2m_->output()->importBuffers(count);
	if (ret < 0)
		return ret;

	ret = m2m_->capture()->importBuffers(count);
	if (ret < 0) {
		stop();
		return ret;
	}

	ret = m2m_->output()->streamOn();
	if (ret < 0) {
		stop();
		return ret;
	}

	ret = m2m_->capture()->streamOn();
	if (ret < 0) {
		stop();
		return ret;
	}

	return 0;
}

void SimpleConverter::stop()
{
	m2m_->capture()->streamOff();
	m2m_->output()->streamOff();
	m2m_->capture()->releaseBuffers();
	m2m_->output()->releaseBuffers();
}

int SimpleConverter::queueBuffers(FrameBuffer *input, FrameBuffer *output)
{
	int ret = m2m_->output()->queueBuffer(input);
	if (ret < 0)
		return ret;

	ret = m2m_->capture()->queueBuffer(output);
	if (ret < 0)
		return ret;

	return 0;
}

void SimpleConverter::captureBufferReady(FrameBuffer *buffer)
{
	if (!outputDoneQueue_.empty()) {
		FrameBuffer *other = outputDoneQueue_.front();
		outputDoneQueue_.pop();
		bufferReady.emit(other, buffer);
	} else {
		captureDoneQueue_.push(buffer);
	}
}

void SimpleConverter::outputBufferReady(FrameBuffer *buffer)
{
	if (!captureDoneQueue_.empty()) {
		FrameBuffer *other = captureDoneQueue_.front();
		captureDoneQueue_.pop();
		bufferReady.emit(buffer, other);
	} else {
		outputDoneQueue_.push(buffer);
	}
}

} /* namespace libcamera */
