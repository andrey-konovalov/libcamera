/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * camera_metadata.cpp - libcamera Android Camera Metadata Helper
 */

#include "camera_metadata.h"

#include "log.h"

using namespace libcamera;

LOG_DEFINE_CATEGORY(CameraMetadata);

CameraMetadata::CameraMetadata(size_t entryCapacity, size_t dataCapacity)
{
	metadata_ = allocate_camera_metadata(entryCapacity, dataCapacity);
	valid_ = metadata_ != nullptr;
}

CameraMetadata::~CameraMetadata()
{
	if (metadata_)
		free_camera_metadata(metadata_);
}

bool CameraMetadata::addEntry(uint32_t tag, const void *data, size_t count)
{
	if (!valid_)
		return false;

	if (!add_camera_metadata_entry(metadata_, tag, data, count))
		return true;

	const char *name = get_camera_metadata_tag_name(tag);
	if (name)
		LOG(CameraMetadata, Error)
			<< "Failed to add tag " << name;
	else
		LOG(CameraMetadata, Error)
			<< "Failed to add unknown tag " << tag;

	valid_ = false;

	return false;
}

camera_metadata_t *CameraMetadata::get()
{
	return valid_ ? metadata_ : nullptr;
}
