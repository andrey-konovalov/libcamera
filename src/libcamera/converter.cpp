/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright 2022 NXP
 *
 * converter.cpp - Generic format converter interface
 */

#include "libcamera/internal/converter.h"

#include <algorithm>

#include <libcamera/base/log.h>

#include "libcamera/internal/media_device.h"

#include "linux/media.h"

/**
 * \file internal/converter.h
 * \brief Abstract converter
 */

namespace libcamera {

LOG_DEFINE_CATEGORY(Converter)

/**
 * \class Converter
 * \brief Abstract Base Class for converter
 *
 * The Converter class is an Abstract Base Class defining the interfaces of
 * converter implementations.
 *
 * Converters offer scaling and pixel format conversion services on an input
 * stream. The converter can output multiple streams with individual conversion
 * parameters from the same input stream.
 */

/**
 * \brief Construct a Converter instance
 */
Converter::Converter()
{
}

Converter::~Converter()
{
}

/**
 * \fn Converter::loadConfiguration()
 * \brief Load converter configuration from file
 * \param[in] filename The file name path
 *
 * Load converter dependent configuration parameters to apply on the hardware.
 *
 * \return 0 on success or a negative error code otherwise
 */

/**
 * \fn Converter::isValid()
 * \brief Check if the converter configuration is valid
 * \return True is the converter is valid, false otherwise
 */

/**
 * \fn Converter::formats()
 * \brief Retrieve the list of supported pixel formats for an input pixel format
 * \param[in] input Input pixel format to retrieve output pixel format list for
 * \return The list of supported output pixel formats
 */

/**
 * \fn Converter::sizes()
 * \brief Retrieve the range of minimum and maximum output sizes for an input size
 * \param[in] input Input stream size to retrieve range for
 * \return A range of output image sizes
 */

/**
 * \fn Converter::strideAndFrameSize()
 * \brief Retrieve the output stride and frame size for an input configutation
 * \param[in] pixelFormat Input stream pixel format
 * \param[in] size Input stream size
 * \return A tuple indicating the stride and frame size or an empty tuple on error
 */

/**
 * \fn Converter::configure()
 * \brief Configure a set of output stream conversion from an input stream
 * \param[in] inputCfg Input stream configuration
 * \param[out] outputCfgs A list of output stream configurations
 * \return 0 on success or a negative error code otherwise
 */

/**
 * \fn Converter::exportBuffers()
 * \brief Export buffers from the converter device
 * \param[in] output Output stream index exporting the buffers
 * \param[in] count Number of buffers to allocate
 * \param[out] buffers Vector to store allocated buffers
 *
 * This function operates similarly to V4L2VideoDevice::exportBuffers() on the
 * output stream indicated by the \a output index.
 *
 * \return The number of allocated buffers on success or a negative error code
 * otherwise
 */

/**
 * \fn Converter::start()
 * \brief Start the converter streaming operation
 * \return 0 on success or a negative error code otherwise
 */

/**
 * \fn Converter::stop()
 * \brief Stop the converter streaming operation
 */

/**
 * \fn Converter::queueBuffers()
 * \brief Queue buffers to converter device
 * \param[in] input The frame buffer to apply the conversion
 * \param[out] outputs The container holding the output stream indexes and
 * their respective frame buffer outputs.
 *
 * This function queues the \a input frame buffer on the output streams of the
 * \a outputs map key and retrieve the output frame buffer indicated by the
 * buffer map value.
 *
 * \return 0 on success or a negative error code otherwise
 */

/**
 * \var Converter::inputBufferReady
 * \brief A signal emitted when the input frame buffer completes
 */

/**
 * \var Converter::outputBufferReady
 * \brief A signal emitted on each frame buffer completion of the output queue
 */

} /* namespace libcamera */
