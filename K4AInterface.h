#pragma once

#include <k4a/k4a.h>
#include <k4arecord/record.h>
#include <iostream>
#include <vector>
#include <k4arecord/playback.h>

#include "opencv2/core.hpp"
#include "opencv2/calib3d.hpp"
#include "transformation_helpers.h"

#include "ThreadMutexObject.h"

using namespace cv;

//azure interface

class K4AInterface
{
public:
	K4AInterface::K4AInterface();
	virtual ~K4AInterface();

	void stopCamera();

	void captureOneFrame();


	static const int numBuffers = 100;
	int64_t lastFrameBufferTime;
	//frame buffer
	ThreadMutexObject<int> latestFrameIndex;
	//first is the depth buffer, the second one is the rgb buffer
	std::pair<std::pair<uint8_t *, uint8_t *>, int64_t> frameBuffers[numBuffers];

	k4a_calibration_t calibration;
	k4a_transformation_t transformation;
private:
	k4a_device_t device;
	k4a_device_configuration_t config;

	int depth_width;
	int depth_height;

	int color_width;
	int color_height;

	bool initSuccessful;

	int tx;
};

