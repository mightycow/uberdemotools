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

static QPlainTextEdit* logWidget = NULL; // Used by Gui::onMessage.


Gui::Gui(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), _demoPlayer(this)
{
	gui = this;
	_progressCallback = &UdtProgressCallback;
	_messageCallback = &UdtMessageCallback;

	_ui.setupUi(this);
	logWidget = _ui.logWidget;

	connectUiElements();
	_ui.pathLineEdit->setReadOnly(true);

	DataPath = "";
	QDir dir;
	const size_t searchDirCount = sizeof(dataSearchDirs) / sizeof(dataSearchDirs[0]);
	for(size_t i = 0; i < searchDirCount; ++i)
	{
		const QString dataFolder = dataSearchDirs[i];
		if(dir.exists(dataFolder))
		{
			DataPath = dataFolder;
			break;
		}
	}
	if(DataPath.isEmpty())
	{
		DataPath = defaultDataDir;
	}
	
	_paused = true;

	loadIconData();
}

Gui::~Gui()
{
}

void Gui::connectUiElements()
{
	connect(&_demoPlayer, SIGNAL(entitiesUpdated()), _ui.paintWidget, SLOT(repaint()));
	connect(&_demoPlayer, SIGNAL(progress(float)), this, SLOT(updateProgressSlider(float)));
	connect(&_demoPlayer, SIGNAL(demoFinished()), this, SLOT(demoFinished()));
	connect(_ui.progressSlider, SIGNAL(valueChanged(int)), this, SLOT(progressSliderValueChanged(int)));
	connect(_ui.playButton, SIGNAL(pressed()), this, SLOT(playButtonPressed()));
	connect(_ui.stopButton, SIGNAL(pressed()), this, SLOT(stopButtonPressed()));
	connect(_ui.timeScaleDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(timeScaleChanged(double)));
	connect(_ui.showClockCheckBox, SIGNAL(stateChanged(int)), this, SLOT(showClockChanged(int)));
	connect(_ui.showScoresCheckBox, SIGNAL(stateChanged(int)), this, SLOT(showScoreChanged(int)));
	connect(_ui.showHudCheckBox, SIGNAL(stateChanged(int)), this, SLOT(showHudChanged(int)));
	connect(_ui.reverseCheckBox, SIGNAL(stateChanged(int)), this, SLOT(reverseTimeChanged(int)));
}

bool Gui::getScalingData( QString scalingPath, int* origin, int* end ) 
{
	QFile file(scalingPath);
	if(!file.open(QIODevice::ReadOnly))
	{
		LogWarning("Failed to open %s.", scalingPath.toLocal8Bit().constData());
		return false;
	}

	QTextStream ts(&file);
	int lineIndex = 1;
	while(!ts.atEnd())
	{
		const QString line = ts.readLine();
		const QStringList tokens = line.split(" ");

		if(tokens.size() != 5)
		{
			LogWarning(
				"Unable to read %s properly. Failed at line %d. Check the syntax, and remove any empty line.", 
				scalingPath.toLocal8Bit().constData(), 
				lineIndex);
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

		++lineIndex;
	}

	file.close();

	return true;
}

void Gui::loadDemo(QString filepath)
{
	_ui.pathLineEdit->setText(filepath);

	_demoPlayer.loadDemo(_ui.pathLineEdit->text());

	const QString mapName = QString::fromStdString(_demoPlayer.demo->_mapName); 
	const QString bgImagePath = DataPath + "maps\\" + mapName + ".png";
	const QString scalingPath = DataPath + "maps\\" + mapName + ".txt";
	const QFileInfo info(bgImagePath);
	if(!info.exists())
	{
		LogWarning("Map data %s not found. Using default parameters. Improper visualization may occur.", bgImagePath.toLocal8Bit().constData());
		_ui.paintWidget->bgMessage = "";
	}

	// Set scaling from our pre-computed data.
	int origin[3]; 
	int end[3];
	if(!getScalingData(scalingPath, origin, end))
	{
		// It failed, so get the scaling data from the demo itself.
		_demoPlayer.getDemoBoundingBox(origin, end);
	}

	_ui.paintWidget->releaseImage();
	_ui.paintWidget->loadImage(bgImagePath);
	_ui.paintWidget->setScaling(origin, end);
	_ui.paintWidget->demo = _demoPlayer.demo;
	_ui.paintWidget->players = &_demoPlayer.playerList;
	_ui.paintWidget->entities = &_demoPlayer.entities;	
	_ui.paintWidget->beams = &_demoPlayer.beams;
	_ui.paintWidget->clock = &_demoPlayer.clock;
	_ui.paintWidget->warmupTime = &_demoPlayer.warmupTime;
	_ui.paintWidget->scoreTable = &_demoPlayer.scoreTable;
	_ui.paintWidget->displayDemo = true;
	_ui.progressSlider->setValue(0);
	_ui.playButton->setText("Pause");
	_paused = false;
	_demoPlayer.playDemo();

	LogInfo("Demo loaded");
}


void Gui::loadIconData()
{
	QStringList filters; 
	filters << "*.png";

	const QDir iconDir(DataPath + "icons" + QDir::separator());
	_ui.paintWidget->loadIcons(DataPath + "icons\\", iconDir.entryList(filters));

	const QDir weaponsDir(DataPath + "weapons" + QDir::separator());
	_ui.paintWidget->loadWeapons(DataPath + "weapons\\", weaponsDir.entryList(filters));
}

void Gui::playButtonPressed()
{
	if(!_paused)
	{
		_demoPlayer.pauseDemo();
		_paused = true;
		_ui.playButton->setText("Play");
		return;
	}

	if(_demoPlayer.demo != NULL)
	{
		_demoPlayer.playDemo();
		_paused = false;
		_ui.playButton->setText("Pause");
	}
	else if(_demoPlayer.loadDemo(_ui.pathLineEdit->text()))
	{
		_ui.paintWidget->demo = _demoPlayer.demo;
		_ui.paintWidget->players = &_demoPlayer.playerList;
		_ui.paintWidget->entities = &_demoPlayer.entities;	
		_ui.paintWidget->beams = &_demoPlayer.beams;
		_ui.paintWidget->clock = &_demoPlayer.clock;
		_ui.paintWidget->warmupTime = &_demoPlayer.warmupTime;
		_ui.paintWidget->displayDemo = true;
		_ui.progressSlider->setValue(0);
		_ui.playButton->setText("Pause");
		_paused = false;
		_demoPlayer.playDemo();
	}
	else
	{
		LogError("Demo file not found.");
	}
}

void Gui::stopButtonPressed()
{
	_paused = true;
	_ui.playButton->setText("Play");
	_demoPlayer.stopDemo();
}

void Gui::updateProgressSlider(float progress)
{
	if(!_ui.progressSlider->hasFocus())
	{
		_ui.progressSlider->setValue(progress * _ui.progressSlider->maximum());
	}
}

void Gui::timeScaleChanged(double editValue)
{
	const float newValue = (float)editValue;
	_demoPlayer.timescale = _ui.reverseCheckBox->isChecked() ? newValue : -newValue;
}

void Gui::showClockChanged(int state)
{
	_ui.paintWidget->showClock = (state == Qt::Checked);
}

void Gui::showScoreChanged(int state)
{
	_ui.paintWidget->showScore = (state == Qt::Checked);
}

void Gui::showHudChanged(int state)
{
	_ui.paintWidget->showHud = (state == Qt::Checked);
}

void Gui::progressSliderValueChanged(int editValue)
{
	const float progressPc = editValue / (float) _ui.progressSlider->maximum(); 
	_demoPlayer.elapsedTime = _demoPlayer.gameStartElapsed + progressPc * _demoPlayer.gameLength;

	// Make sure the visualization is updated.
	if(!_demoPlayer.timer.isActive())
	{
		_demoPlayer.update();
	}
}

void Gui::reverseTimeChanged(int)
{
	if(_ui.reverseCheckBox->isChecked())
	{
		_ui.timeScaleDoubleSpinBox->setPrefix("-");
		_demoPlayer.timescale = -_demoPlayer.timescale;
	}
	else
	{
		_ui.timeScaleDoubleSpinBox->setPrefix("");
		_demoPlayer.timescale = -_demoPlayer.timescale;
	}
}

void Gui::dropEvent(QDropEvent* event)
{
	// Check for our needed mime type, here a file or a list of files.
	const QMimeData* mimeData = event->mimeData();
	if(!mimeData->hasUrls())
	{
		return;
	}

	QStringList pathList;
	const QList<QUrl> urlList = mimeData->urls();

	for(int i = 0; i < urlList.size() && i < 32; ++i)
	{
		_ui.paintWidget->displayDemo = false;
		_ui.paintWidget->bgMessage = "Loading... 0%";
		_ui.paintWidget->repaint();
		pathList.append(urlList.at(i).toLocalFile());
		_progressTimer.restart();
		loadDemo(pathList[0]);
	}
}

void Gui::dragEnterEvent( QDragEnterEvent* event )
{
	// Check for our needed mime type, here a file or a list of files.
	const QMimeData* mimeData = event->mimeData();
	if(!mimeData->hasUrls())
	{
		return;
	}

	QStringList pathList;
	const QList<QUrl> urlList = mimeData->urls();

	// Extract the local file paths.
	for(int i = 0; i < urlList.size() && i < 32; ++i)
	{
		pathList.append(urlList.at(i).toLocalFile());
	}

	if(pathList.size() == 1)
	{
		const QStringList tokens = pathList.at(0).split(".");
		if(tokens.back() == "dm_73" || tokens.back() == "dm_68")
		{
			event->accept();
		}
	}
}

void Gui::demoFinished()
{
	_demoPlayer.pauseDemo();
	_paused = true;
	_ui.playButton->setText("Play");
}

void Gui::onProgress(float progress)
{
	// We don't upgrade the progress more than 10x a second.
	if(_progressTimer.elapsed() < 100)
	{
		return;
	}

	QString message;
	message.sprintf("Loading... %d%%", (int)(100.0f * progress));
	_ui.paintWidget->bgMessage = message;
	_ui.paintWidget->repaint();

	_progressTimer.restart();
}

void Gui::onMessage(int logLevel, const char* message)
{
	QString formattedMsg;
	switch(logLevel)
	{
	case 1: formattedMsg = QString("<p style=\"color:#FF7C21\">Warning: %1</p>").arg(message); break;
	case 2: formattedMsg = QString("<p style=\"color:red\">Error: %1</p>").arg(message); break;
	case 3: formattedMsg = QString("<p style=\"color:red\">Critical Error: %1</p>").arg(message); break;
	default: formattedMsg = message; break;
	}

	logWidget->appendHtml(formattedMsg);
}



