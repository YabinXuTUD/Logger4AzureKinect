
#include <stdint.h>
#include <iostream>

#include <k4a/k4a.h>

#include <QMessageBox>
#include <QWidget>

#include "MemoryBuffer.h"
#include <string>
#include "K4AInterface.h"

#include "opencv2/core.hpp"
#include "opencv2/calib3d.hpp"
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc_c.h"
//#include <opencv2/imgcodecs/imgcodecs_c.h>
#include "opencv2/highgui/highgui.hpp"
#include "zlib.h"

//#include <boost/thread.hpp>

//save file here

class Recorder4Azure
{
public:
	Recorder4Azure();
		virtual ~Recorder4Azure();


	void startWriting();
	void stopWriting();

	void loggingThread();

	void logData(int64_t * timestamp,
		int32_t * depthSize,
		int32_t * imageSize,
		unsigned char * depthData,
		unsigned char * rgbData);

	K4AInterface * getK4AInterface()
	{
		return k4aInterface;
	}

	ThreadMutexObject<std::pair<bool, int64_t> > dropping;
private:

	K4AInterface* k4aInterface;
	MemoryBuffer memoryBuffer;

	//open a thread to write
	//boost::thread * writeThread;

	int depth_compress_buf_size;
	uint8_t * depth_compress_buf;
	cv::Mat encodedImage;
	std::vector<uchar> buff;

	int lastWritten;
	boost::thread * writeThread;
	ThreadMutexObject<bool> writing;

	FILE * logFile;
	int32_t numFrames;

	void encodeJpeg(cv::Vec<unsigned char, 4> * rgb_data);

	int width;
	int height;
	int fps;

	bool logToMemory;
	bool compressed;
};
