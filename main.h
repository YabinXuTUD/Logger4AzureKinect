#ifndef MAIN_H
#define MAIN_H

#endif // MAIN_H

#include <QMainWindow>
#include "ui_AzureRecorder.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QImage>
#include <QCheckBox>
#include <QPainter>
#include <QTimer>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>

#include <iostream>

#include "AzureRecorder.h"
#include <opencv2/opencv.hpp>
#include "opencv2/core.hpp"
#include "opencv2/calib3d.hpp"
#include <opencv2/imgproc/types_c.h>
#include<opencv2/highgui/highgui.hpp>
using namespace cv;

//#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/filesystem.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {class MainWindow;}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
	virtual~MainWindow();

private slots:
	void timerCallback();
	void controlCameraToggle();
	void oneFrameCaputre();
	void recordToggle();

private:
    Ui::MainWindow *ui;

	Recorder4Azure* recorder;

	QImage depthImage;
	QImage rgbImage;
	bool recording;

    QPushButton* startStop;
	QPushButton* CapureOneFrame;
	QPushButton* startStopDevice;

	QPushButton * browseButton;
	QPushButton * dateNameButton;
	QCheckBox * autoExposure;
	QCheckBox * autoWhiteBalance;
	QCheckBox * compressed;
	QCheckBox * memoryRecord;
	QLabel * logFile;

	QLabel * depthLabel;
	QLabel * imageLabel;

	cv::Mat1b tmp;

	//cv::Mat1b tmp;
	QPainter * painter;

	QLabel * memoryStatus;
	QTimer * timer;


	int64_t lastFrameTime;

	int width;
	int height;
	int fps;
	bool tcp;
	std::string logFolder;
	std::string lastFilename;
};

