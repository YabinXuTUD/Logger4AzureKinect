#include "AzureRecorder.h"

Recorder4Azure::Recorder4Azure()
	:width(width),
	height(height),
	fps(fps)
{
	k4aInterface = new K4AInterface();
}


Recorder4Azure::~Recorder4Azure()
{
	delete k4aInterface;
}


void Recorder4Azure::startWriting(std::string filename)
{

	//open a thread to write the file;
	/*writeThread = new boost::thread(boost::bind(&Recorder4Azure::loggingThread,
		this));*/
}

void Recorder4Azure::stopWriting()
{
	
}

void Recorder4Azure::loggingThread()
{


}

