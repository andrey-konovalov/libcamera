/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Laurent Pinchart
 * Copyright 2022 NXP
 * Copyright 2023, Linaro Ltd
 *
 * converter_media.h - Generic media device based format converter interface
 */

#pragma once

#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <libcamera/base/class.h>
#include <libcamera/base/signal.h>

#include <libcamera/geometry.h>

#include "libcamera/internal/converter.h"

namespace libcamera {

class FrameBuffer;
class MediaDevice;
class PixelFormat;
struct StreamConfiguration;

class ConverterMD : public Converter
{
public:
	ConverterMD(MediaDevice *media);
	~ConverterMD();

	const std::string &deviceNode() const { return deviceNode_; }

private:
	std::string deviceNode_;
};

class ConverterMDFactoryBase
{
public:
	ConverterMDFactoryBase(const std::string name, std::initializer_list<std::string> compatibles);
	virtual ~ConverterMDFactoryBase() = default;

	const std::vector<std::string> &compatibles() const { return compatibles_; }

	static std::unique_ptr<ConverterMD> create(MediaDevice *media);
	static std::vector<ConverterMDFactoryBase *> &factories();
	static std::vector<std::string> names();

private:
	LIBCAMERA_DISABLE_COPY_AND_MOVE(ConverterMDFactoryBase)

	static void registerType(ConverterMDFactoryBase *factory);

	virtual std::unique_ptr<ConverterMD> createInstance(MediaDevice *media) const = 0;

	std::string name_;
	std::vector<std::string> compatibles_;
};

template<typename _ConverterMD>
class ConverterMDFactory : public ConverterMDFactoryBase
{
public:
	ConverterMDFactory(const char *name, std::initializer_list<std::string> compatibles)
		: ConverterMDFactoryBase(name, compatibles)
	{
	}

	std::unique_ptr<ConverterMD> createInstance(MediaDevice *media) const override
	{
		return std::make_unique<_ConverterMD>(media);
	}
};

#define REGISTER_CONVERTER_MD(name, converter, compatibles) \
	static ConverterMDFactory<converter> global_##converter##MDFactory(name, compatibles);

} /* namespace libcamera */
