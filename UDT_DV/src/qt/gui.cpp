#include "gui.h"
#include "demo73.hpp"
#include "common.hpp"
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QMimeData>
#include <QUrl>


static const char* const defaultDataDir = "..\\data\\";
static const char* const dataSearchDirs[] =
{
	"..\\data\\",			// Deployment.
	"..\\..\\..\\data\\"	// Development.
};

static Gui* gui = NULL;
int Gui::UdtProgressCallback(float progress)
{
	gui->onProgress(progress);

	return 0;
}

void Gui::UdtMessageCallback(int logLevel, const char* message)
{
	gui->onMessage(logLevel, message);
}

static QPlainTextEdit* logWidget = NULL;
bool Gui::LogMessage(const std::string& message)
{
	if(logWidget == NULL)
	{
		return false;
	}

	logWidget->appendPlainText(QString::fromStdString(message));
	logWidget->appendPlainText("\n");

	return true;
}


Gui::Gui(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), demoPlayer(this)
{
	gui = this;
	_progressCallback = &UdtProgressCallback;
	_messageCallback = &UdtMessageCallback;

	ui.setupUi(this);
	logWidget = ui.logWidget;

	connectUiElements();
	ui.pathLineEdit->setReadOnly(true);

	dataPath = "";
	QDir dir;
	const size_t searchDirCount = sizeof(dataSearchDirs) / sizeof(dataSearchDirs[0]);
	for(size_t i = 0; i < searchDirCount; ++i)
	{
		const QString dataFolder = dataSearchDirs[i];
		if(dir.exists(dataFolder))
		{
			dataPath = dataFolder;
			break;
		}
	}
	if(dataPath.isEmpty())
	{
		dataPath = defaultDataDir;
	}
	
	paused = true;

	loadIconData();

	QString demoPath;

	// Test code
	int demo = 2;
	switch(demo)
	{
	case 0:
		{
			loadDemo(dataPath + "Memento_Mori(POV)-vs-ischju-toxicity-2012_02_16-12_19_42.dm_73");
			break;
		}
	case 1:
		{
			loadDemo(dataPath + "Memento_Mori(POV)-vs-sittodimitri-furiousheights-2012_06_24-16_26_00.dm_73");
			break;
		}
	default:
		break;
	}
}

Gui::~Gui()
{
}

void Gui::connectUiElements()
{
	connect(&demoPlayer, SIGNAL(entitiesUpdated()), ui.paintWidget, SLOT(repaint()));
	connect(&demoPlayer, SIGNAL(progress(float)), this, SLOT(updateProgressSlider(float)));
	connect(&demoPlayer, SIGNAL(demoFinished()), this, SLOT(demoFinished()));
	connect(ui.progressSlider, SIGNAL(valueChanged(int)), this, SLOT(progressSliderValueChanged(int)));
	connect(ui.playButton, SIGNAL(pressed()), this, SLOT(playButtonPressed()));
	connect(ui.stopButton, SIGNAL(pressed()), this, SLOT(stopButtonPressed()));
	connect(ui.timeScaleDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(timeScaleChanged(double)));
	connect(ui.showClockCheckBox, SIGNAL(stateChanged(int)), this, SLOT(showClockChanged(int)));
	connect(ui.showScoresCheckBox, SIGNAL(stateChanged(int)), this, SLOT(showScoreChanged(int)));
	connect(ui.showHudCheckBox, SIGNAL(stateChanged(int)), this, SLOT(showHudChanged(int)));
	connect(ui.reverseCheckBox, SIGNAL(stateChanged(int)), this, SLOT(reverseTimeChanged(int)));
}

bool Gui::getScalingData( QString scalingPath, int* origin, int* end ) 
{
	QFile file(scalingPath);
	if(file.open(QIODevice::ReadOnly))
	{
		QTextStream ts(&file);
		while(!ts.atEnd())
		{
			QString line = ts.readLine();
			QStringList tokens = line.split(" ");

			if(tokens.size() != 5)
			{
				QMessageBox msgBox;
				msgBox.setText("Unable to read " + scalingPath + " properly. Check the syntax, and remove any empty line.");
				msgBox.exec();
				return false;
			}
			if(tokens[0] == "origin")
			{
				origin[0] = tokens[2].toInt();
				origin[1] = tokens[3].toInt();
				origin[2] = tokens[4].toInt();
			}
			else if(tokens[0] == "end")
			{
				end[0] = tokens[2].toInt();
				end[1] = tokens[3].toInt();
				end[2] = tokens[4].toInt();
			}
		}
	}
	else
		return false;
	file.close();
	return true;
}

void Gui::loadDemo( QString filepath )
{
	ui.pathLineEdit->setText(filepath);
	
	// Load demo
	demoPlayer.loadDemo(ui.pathLineEdit->text());

	QString mapName = QString::fromStdString(demoPlayer.demo->_mapName); 

	// Load background image and scalinda data
	QString bgImagePath = dataPath + "maps\\" + mapName + ".png";
	QString scalingPath = dataPath + "maps\\" + mapName + ".txt";

	int origin[3]; int end[3];

	bool forceData = false;

	QFileInfo info(bgImagePath);
	if(!info.exists())
	{
		if(forceData)
		{
			QMessageBox msgBox;
			msgBox.setText("Map data not found.");		
			msgBox.exec();
			ui.pathLineEdit->setText("");
			ui.paintWidget->bgMessage = "Drag and drop a demo here.";
			return;
		}
		else
		{
			QMessageBox msgBox;
			msgBox.setText("Map data not found. Using default parameters. Improper visualization may occur.");		
			msgBox.exec();
			ui.paintWidget->bgMessage = "";
		}
	}

	// Set Scaling
	if(!getScalingData(scalingPath, origin, end))
	{
		// Get scaling data from the demo itself
		demoPlayer.getDemoBoundingBox(origin, end);
	}

	ui.paintWidget->releaseImage();
	ui.paintWidget->loadImage(bgImagePath);
	ui.paintWidget->setScaling(origin, end);
	ui.paintWidget->demo = demoPlayer.demo;
	ui.paintWidget->players = &demoPlayer.playerList;
	ui.paintWidget->entities = &demoPlayer.entities;	
	ui.paintWidget->beams = &demoPlayer.beams;
	ui.paintWidget->clock = &demoPlayer.clock;
	ui.paintWidget->warmupTime = &demoPlayer.warmupTime;
	ui.paintWidget->displayDemo = true;
	ui.progressSlider->setValue(0);
	ui.playButton->setText("Pause");
	paused = false;
	demoPlayer.playDemo();

	LogMessage("Demo loaded");
}


void Gui::loadIconData()
{
	QStringList filters; 
	filters << "*.png";
	QDir dir(dataPath + "icons" + QDir::separator());
	ui.paintWidget->loadIcons(dataPath + "icons\\", dir.entryList(filters));

	QDir dir2(dataPath + "weapons" + QDir::separator());
	ui.paintWidget->loadWeapons(dataPath + "weapons\\", dir2.entryList(filters));
}


void Gui::playButtonPressed()
{
	if(paused)
	{
		if(demoPlayer.demo != NULL)
		{
			demoPlayer.playDemo();
			paused = false;
			ui.playButton->setText("Pause");
		}
		else if(demoPlayer.loadDemo(ui.pathLineEdit->text()))
		{
			ui.paintWidget->demo = demoPlayer.demo;
			ui.paintWidget->players = &demoPlayer.playerList;
			ui.paintWidget->entities = &demoPlayer.entities;	
			ui.paintWidget->beams = &demoPlayer.beams;
			ui.paintWidget->clock = &demoPlayer.clock;
			ui.paintWidget->warmupTime = &demoPlayer.warmupTime;
			ui.paintWidget->displayDemo = true;
			ui.progressSlider->setValue(0);
			ui.playButton->setText("Pause");
			paused = false;
			demoPlayer.playDemo();
		}
		else
		{
			QMessageBox msgBox;
			msgBox.setText("Demo file not found.");
			msgBox.exec();
		}
	}
	else
	{
		demoPlayer.pauseDemo();
		paused = true;
		ui.playButton->setText("Play");
	}
	
}

void Gui::stopButtonPressed()
{
	paused = true;
	ui.playButton->setText("Play");
	demoPlayer.stopDemo();
}

void Gui::updateProgressSlider( float p )
{
	if(!ui.progressSlider->hasFocus())
		ui.progressSlider->setValue(p * ui.progressSlider->maximum());
}

void Gui::timeScaleChanged( double v )
{
	float value = v;
	if(ui.reverseCheckBox->isChecked())
		v = -v;
	demoPlayer.timescale = v;
}

void Gui::showClockChanged(int state)
{
	ui.paintWidget->showClock = (state == Qt::Checked);
}

void Gui::showScoreChanged( int state)
{
	ui.paintWidget->showScore = (state == Qt::Checked);
}

void Gui::showHudChanged( int state)
{
	ui.paintWidget->showHud = (state == Qt::Checked);
}


void Gui::progressSliderValueChanged(int v)
{
	float perc = v / (float) ui.progressSlider->maximum(); 
	demoPlayer.elapsedTime = demoPlayer.gameStartElapsed + perc * demoPlayer.gameLength;

	//ui.progressSlider->clearFocus();

	// Make sure the visualization is updated.
	if(!demoPlayer.timer.isActive())
		demoPlayer.update();
}

void Gui::reverseTimeChanged( int )
{
	if(ui.reverseCheckBox->isChecked())
	{
		ui.timeScaleDoubleSpinBox->setPrefix("-");
		demoPlayer.timescale = -demoPlayer.timescale;
	}
	else
	{
		ui.timeScaleDoubleSpinBox->setPrefix("");
		demoPlayer.timescale = -demoPlayer.timescale;
	}
}

void Gui::dropEvent( QDropEvent* event )
{
	const QMimeData* mimeData = event->mimeData();

	// check for our needed mime type, here a file or a list of files
	if (mimeData->hasUrls())
	{
		QStringList pathList;
		QList<QUrl> urlList = mimeData->urls();

		// extract the local paths of the files
		for (int i = 0; i < urlList.size() && i < 32; ++i)
		{
			ui.paintWidget->displayDemo = false;
			ui.paintWidget->bgMessage = "Loading... 0%";
			ui.paintWidget->repaint();
			pathList.append(urlList.at(i).toLocalFile());
			progressTimer.restart();
			loadDemo(pathList[0]);
		}
	}
}

void Gui::dragEnterEvent( QDragEnterEvent* event )
{
	const QMimeData* mimeData = event->mimeData();

	// check for our needed mime type, here a file or a list of files
	if (mimeData->hasUrls())
	{
		QStringList pathList;
		QList<QUrl> urlList = mimeData->urls();

		// extract the local paths of the files
		for (int i = 0; i < urlList.size() && i < 32; ++i)
		{
			pathList.append(urlList.at(i).toLocalFile());
		}
		if(pathList.size() == 1)
		{
			QStringList tockens = pathList.at(0).split(".");
			if(tockens.back() == "dm_73" || tockens.back() == "dm_68")
			{
				
				event->accept();
			}
		}
	}
}

void Gui::demoFinished()
{
	demoPlayer.pauseDemo();
	paused = true;
	ui.playButton->setText("Play");
}

void Gui::onProgress(float progress)
{
	if(progressTimer.elapsed() < 100)
	{
		return;
	}

	QString message;
	message.sprintf("Loading... %d%%", (int)(100.0f * progress));
	ui.paintWidget->bgMessage = message;
	ui.paintWidget->repaint();

	progressTimer.restart();
}

void Gui::onMessage(int logLevel, const char* message)
{
	switch(logLevel)
	{
	case 0: logWidget->appendPlainText("Info: "); break;
	case 1: logWidget->appendPlainText("Warning: "); break;
	case 2: logWidget->appendPlainText("Error: "); break;
	case 3: logWidget->appendPlainText("Critical: "); break;
	}

	logWidget->appendPlainText(message);
	logWidget->appendPlainText("\n");
}



