#include "AzureRecorder.h"

Recorder4Azure::Recorder4Azure()
	:dropping(std::pair<bool, int64_t>(false, -1)),
	writeThread(0),
	logFile(0),
	numFrames(0),
	logToMemory(false),
	compressed(true)
{
	writing.assignValue(false);
	k4aInterface = new K4AInterface();
	width = k4aInterface->calibration.depth_camera_calibration.resolution_width;
	height = k4aInterface->calibration.depth_camera_calibration.resolution_height;
	depth_compress_buf_size = width * height * sizeof(int16_t) * 4;
	depth_compress_buf = (uint8_t*)malloc(depth_compress_buf_size);
	encodedImage = 0;
}


Recorder4Azure::~Recorder4Azure()
{
	free(depth_compress_buf);

	if (encodedImage != 0)
	{
		cvReleaseMat(&encodedImage);
	}
	delete k4aInterface;
}

void Recorder4Azure::encodeJpeg(cv::Vec<unsigned char, 4> * rgb_data)
{
	cv::Mat4b rgbA(height, width, rgb_data, width * 4);
	cv::Mat3b rgb;
	cv::cvtColor(rgbA, rgb, cv::COLOR_BGRA2RGB);

	IplImage img = cvIplImage(rgb);

	int jpeg_params[] = { CV_IMWRITE_JPEG_QUALITY, 90, 0 };

	if (encodedImage != 0)
	{
		cvReleaseMat(&encodedImage);
	}

	encodedImage = cvEncodeImage(".jpg", &img, jpeg_params);
}

void Recorder4Azure::startWriting()
{
	assert(!writeThread && !writing.getValue() && !logFile);

	writing.assignValue(true);
	numFrames = 0;

	if (logToMemory)
	{
		memoryBuffer.clear();
		memoryBuffer.addData((unsigned char *)&numFrames, sizeof(int32_t));
	}
	else
	{
		logFile = fopen("scanning.klg", "wb+");
		fwrite(&numFrames, sizeof(int32_t), 1, logFile); //"the number of frames" is modified it later;
	}

	//open a thread to write;
	writeThread = new boost::thread(boost::bind(&Recorder4Azure::loggingThread,
		this));
}

void Recorder4Azure::stopWriting()
{
	assert(writeThread && writing.getValue());

	writing.assignValue(false);

	writeThread->join();

	dropping.assignValue(std::pair<bool, int64_t>(false, -1));

	if (logToMemory)
	{
		//leave to be addressed later
		QWidget* parent;
		memoryBuffer.writeOutAndClear("down_load.klg", numFrames, parent);
	}
	else
	{
		fseek(logFile, 0, SEEK_SET);
		fwrite(&numFrames, sizeof(int32_t), 1, logFile);

		fflush(logFile);
		fclose(logFile);
	}

	//set to initial parameter
	writeThread = 0;
	logFile = 0;
	numFrames = 0;
}

void Recorder4Azure::loggingThread()
{
	while (writing.getValueWait(1000))
	{	
		int lastDepth = k4aInterface->latestFrameIndex.getValue();
		
		if (lastDepth == -1)
		{
			continue;
		}
		int bufferIndex = lastDepth % K4AInterface::numBuffers;
		if (bufferIndex == lastWritten)
		{
			continue;
		}

		unsigned char * rgbData = 0;
		unsigned char * depthData = 0;
		unsigned long depthSize = depth_compress_buf_size;
		int32_t rgbSize = 0;

		if (compressed)
		{
			boost::thread_group threads;
			std::cout << "depthSize: " << depthSize << std::endl;
			threads.add_thread(new boost::thread(compress2,
				depth_compress_buf,
				&depthSize,
				(const Bytef*)k4aInterface->frameBuffers[bufferIndex].first.first,
				width * height * sizeof(short),
				Z_BEST_SPEED));

			threads.add_thread(new boost::thread(boost::bind(&Recorder4Azure::encodeJpeg,
				this,
				(cv::Vec<unsigned char, 4> *)k4aInterface->frameBuffers[bufferIndex].first.second)));

			threads.join_all();

			rgbSize = encodedImage->width;
			std::cout << "depthSize: " << depthSize << std::endl;
			std::cout << "rgbSize: " << rgbSize << std::endl;
			
			depthData = (unsigned char *)depth_compress_buf;
			rgbData = (unsigned char *)encodedImage->data.ptr;
		}
		else
		{
			depthSize = width * height * sizeof(short);
			rgbSize = width * height * sizeof(unsigned char) * 3;

			depthData = (unsigned char *)k4aInterface->frameBuffers[bufferIndex].first.first;
			rgbData = (unsigned char *)k4aInterface->frameBuffers[bufferIndex].first.second;
		}

		if (logToMemory)
		{
			/*memoryBuffer.addData((unsigned char *)&k4aInterface->frameBuffers[bufferIndex].second, sizeof(int64_t));
			memoryBuffer.addData((unsigned char *)&depthSize, sizeof(int32_t));
			memoryBuffer.addData((unsigned char *)&rgbSize, sizeof(int32_t));
			memoryBuffer.addData(depthData, depthSize);
			memoryBuffer.addData(rgbData, rgbSize);*/
		}
		else
		{
			logData((int64_t *)&k4aInterface->frameBuffers[bufferIndex].second,
				(int32_t *)&depthSize,
				&rgbSize,
				depthData,
				rgbData);
		}

		numFrames++;
		lastWritten = bufferIndex;
	}
}

void Recorder4Azure::logData(int64_t * timestamp,
	int32_t * depthSize,
	int32_t * imageSize,
	unsigned char * depthData,
	unsigned char * rgbData)
{
	fwrite(timestamp, sizeof(int64_t), 1, logFile);
	fwrite(depthSize, sizeof(int32_t), 1, logFile);
	fwrite(imageSize, sizeof(int32_t), 1, logFile);
	fwrite(depthData, *depthSize, 1, logFile);
	fwrite(rgbData, *imageSize, 1, logFile);
}