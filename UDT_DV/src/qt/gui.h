#pragma once


#include "qt_gen/gui_gen.hpp"
#include "demo_player.h"

#include <QtGui/QMainWindow>
#include <QElapsedTimer>
#include <QPlainTextEdit>


class Gui : public QMainWindow
{
	Q_OBJECT

public:
	static int UdtProgressCallback(float progress);
	static void UdtMessageCallback(int logLevel, const char* message);

	Gui(QWidget *parent = 0, Qt::WFlags flags = 0);
	~Gui();

	void loadDemo(QString filepath);
	void loadIconData();
	void connectUiElements();

public slots:
	void playButtonPressed();	
	void stopButtonPressed();
	void updateProgressSlider(float p);
	void progressSliderValueChanged(int);
	void timeScaleChanged(double v);
	void showClockChanged(int);
	void showScoreChanged(int);
	void showHudChanged(int);
	void reverseTimeChanged(int);
	bool getScalingData(QString scalingPath, int* origin, int* end);
	void demoFinished();

public:
	QString DataPath;

protected:
	void dragEnterEvent(QDragEnterEvent* event);
	void dropEvent(QDropEvent* event);
	void onProgress(float progress);
	void onMessage(int logLevel, const char* message);

private:
	Ui::MyClassClass _ui;
	DemoPlayer _demoPlayer;
	bool _paused;
	QElapsedTimer _progressTimer;
};
