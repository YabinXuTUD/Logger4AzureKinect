
#include <stdint.h>
#include <iostream>

#include <k4a/k4a.h>

#include <QMessageBox>
#include <QWidget>

//#include "MemoryBuffer.h"
#include <string>
#include "K4AInterface.h"

//#include <boost/thread.hpp>

//save file here

class Recorder4Azure
{
public:
	Recorder4Azure();
		virtual ~Recorder4Azure();


	void startWriting(std::string filename);
	void stopWriting();

	void loggingThread();

	K4AInterface * getK4AInterface()
	{
		return k4aInterface;
	}


private:

	K4AInterface* k4aInterface;

	//open a thread to write
	//boost::thread * writeThread;

	int depth_compress_buf_size;

	int width;
	int height;
	int fps;
};
