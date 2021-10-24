#include <QApplication>
#include "main.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow *window = new MainWindow();



    window->show();
    return a.exec();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
	recording(false),
    ui(new Ui::MainWindow),
	recorder(new Recorder4Azure())
{
    ui->setupUi(this);

	//obtain width and height in the current configuration;
	width = recorder->getK4AInterface()->calibration.depth_camera_calibration.resolution_width;
	height = recorder->getK4AInterface()->calibration.depth_camera_calibration.resolution_height;

	depthImage = QImage(width, height, QImage::Format_RGB888);
	rgbImage = QImage(width, height, QImage::Format_RGBA8888);

	this->setMaximumSize(width * 2 + 100, height + 300);
	this->setMinimumSize(width * 2 + 100, height + 300);

	startStop = new QPushButton("Start/Stop Recording", this);
	startStop->setGeometry(80, 600, 231, 151);
	connect(startStop, SIGNAL(clicked()), this, SLOT(recordToggle()));

	CapureOneFrame = new QPushButton("PerFrameCapture", this);
	CapureOneFrame->setGeometry(360, 600, 231, 151);
	connect(CapureOneFrame, SIGNAL(clicked()), this, SLOT(oneFrameCaputre()));

	startStopDevice = new QPushButton("Start/Stop Streaming", this);
	startStopDevice->setGeometry(680, 600, 231, 151);
	connect(startStopDevice, SIGNAL(clicked()), this, SLOT(controlCameraToggle()));

	imageLabel = new QLabel(this);
	imageLabel->setGeometry(0, 0, width, height);
	imageLabel->setPixmap(QPixmap::fromImage(rgbImage));

	depthLabel = new QLabel(this);
	depthLabel->setGeometry(width + 50, 0, width, height);
	depthLabel->setPixmap(QPixmap::fromImage(depthImage));
	
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timerCallback()));
	timer->start(15);

	//call destructor when closing the MainWindow
	setAttribute(Qt::WA_DeleteOnClose);
	//buttonLayout->addWidget(startStop);
}

MainWindow::~MainWindow()
{
    delete ui;
	timer->stop();
	delete recorder;
}

void MainWindow::recordToggle()
{
	std::cout << "start scanning" << std::endl;
	if (!recording)
	{
		QMessageBox::information(this, "Information", "stop recording, writing to file");
		recording = true;
	}
	else
	{
		QMessageBox::information(this, "Information", "start recording");
		recording = false;
	}
}

void MainWindow::controlCameraToggle()
{
	//close camera;
	recorder->getK4AInterface()->stopCamera();
}

//streaming
void MainWindow::timerCallback()
{
	//visualize current capture;
	//recorder->getK4AInterface()->captureOneFrame();

	recorder->getK4AInterface()->captureOneFrame();

	int lastDepth = recorder->getK4AInterface()->latestFrameIndex.getValue();

	if (lastDepth == -1)
	{
		return;
	}

	int bufferIndex = lastDepth % K4AInterface::numBuffers;

	if (lastFrameTime == recorder->getK4AInterface()->frameBuffers[bufferIndex].second)
	{
		return;
	}

	memcpy(&depthBuffer[0], recorder->getK4AInterface()->frameBuffers[bufferIndex].first.first, width * height * 2);
	memcpy(rgbImage.bits(), recorder->getK4AInterface()->frameBuffers[bufferIndex].first.second, width * height * 4);

	cv::Mat1w depth(height, width, (unsigned short *)&depthBuffer[0]);
	normalize(depth, tmp, 0, 255, cv::NORM_MINMAX, 0);

	cv::Mat3b depthImg(height, width, (cv::Vec<unsigned char, 3> *)depthImage.bits());
	cv::cvtColor(tmp, depthImg, CV_GRAY2RGB);

	lastFrameTime = recorder->getK4AInterface()->frameBuffers[bufferIndex].second;

	depthLabel->setPixmap(QPixmap::fromImage(depthImage));
	imageLabel->setPixmap(QPixmap::fromImage(rgbImage));

}

void MainWindow::oneFrameCaputre()
{
	recorder->getK4AInterface()->captureOneFrame();

	int lastDepth = recorder->getK4AInterface()->latestFrameIndex.getValue();

	if (lastDepth == -1)
	{
		return;
	}

	int bufferIndex = lastDepth % K4AInterface::numBuffers;

	/*if (lastFrameTime == recorder->getK4AInterface()->frameBuffers[bufferIndex].second)
	{
		return;
	}*/

	memcpy(&depthBuffer[0], recorder->getK4AInterface()->frameBuffers[bufferIndex].first.first, width * height * 2);
	memcpy(rgbImage.bits(), recorder->getK4AInterface()->frameBuffers[bufferIndex].first.second, width * height * 4);

	cv::Mat1w depth(height, width, (unsigned short *)&depthBuffer[0]);
	normalize(depth, tmp, 0, 255, cv::NORM_MINMAX, 0);

	cv::Mat3b depthImg(height, width, (cv::Vec<unsigned char, 3> *)depthImage.bits());
	cv::cvtColor(tmp, depthImg, CV_GRAY2RGB);

	depthLabel->setPixmap(QPixmap::fromImage(depthImage));
	imageLabel->setPixmap(QPixmap::fromImage(rgbImage));
}
