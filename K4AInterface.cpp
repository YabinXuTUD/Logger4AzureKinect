#include "K4AInterface.h"

const int32_t TIMEOUT_IN_MS = 1000;

static k4a_image_t transform_color_image(k4a_transformation_t transformation_handle,
	const k4a_image_t depth_image,
	const k4a_image_t color_image
	)
{
	//get the size of depth image
	int depth_image_width_pixels = k4a_image_get_width_pixels(depth_image);
	int depth_image_height_pixels = k4a_image_get_height_pixels(depth_image);

	//create transformed color image
	k4a_image_t transformed_color_image = NULL;
	if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
		depth_image_width_pixels,
		depth_image_height_pixels,
		depth_image_width_pixels * 4 * (int)sizeof(uint8_t),
		&transformed_color_image))
	{
		printf("Failed to create transformed color image\n");
		return false;
	}

	if (K4A_RESULT_SUCCEEDED != k4a_transformation_color_image_to_depth_camera(transformation_handle,
		depth_image,
		color_image,
		transformed_color_image))
	{
		printf("Failed to compute transformed color image\n");
		return false;
	}

	return transformed_color_image;
}

static k4a_image_t transform_depth_image(k4a_transformation_t transformation_handle,
	const k4a_image_t depth_image,
	const k4a_image_t color_image
	)
{
	// transform color image into depth camera geometry
	int color_image_width_pixels = k4a_image_get_width_pixels(color_image);
	int color_image_height_pixels = k4a_image_get_height_pixels(color_image);

	//create transformed depth image
	k4a_image_t transformed_depth_image = NULL;
	if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
		color_image_width_pixels,
		color_image_height_pixels,
		color_image_width_pixels * (int)sizeof(uint16_t),
		&transformed_depth_image))
	{
		printf("Failed to create transformed depth image\n");
		return false;
	}

	if (K4A_RESULT_SUCCEEDED !=
		k4a_transformation_depth_image_to_color_camera(transformation_handle, depth_image, transformed_depth_image))
	{
		printf("Failed to compute transformed depth image\n");
		return false;
	}

	return transformed_depth_image;
}

K4AInterface::K4AInterface():
	initSuccessful(true)
{
	//setup device;
	uint32_t device_count = 0;
	config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

	device_count = k4a_device_get_installed_count();
	std::cout << "find " << device_count << " devices\n";
	if (device_count == 0)
	{
		printf("No K4A devices found");
		initSuccessful = false;
	}

	//open device;
	if (K4A_RESULT_SUCCEEDED != k4a_device_open(K4A_DEVICE_DEFAULT, &device))
	{
		printf("Failed to open device\n");
		initSuccessful = false;
	}
	
	//setup all parameters of the device;
	config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
	config.color_resolution = K4A_COLOR_RESOLUTION_720P;
	config.synchronized_images_only = true;
	config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
	config.camera_fps = K4A_FRAMES_PER_SECOND_30;
	config.wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
	
	//start device with all configuration
	k4a_device_start_cameras(device, &config);

	//get calibration of the camera in current configration;
	if (K4A_RESULT_SUCCEEDED !=
		k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration))
	{
		printf("Failed to get calibration\n");
		initSuccessful = false;
	}

	//transformation at current calibration;
	transformation = k4a_transformation_create(&calibration);
	
	//initialize frame buffers(color map will be transfomed to depth map);
	latestFrameIndex.assignValue(-1);
	for (int i = 0; i < numBuffers; i++)
	{
		uint8_t * newDepth = (uint8_t *)calloc(calibration.color_camera_calibration.resolution_width * calibration.color_camera_calibration.resolution_height * 2, sizeof(uint8_t));
		uint8_t * newImage = (uint8_t *)calloc(calibration.color_camera_calibration.resolution_width * calibration.color_camera_calibration.resolution_height * 4, sizeof(uint8_t));
		frameBuffers[i] = std::pair<std::pair<uint8_t *, uint8_t *>, int64_t>(std::pair<uint8_t *, uint8_t *>(newDepth, newImage), 0);
	}

	auto calib = calibration.color_camera_calibration;
	using namespace std;
	cout << "resolution width: " << calib.resolution_width << endl;
	cout << "resolution height: " << calib.resolution_height << endl;
	cout << "principal point x: " << calib.intrinsics.parameters.param.cx << endl;
	cout << "principal point y: " << calib.intrinsics.parameters.param.cy << endl;
	cout << "focal length x: " << calib.intrinsics.parameters.param.fx << endl;
	cout << "focal length y: " << calib.intrinsics.parameters.param.fy << endl;
	cout << "radial distortion coefficients:" << endl;
	cout << "k1: " << calib.intrinsics.parameters.param.k1 << endl;
	cout << "k2: " << calib.intrinsics.parameters.param.k2 << endl;
	cout << "k3: " << calib.intrinsics.parameters.param.k3 << endl;
	cout << "k4: " << calib.intrinsics.parameters.param.k4 << endl;
	cout << "k5: " << calib.intrinsics.parameters.param.k5 << endl;
	cout << "k6: " << calib.intrinsics.parameters.param.k6 << endl;
	cout << "center of distortion in Z=1 plane, x: " << calib.intrinsics.parameters.param.codx << endl;
	cout << "center of distortion in Z=1 plane, y: " << calib.intrinsics.parameters.param.cody << endl;
	cout << "tangential distortion coefficient x: " << calib.intrinsics.parameters.param.p1 << endl;
	cout << "tangential distortion coefficient y: " << calib.intrinsics.parameters.param.p2 << endl;
	cout << "metric radius: " << calib.intrinsics.parameters.param.metric_radius << endl;
}

K4AInterface::~K4AInterface()
{
	k4a_device_stop_cameras(device);
	k4a_device_close(device);
	
	for (int i = 0; i < numBuffers; i++)
	{
		free(frameBuffers[i].first.first);
		free(frameBuffers[i].first.second);
	}
}

void K4AInterface::captureOneFrame()
{
	// Get a capture
	k4a_image_t depth_image;
	k4a_image_t rgb_image;
	k4a_image_t transformed_depth_image;
	k4a_capture_t capture;

	switch (k4a_device_get_capture(device, &capture, TIMEOUT_IN_MS))
	{
	case K4A_WAIT_RESULT_SUCCEEDED:
		break;
	case K4A_WAIT_RESULT_TIMEOUT:
		printf("Timed out waiting for a capture\n");
	case K4A_WAIT_RESULT_FAILED:
		printf("Failed to read a capture\n");
	}
	
	// Get a depth image(remember to release it right after using it)
	depth_image = k4a_capture_get_depth_image(capture);
	if (depth_image == 0)
	{
		printf("Failed to get depth image from capture\n");
	}

	// Get a rgb image
	rgb_image = k4a_capture_get_color_image(capture);
	if (rgb_image == 0)
	{
		printf("Failed to get depth image from capture\n");
	}

	transformed_depth_image = transform_depth_image(transformation, depth_image, rgb_image);

	//record system time
	boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
	boost::posix_time::time_duration duration(time.time_of_day());
	lastFrameBufferTime = duration.total_microseconds();

	int bufferIndex = (latestFrameIndex.getValue() + 1) % numBuffers;
	memcpy(frameBuffers[bufferIndex].first.first, k4a_image_get_buffer(transformed_depth_image), k4a_image_get_width_pixels(transformed_depth_image)
			* k4a_image_get_height_pixels(transformed_depth_image) * 2);
	memcpy(frameBuffers[bufferIndex].first.second, k4a_image_get_buffer(rgb_image), k4a_image_get_width_pixels(rgb_image)
		* k4a_image_get_height_pixels(rgb_image) * 4);
	frameBuffers[bufferIndex].second = lastFrameBufferTime;

	latestFrameIndex++;

	//release image and capture;
	k4a_image_release(depth_image);
	k4a_image_release(rgb_image);
	k4a_image_release(transformed_depth_image);
	k4a_capture_release(capture);
	fflush(stdout);
}

void K4AInterface::stopCamera()
{
	k4a_device_stop_cameras(device);
	k4a_device_close(device);
}