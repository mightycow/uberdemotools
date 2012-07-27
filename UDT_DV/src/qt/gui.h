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
	static int	UdtProgressCallback(float progress);
	static void UdtMessageCallback(int logLevel, const char* message);

	Gui();
	~Gui();

	void	ProcessCommandLine(int argumentCount, char** arguments);
	void	LoadDemo(const QString& filepath);
	void	LoadIconData();
	void	ConnectUiElements();

public slots:
	void	PlayButtonPressed();	
	void	StopButtonPressed();
	void	UpdateProgressSlider(float p);
	void	ProgressSliderValueChanged(int);
	void	TimeScaleChanged(double v);
	void	ShowClockChanged(int);
	void	ShowScoreChanged(int);
	void	ShowHudChanged(int);
	void	ShowPUChanged(int);
	void	ReverseTimeChanged(int);
	bool	GetScalingData(const QString& scalingPath, int* origin, int* end);
	void	DemoFinished();
	void	OnLoadDemoClicked();
	void	OnQuitClicked();
	void	OnAboutClicked();
	void	OnLogWindowClicked();
	void	OnLogClearClicked();
	void	OnLogSaveClicked();

protected:
	// Qt overrides.
	void	dragEnterEvent(QDragEnterEvent* event);
	void	dropEvent(QDropEvent* event);

	void	OnProgress(float progress);
	void	OnMessage(int logLevel, const char* message);

public:
	QString DataPath;

private:
	Ui::MyClassClass _ui;
	DemoPlayer _demoPlayer;
	bool _paused;
	QElapsedTimer _progressTimer;
	int _argumentCount;
	char** _arguments;
};
