//#include <libcamera/camera.h>
#include <libcamera/controls.h>

#include "libcamera/internal/camera_sensor.h"

namespace libcamera {

class Agc
{
public:
	ControlList ctrls_;

	int exposure_min_, exposure_max_;
	int again_min_, again_max_;
	int again_, exposure_;
	int ignore_updates_;
	float bright_ratios_[2];
	float too_bright_ratios_[2];

	Agc(std::unique_ptr<CameraSensor> & sensor) : ignore_updates_(0)
	{
		init(sensor);
	}

	void init(std::unique_ptr<CameraSensor> & sensor);

	void get_exposure();
	void set_exposure(std::unique_ptr<CameraSensor> & sensor);

	void update_exposure(double ev_adjustment);

	void process(std::unique_ptr<CameraSensor> & sensor,
		     float bright_ratio, float too_bright_ratio);
};

} /* namespace libcamera */
