
#include <algorithm>
#include <array>
#include <iomanip>
#include <memory>
#include <vector>

#include <linux/media-bus-format.h>

#include <libcamera/camera.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>

#include "camera_sensor.h"
#include "device_enumerator.h"
#include "log.h"
#include "media_device.h"
#include "pipeline_handler.h"
#include "utils.h"
#include "v4l2_subdevice.h"
#include "v4l2_videodevice.h"

namespace libcamera {
LOG_DEFINE_CATEGORY(Simple)

struct SimplePipelineInfo {
	std::string driverName;
	std::string phyName;
	std::string v4l2Name;
	unsigned int v4l2PixFmt;
	unsigned int mediaBusFmt;
	unsigned int maxWidth;
	unsigned int maxHeight;
};

class SimpleCameraData : public CameraData
{
public:
	SimpleCameraData(PipelineHandler *pipe)
		: CameraData(pipe), sensor_(nullptr)
	{
	}

	~SimpleCameraData()
	{
		delete sensor_;
	}

	Stream stream_;
	CameraSensor *sensor_;
};

class SimpleCameraConfiguration : public CameraConfiguration
{
public:
	SimpleCameraConfiguration(Camera *camera, SimpleCameraData *data, const SimplePipelineInfo *pipelineInfo);

	Status validate() override;

	const V4L2SubdeviceFormat &sensorFormat() { return sensorFormat_; }

private:
	/*
         * The SimpleCameraData instance is guaranteed to be valid as long as the
         * corresponding Camera instance is valid. In order to borrow a
         * reference to the camera data, store a new reference to the camera.
         */
	std::shared_ptr<Camera> camera_;
	const SimpleCameraData *data_;

	V4L2SubdeviceFormat sensorFormat_;

	const SimplePipelineInfo *pipelineInfo_;
};

class PipelineHandlerSimple : public PipelineHandler
{
public:
	PipelineHandlerSimple(CameraManager *manager);

	~PipelineHandlerSimple();

	CameraConfiguration *generateConfiguration(Camera *camera,
						   const StreamRoles &roles) override;

	int configure(Camera *camera, CameraConfiguration *config) override;

	int exportFrameBuffers(Camera *camera, Stream *stream,
			       std::vector<std::unique_ptr<FrameBuffer>> *buffers) override;
	int importFrameBuffers(Camera *camera, Stream *stream) override;
	void freeFrameBuffers(Camera *camera, Stream *stream) override;

	int start(Camera *camera) override;

	void stop(Camera *camera) override;

	int queueRequestDevice(Camera *camera, Request *request) override;

	bool match(DeviceEnumerator *enumerator) override;

private:
	SimpleCameraData *cameraData(const Camera *camera)
	{
		return static_cast<SimpleCameraData *>(
			PipelineHandler::cameraData(camera));
	}

	int initLinks();

	int createCamera(MediaEntity *sensor);

	void bufferReady(FrameBuffer *buffer);

	MediaDevice *media_;
	V4L2Subdevice *dphy_;
	V4L2VideoDevice *video_;

	Camera *activeCamera_;

	const SimplePipelineInfo *pipelineInfo_;
};

SimpleCameraConfiguration::SimpleCameraConfiguration(Camera *camera,
						     SimpleCameraData *data,
						     const SimplePipelineInfo *pipelineInfo)
	: CameraConfiguration()
{
	camera_ = camera->shared_from_this();
	data_ = data;
	pipelineInfo_ = pipelineInfo;
}

CameraConfiguration::Status SimpleCameraConfiguration::validate()
{
	const CameraSensor *sensor = data_->sensor_;
	Status status = Valid;

	if (config_.empty())
		return Invalid;

	/* Cap the number of entries to the available streams. */
	if (config_.size() > 1) {
		config_.resize(1);
		status = Adjusted;
	}

	StreamConfiguration &cfg = config_[0];

	/* Adjust the pixel format. */
	if (cfg.pixelFormat != pipelineInfo_->v4l2PixFmt) {
		LOG(Simple, Debug) << "Adjusting pixel format";
		cfg.pixelFormat = pipelineInfo_->v4l2PixFmt;
		status = Adjusted;
	}

	/* Select the sensor format. */
	sensorFormat_ = sensor->getFormat({ pipelineInfo_->mediaBusFmt },
					  cfg.size);
	if (!sensorFormat_.size.width || !sensorFormat_.size.height)
		sensorFormat_.size = sensor->resolution();

	/*
         * Provide a suitable default that matches the sensor aspect
         * ratio and clamp the size to the hardware bounds.
         *
         * \todo: Check the hardware alignment constraints.
         */
	const Size size = cfg.size;

	unsigned int pipelineMaxWidth = std::min(sensorFormat_.size.width, pipelineInfo_->maxWidth);
	unsigned int pipelineMaxHeight = std::min(sensorFormat_.size.height, pipelineInfo_->maxHeight);

	if (!cfg.size.width || !cfg.size.height) {
		cfg.size.width = pipelineMaxWidth;
		cfg.size.height = pipelineMaxWidth * sensorFormat_.size.height / sensorFormat_.size.width;
	}

	cfg.size.width = std::min(pipelineMaxWidth, cfg.size.width);
	cfg.size.height = std::min(pipelineMaxHeight, cfg.size.height);

	cfg.size.width = std::max(32U, std::min(4416U, cfg.size.width));
	cfg.size.height = std::max(16U, std::min(3312U, cfg.size.height));

	if (cfg.size != size) {
		LOG(Simple, Debug)
			<< "Adjusting size from " << size.toString()
			<< " to " << cfg.size.toString();
		status = Adjusted;
	}

	cfg.bufferCount = 3;

	return status;
}

PipelineHandlerSimple::PipelineHandlerSimple(CameraManager *manager)
	: PipelineHandler(manager), dphy_(nullptr), video_(nullptr)
{
}

PipelineHandlerSimple::~PipelineHandlerSimple()
{
	delete video_;
	delete dphy_;
}

/* -----------------------------------------------------------------------------
 * Pipeline Operations
 */

CameraConfiguration *PipelineHandlerSimple::generateConfiguration(Camera *camera,
								  const StreamRoles &roles)
{
	SimpleCameraData *data = cameraData(camera);
	CameraConfiguration *config = new SimpleCameraConfiguration(camera, data, PipelineHandlerSimple::pipelineInfo_);

	if (roles.empty())
		return config;

	StreamConfiguration cfg{};
	cfg.pixelFormat = pipelineInfo_->v4l2PixFmt;
	cfg.size = data->sensor_->resolution();

	config->addConfiguration(cfg);

	config->validate();

	return config;
}

int PipelineHandlerSimple::configure(Camera *camera, CameraConfiguration *c)
{
	SimpleCameraConfiguration *config =
		static_cast<SimpleCameraConfiguration *>(c);
	SimpleCameraData *data = cameraData(camera);
	StreamConfiguration &cfg = config->at(0);
	CameraSensor *sensor = data->sensor_;
	int ret;

	/*
         * Configure the sensor links: enable the link corresponding to this
         * camera and disable all the other sensor links.
         */
	const MediaPad *pad = dphy_->entity()->getPadByIndex(0);

	for (MediaLink *link : pad->links()) {
		bool enable = link->source()->entity() == sensor->entity();

		if (!!(link->flags() & MEDIA_LNK_FL_ENABLED) == enable)
			continue;

		LOG(Simple, Debug)
			<< (enable ? "Enabling" : "Disabling")
			<< " link from sensor '"
			<< link->source()->entity()->name()
			<< "' to CSI-2 receiver";

		ret = link->setEnabled(enable);
		if (ret < 0)
			return ret;
	}

	/*
         * Configure the format on the sensor output and propagate it through
         * the pipeline.
         */
	V4L2SubdeviceFormat format = config->sensorFormat();
	LOG(Simple, Debug) << "Configuring sensor with " << format.toString();

	ret = sensor->setFormat(&format);
	if (ret < 0)
		return ret;

	LOG(Simple, Debug) << "Sensor configured with " << format.toString();

	V4L2DeviceFormat outputFormat = {};
	outputFormat.fourcc = cfg.pixelFormat;
	outputFormat.size = cfg.size;
	outputFormat.planesCount = 2;

	ret = video_->setFormat(&outputFormat);
	if (ret)
		return ret;

	if (outputFormat.size != cfg.size ||
	    outputFormat.fourcc != cfg.pixelFormat) {
		LOG(Simple, Error)
			<< "Unable to configure capture in " << cfg.toString();
		return -EINVAL;
	}

	cfg.setStream(&data->stream_);

	return 0;
}

int PipelineHandlerSimple::exportFrameBuffers(Camera *camera, Stream *stream,
					      std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	//SimpleCameraData *data = cameraData(camera);
	unsigned int count = stream->configuration().bufferCount;

	return video_->exportBuffers(count, buffers);
}

int PipelineHandlerSimple::importFrameBuffers(Camera *camera, Stream *stream)
{
	//SimpleCameraData *data = cameraData(camera);
	unsigned int count = stream->configuration().bufferCount;

	return video_->importBuffers(count);
}

void PipelineHandlerSimple::freeFrameBuffers(Camera *camera, Stream *stream)
{
	//SimpleCameraData *data = cameraData(camera);

	video_->releaseBuffers();
}

int PipelineHandlerSimple::start(Camera *camera)
{
	int ret;

	ret = video_->streamOn();
	if (ret)
		LOG(Simple, Error)
			<< "Failed to start camera " << camera->name();

	activeCamera_ = camera;

	return ret;
}

void PipelineHandlerSimple::stop(Camera *camera)
{
	int ret;

	ret = video_->streamOff();
	if (ret)
		LOG(Simple, Warning)
			<< "Failed to stop camera " << camera->name();

	activeCamera_ = nullptr;
}

int PipelineHandlerSimple::queueRequestDevice(Camera *camera, Request *request)
{
	SimpleCameraData *data = cameraData(camera);
	Stream *stream = &data->stream_;

	FrameBuffer *buffer = request->findBuffer(stream);
	if (!buffer) {
		LOG(Simple, Error)
			<< "Attempt to queue request with invalid stream";
		return -ENOENT;
	}

	int ret = video_->queueBuffer(buffer);
	if (ret < 0)
		return ret;

	return 0;
}

/* -----------------------------------------------------------------------------
 * Match and Setup
 */

int PipelineHandlerSimple::createCamera(MediaEntity *sensor)
{
	int ret;

	std::unique_ptr<SimpleCameraData> data =
		std::make_unique<SimpleCameraData>(this);

	data->sensor_ = new CameraSensor(sensor);
	ret = data->sensor_->init();

LOG(Simple,Debug) << sensor->name()  << "CameraSensor created";

	if (ret)
		return ret;

	std::set<Stream *> streams{ &data->stream_ };
	std::shared_ptr<Camera> camera =
		Camera::create(this, sensor->name(), streams);
	registerCamera(std::move(camera), std::move(data));

	return 0;
}

bool PipelineHandlerSimple::match(DeviceEnumerator *enumerator)
{
	const MediaPad *pad;

	static const SimplePipelineInfo infos[] = {
		{
			.driverName = "sun6i-csi",
			.phyName = "sun6i-csi",
			.v4l2Name = "sun6i-csi",
			.v4l2PixFmt = V4L2_PIX_FMT_UYVY,
			.mediaBusFmt = MEDIA_BUS_FMT_UYVY8_2X8,
			.maxWidth = 1280,
			.maxHeight = 720,
		},
		{
			.driverName = "unicam",
			.phyName = "unicam",
			.v4l2Name = "unicam",
			.v4l2PixFmt = V4L2_PIX_FMT_SRGGB10P,	/* pRAA */
			.mediaBusFmt = MEDIA_BUS_FMT_SRGGB10_1X10,
			.maxWidth = 3280,
			.maxHeight = 2464,
		},
	};

	for (unsigned int i = 0; i < ARRAY_SIZE(infos); i++) {
		const SimplePipelineInfo *ptr = &infos[i];

		DeviceMatch dm(ptr->driverName);
		dm.add(ptr->phyName);

		media_ = acquireMediaDevice(enumerator, dm);
		if (!media_)
			continue;

LOG(Simple,Debug) << ptr->driverName << "acquired";

		PipelineHandlerSimple::pipelineInfo_ = ptr;

		/* Create the V4L2 subdevices we will need. */
		dphy_ = V4L2Subdevice::fromEntityName(media_, ptr->phyName);
		if (dphy_->open() < 0)
			return false;

		/* Locate and open the capture video node. */
		video_ = V4L2VideoDevice::fromEntityName(media_, ptr->v4l2Name);
		if (video_->open() < 0)
			return false;

		video_->bufferReady.connect(this, &PipelineHandlerSimple::bufferReady);

		/*
             * Enumerate all sensors connected to the CSI-2 receiver and create one
             * camera instance for each of them.
             */
		pad = dphy_->entity()->getPadByIndex(0);
		if (!pad)
			return false;

		for (MediaLink *link : pad->links())
			createCamera(link->source()->entity());

		return true;
	}
	return false;
}

/* -----------------------------------------------------------------------------
 * Buffer Handling
 */

void PipelineHandlerSimple::bufferReady(FrameBuffer *buffer)
{
	ASSERT(activeCamera_);
	LOG(Simple, Debug) << "bufferReady";
	Request *request = buffer->request();
	completeBuffer(activeCamera_, request, buffer);
	completeRequest(activeCamera_, request);
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerSimple);

} // namespace libcamera
