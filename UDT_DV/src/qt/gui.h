#pragma once

#include <QtGui/QMainWindow>
#include <QElapsedTimer>
#include <QPlainTextEdit>
#include "qt_gen/gui_gen.hpp"
#include "demo_player.h"

class Gui : public QMainWindow
{
	Q_OBJECT

public:
	static int UdtProgressCallback(float progress);
	static bool LogMessage(std::string message);

	Gui(QWidget *parent = 0, Qt::WFlags flags = 0);

	void loadDemo(QString filepath);
	void loadIconData();

	void connectUiElements();

	~Gui();

	QString dataPath;

protected:
	void dragEnterEvent(QDragEnterEvent* event);
	void dropEvent(QDropEvent* event);

private:
	Ui::MyClassClass ui;
	DemoPlayer demoPlayer;
	bool paused;

	void _parseDemo();
	void onProgress(float progress);
	QElapsedTimer progressTimer;

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
	bool getScalingData( QString scalingPath, int* origin, int* end );
	void demoFinished();

};
