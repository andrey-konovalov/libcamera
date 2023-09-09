/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright 2022 NXP
 *
 * converter.cpp - Generic format converter interface
 */

#include "libcamera/internal/converter_media.h"

#include <algorithm>

#include <libcamera/base/log.h>

#include "libcamera/internal/media_device.h"

#include "linux/media.h"

/**
 * \file internal/converter_media.h
 * \brief Abstract media device based converter
 */

namespace libcamera {

LOG_DECLARE_CATEGORY(Converter)

/**
 * \class ConverterMD
 * \brief Abstract Base Class for media device based converter
 *
 * The ConverterMD class is an Abstract Base Class defining the interfaces of
 * media device based converter implementations.
 *
 * Converters offer scaling and pixel format conversion services on an input
 * stream. The converter can output multiple streams with individual conversion
 * parameters from the same input stream.
 */

/**
 * \brief Construct a ConverterMD instance
 * \param[in] media The media device implementing the converter
 *
 * This searches for the entity implementing the data streaming function in the
 * media graph entities and use its device node as the converter device node.
 */
ConverterMD::ConverterMD(MediaDevice *media)
{
	const std::vector<MediaEntity *> &entities = media->entities();
	auto it = std::find_if(entities.begin(), entities.end(),
			       [](MediaEntity *entity) {
				       return entity->function() == MEDIA_ENT_F_IO_V4L;
			       });
	if (it == entities.end()) {
		LOG(Converter, Error)
			<< "No entity suitable for implementing a converter in "
			<< media->driver() << " entities list.";
		return;
	}

	deviceNode_ = (*it)->deviceNode();
}

ConverterMD::~ConverterMD()
{
}

/**
 * \fn ConverterMD::deviceNode()
 * \brief The converter device node attribute accessor
 * \return The converter device node string
 */

/**
 * \class ConverterMDFactoryBase
 * \brief Base class for media device based converter factories
 *
 * The ConverterMDFactoryBase class is the base of all specializations of the
 * ConverterMDFactory class template. It implements the factory registration,
 * maintains a registry of factories, and provides access to the registered
 * factories.
 */

/**
 * \brief Construct a media device based converter factory base
 * \param[in] name Name of the converter class
 * \param[in] compatibles Name aliases of the converter class
 *
 * Creating an instance of the factory base registers it with the global list of
 * factories, accessible through the factories() function.
 *
 * The factory \a name is used as unique identifier. If the converter
 * implementation fully relies on a generic framework, the name should be the
 * same as the framework. Otherwise, if the implementation is specialized, the
 * factory name should match the driver name implementing the function.
 *
 * The factory \a compatibles holds a list of driver names implementing a generic
 * subsystem without any personalizations.
 */
ConverterMDFactoryBase::ConverterMDFactoryBase(const std::string name, std::initializer_list<std::string> compatibles)
	: name_(name), compatibles_(compatibles)
{
	registerType(this);
}

/**
 * \fn ConverterMDFactoryBase::compatibles()
 * \return The names compatibles
 */

/**
 * \brief Create an instance of the converter corresponding to a named factory
 * \param[in] media Name of the factory
 *
 * \return A unique pointer to a new instance of the media device based
 * converter subclass corresponding to the named factory or one of its alias.
 * Otherwise a null pointer if no such factory exists.
 */
std::unique_ptr<ConverterMD> ConverterMDFactoryBase::create(MediaDevice *media)
{
	const std::vector<ConverterMDFactoryBase *> &factories =
		ConverterMDFactoryBase::factories();

	for (const ConverterMDFactoryBase *factory : factories) {
		const std::vector<std::string> &compatibles = factory->compatibles();
		auto it = std::find(compatibles.begin(), compatibles.end(), media->driver());

		if (it == compatibles.end() && media->driver() != factory->name_)
			continue;

		LOG(Converter, Debug)
			<< "Creating converter from "
			<< factory->name_ << " factory with "
			<< (it == compatibles.end() ? "no" : media->driver()) << " alias.";

		std::unique_ptr<ConverterMD> converter = factory->createInstance(media);
		if (converter->isValid())
			return converter;
	}

	return nullptr;
}

/**
 * \brief Add a media device based converter class to the registry
 * \param[in] factory Factory to use to construct the media device based
 * converter class
 *
 * The caller is responsible to guarantee the uniqueness of the converter name.
 */
void ConverterMDFactoryBase::registerType(ConverterMDFactoryBase *factory)
{
	std::vector<ConverterMDFactoryBase *> &factories =
		ConverterMDFactoryBase::factories();

	factories.push_back(factory);
}

/**
 * \brief Retrieve the list of all media device based converter factory names
 * \return The list of all media device based converter factory names
 */
std::vector<std::string> ConverterMDFactoryBase::names()
{
	std::vector<std::string> list;

	std::vector<ConverterMDFactoryBase *> &factories =
		ConverterMDFactoryBase::factories();

	for (ConverterMDFactoryBase *factory : factories) {
		list.push_back(factory->name_);
		for (auto alias : factory->compatibles())
			list.push_back(alias);
	}

	return list;
}

/**
 * \brief Retrieve the list of all media device based converter factories
 * \return The list of media device based converter factories
 */
std::vector<ConverterMDFactoryBase *> &ConverterMDFactoryBase::factories()
{
	/*
	 * The static factories map is defined inside the function to ensure
	 * it gets initialized on first use, without any dependency on link
	 * order.
	 */
	static std::vector<ConverterMDFactoryBase *> factories;
	return factories;
}

/**
 * \var ConverterMDFactoryBase::name_
 * \brief The name of the factory
 */

/**
 * \var ConverterMDFactoryBase::compatibles_
 * \brief The list holding the factory compatibles
 */

/**
 * \class ConverterMDFactory
 * \brief Registration of ConverterMDFactory classes and creation of instances
 * \param _Converter The converter class type for this factory
 *
 * To facilitate discovery and instantiation of ConverterMD classes, the
 * ConverterMDFactory class implements auto-registration of converter helpers.
 * Each ConverterMD subclass shall register itself using the
 * REGISTER_CONVERTER() macro, which will create a corresponding instance of a
 * ConverterMDFactory subclass and register it with the static list of
 * factories.
 */

/**
 * \fn ConverterMDFactory::ConverterMDFactory(const char *name, std::initializer_list<std::string> compatibles)
 * \brief Construct a media device converter factory
 * \details \copydetails ConverterMDFactoryBase::ConverterMDFactoryBase
 */

/**
 * \fn ConverterMDFactory::createInstance() const
 * \brief Create an instance of the ConverterMD corresponding to the factory
 * \param[in] media Media device pointer
 * \return A unique pointer to a newly constructed instance of the ConverterMD
 * subclass corresponding to the factory
 */

/**
 * \def REGISTER_CONVERTER_MD
 * \brief Register a media device based converter with the ConverterMD factory
 * \param[in] name ConverterMD name used to register the class
 * \param[in] converter Class name of ConverterMD derived class to register
 * \param[in] compatibles List of compatible names
 *
 * Register a ConverterMD subclass with the factory and make it available to try
 * and match converters.
 */

} /* namespace libcamera */
