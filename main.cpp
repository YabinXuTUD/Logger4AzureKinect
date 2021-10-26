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

	this->setMaximumSize(width * 2 + 150, height + 300);
	this->setMinimumSize(width * 2 + 150, height + 300);

	int fontsize = 20;
	int startpose = 120;
	int interver = 80;
	int bottonsizeWidth = 350;
	int bottonsizeHeight = 200;
	startStop = new QPushButton("Start/Stop Recording", this);
	startStop->setGeometry(startpose, 600, bottonsizeWidth, bottonsizeHeight);
	startStop->setFont(QFont("Arial", fontsize));
	connect(startStop, SIGNAL(clicked()), this, SLOT(recordToggle()));

	CapureOneFrame = new QPushButton("PerFrameCapture", this);
	CapureOneFrame->setGeometry(startpose + bottonsizeWidth + interver, 600, bottonsizeWidth,bottonsizeHeight);
	CapureOneFrame->setFont(QFont("Arial", fontsize));
	connect(CapureOneFrame, SIGNAL(clicked()), this, SLOT(oneFrameCaputre()));

	startStopDevice = new QPushButton("Start/Stop Streaming", this);
	startStopDevice->setGeometry(startpose + 2 * bottonsizeWidth + 2 * interver, 600, bottonsizeWidth, bottonsizeHeight);
	startStopDevice->setFont(QFont("Arial", fontsize));
	connect(startStopDevice, SIGNAL(clicked()), this, SLOT(controlCameraToggle()));

	//memoryRecord = new QCheckBox("Record to RAM");
	//memoryRecord->setChecked(false);
	//memoryRecord->setGeometry(0, 800, 40, 40);

	imageLabel = new QLabel(this);
	imageLabel->setGeometry(50, 0, width, height);
	imageLabel->setPixmap(QPixmap::fromImage(rgbImage));

	depthLabel = new QLabel(this);
	depthLabel->setGeometry(width + 100, 0, width, height);
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
	
	if (!recording)
	{
		//start writing file;
		std::cout << "start scanning" << std::endl;
		recorder->startWriting();
		recording = true;
	}
	else
	{
		std::cout << "writing to file" << std::endl;
		////stop writing file;
		recorder->stopWriting();
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

	unsigned short* depthBuffer = new unsigned short[width * height * 2];

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

	delete depthBuffer;
}

void MainWindow::oneFrameCaputre()
{
	//save a image to hard disk;


}
