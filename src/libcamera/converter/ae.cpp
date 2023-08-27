/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022-2023 Pavel Machek
 *
 * simple.cpp - Pipeline handler for simple pipelines
 */

#include <map>
#include <string>
#include <vector>

#include <libcamera/base/log.h>

#include <libcamera/camera.h>
#include <libcamera/control_ids.h>

#include "libcamera/internal/camera.h"

#include "libcamera/internal/converter/ae.h"

using libcamera::utils::Duration;

namespace libcamera {

LOG_DEFINE_CATEGORY(SimpleAgc)

void Agc::init(std::unique_ptr<CameraSensor> & sensor)
{
	ctrls_ = sensor->getControls(
		 { V4L2_CID_EXPOSURE, V4L2_CID_ANALOGUE_GAIN } );

	if (!ctrls_.contains(V4L2_CID_EXPOSURE)) {
		LOG(SimpleAgc, Error) << "Don't have exposure control";
	}
	if (!ctrls_.contains(V4L2_CID_ANALOGUE_GAIN)) {
		LOG(SimpleAgc, Error) << "Don't have gain control";
	}

	const ControlInfoMap &infoMap = *ctrls_.infoMap();
	const ControlInfo &exposure_info = infoMap.find(V4L2_CID_EXPOSURE)->second;
	const ControlInfo &gain_info = infoMap.find(V4L2_CID_ANALOGUE_GAIN)->second;

	exposure_min_ = exposure_info.min().get<int>();
	if (!exposure_min_) {
		LOG(SimpleAgc, Error) << "Minimum exposure is zero, that can't be linear";
		exposure_min_ = 1;
	}
	exposure_max_ = exposure_info.max().get<int>();
	again_min_ = gain_info.min().get<int>();
	if (!again_min_) {
		LOG(SimpleAgc, Error) << "Minimum gain is zero, that can't be linear";
		again_min_ = 100;
	}
	again_max_ = gain_info.max().get<int>();

	LOG(SimpleAgc, Info) << "Exposure" << exposure_min_ << " " << exposure_max_
			     << ", gain" << again_min_ << " " << again_max_;
}

void Agc::get_exposure()
{
	exposure_ = ctrls_.get(V4L2_CID_EXPOSURE).get<int>();
	again_ = ctrls_.get(V4L2_CID_ANALOGUE_GAIN).get<int>();

	LOG(SimpleAgc, Debug) << "Got: exposure = " << exposure_
			      << ", aGain = " << again_;
}

void Agc::set_exposure(std::unique_ptr<CameraSensor> & sensor)
{
	ctrls_.set(V4L2_CID_EXPOSURE, exposure_);
	ctrls_.set(V4L2_CID_ANALOGUE_GAIN, again_);
	sensor->setControls(&ctrls_);

	LOG(SimpleAgc, Debug) << "Set: exposure = " << exposure_
			      << ", aGain = " << again_;
}

void Agc::update_exposure(double ev_adjustment)
{
	double exp = (double)exposure_;
	double gain = (double)again_;
	double ev = ev_adjustment * exp * gain;

	/*
	 * Try to use the minimal possible analogue gain.
	 * The exposure can be any value from exposure_min_ to exposure_max_,
	 * and normally this should keep the frame rate intact.
	 */

	exp = ev / again_min_;
	if (exp > exposure_max_) exposure_ = exposure_max_;
	else if (exp < exposure_min_) exposure_ = exposure_min_;
	else exposure_ = (int)exp;

	gain = ev / exposure_;
	if (gain > again_max_) again_ = again_max_;
	else if (gain < again_min_) again_ = again_min_;
	else again_ = (int)gain;

	LOG(SimpleAgc, Debug) << "Desired EV = " << ev
			      << ", real EV = " << (double)again_ * exposure_;
}

void Agc::process(std::unique_ptr<CameraSensor> & sensor,
		  float bright_ratio, float too_bright_ratio)
{
	double ev_adjustment = 0.0;

	/*
	 * Use 2 frames delay to make sure that the exposure and the gain set
	 * have applied to the camera sensor
	 */
	if (ignore_updates_ > 0) {
		LOG(SimpleAgc, Debug) << "Skipping exposure update: " << ignore_updates_;
		--ignore_updates_;
		return;
	}

	if (bright_ratio < 0.01) ev_adjustment = 1.1;
	if (too_bright_ratio > 0.04) ev_adjustment = 0.9;

	if (ev_adjustment != 0.0) {
		get_exposure();
		update_exposure(ev_adjustment);
		set_exposure(sensor);
		ignore_updates_ = 2;
	}
}

} /* namespace libcamera */
