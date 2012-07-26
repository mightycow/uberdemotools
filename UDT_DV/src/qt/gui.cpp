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
#include <QFileDialog>
#include <QApplication>


static const char* U2DDV_version = "0.1";


static const char* const defaultDataDir = "..\\data\\";
static const char* const dataSearchDirs[] =
{
	"..\\data\\",			// Deployment.
	"..\\..\\..\\data\\"	// Development.
};

static Gui* gui = NULL;
int Gui::UdtProgressCallback(float progress)
{
	gui->OnProgress(progress);

	return 0;
}

void Gui::UdtMessageCallback(int logLevel, const char* message)
{
	gui->OnMessage(logLevel, message);
}

static QPlainTextEdit* logWidget = NULL; // Used by Gui::onMessage.


Gui::Gui()
	: QMainWindow(NULL, 0)
	, _demoPlayer(this)
	, _argumentCount(0)
	, _arguments(NULL)
{
	gui = this;
	_progressCallback = &UdtProgressCallback;
	_messageCallback = &UdtMessageCallback;

	_ui.setupUi(this);
	logWidget = _ui.logWidget;

	ConnectUiElements();
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

	LoadIconData();

	LogInfo("Uber 2D Demo Viewer version %s is now operational!", U2DDV_version);
}

Gui::~Gui()
{
}

void Gui::ConnectUiElements()
{
	connect(&_demoPlayer, SIGNAL(EntitiesUpdated()), _ui.paintWidget, SLOT(repaint()));
	connect(&_demoPlayer, SIGNAL(Progress(float)), this, SLOT(UpdateProgressSlider(float)));
	connect(&_demoPlayer, SIGNAL(DemoFinished()), this, SLOT(DemoFinished()));
	connect(_ui.progressSlider, SIGNAL(valueChanged(int)), this, SLOT(ProgressSliderValueChanged(int)));
	connect(_ui.playButton, SIGNAL(pressed()), this, SLOT(PlayButtonPressed()));
	connect(_ui.stopButton, SIGNAL(pressed()), this, SLOT(StopButtonPressed()));
	connect(_ui.timeScaleDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(TimeScaleChanged(double)));
	connect(_ui.showClockCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ShowClockChanged(int)));
	connect(_ui.showScoresCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ShowScoreChanged(int)));
	connect(_ui.showHudCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ShowHudChanged(int)));
	connect(_ui.reverseCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ReverseTimeChanged(int)));
}

bool Gui::GetScalingData(const QString& scalingPath, int* origin, int* end) 
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

void Gui::LoadDemo(const QString& filepath)
{
	if(!_demoPlayer.LoadDemo(filepath))
	{
		return;
	}

	_ui.pathLineEdit->setText(filepath);

	const QString mapName = QString::fromStdString(_demoPlayer.DemoData.Demo->_mapName); 
	const QString bgImagePath = DataPath + "maps\\" + mapName + ".png";
	const QString scalingPath = DataPath + "maps\\" + mapName + ".txt";
	const QFileInfo info(bgImagePath);
	if(!info.exists())
	{
		LogWarning("Map data %s not found. Using default parameters. Improper visualization may occur.", bgImagePath.toLocal8Bit().constData());
		_ui.paintWidget->BackgroundMessage = "";
	}

	// Set scaling from our pre-computed data.
	int origin[3]; 
	int end[3];
	if(!GetScalingData(scalingPath, origin, end))
	{
		// It failed, so get the scaling data from the demo itself.
		_demoPlayer.GetDemoBoundingBox(origin, end);
	}

	_ui.paintWidget->ReleaseImage();
	_ui.paintWidget->LoadImage(bgImagePath);
	_ui.paintWidget->SetScaling(origin, end);
	_ui.paintWidget->DemoData = &_demoPlayer.DemoData;
	_ui.paintWidget->DisplayDemo = true;
	_ui.progressSlider->setValue(0);
	_ui.playButton->setText("Pause");
	_paused = false;
	_demoPlayer.PlayDemo();

	const QFileInfo fileInfo(filepath);
	LogInfo("Demo '%s' loaded", fileInfo.fileName().toLocal8Bit().constData());
}

void Gui::LoadIconData()
{
	QStringList filters; 
	filters << "*.png";

	const QDir iconDir(DataPath + "icons" + QDir::separator());
	_ui.paintWidget->LoadIcons(DataPath + "icons\\", iconDir.entryList(filters));

	const QDir weaponsDir(DataPath + "weapons" + QDir::separator());
	_ui.paintWidget->LoadWeapons(DataPath + "weapons\\", weaponsDir.entryList(filters));
}

void Gui::PlayButtonPressed()
{
	if(!_paused)
	{
		_demoPlayer.PauseDemo();
		_paused = true;
		_ui.playButton->setText("Play");
		return;
	}

	if(_demoPlayer.DemoData.Demo != NULL)
	{
		_demoPlayer.PlayDemo();
		_paused = false;
		_ui.playButton->setText("Pause");
	}
	else if(_demoPlayer.LoadDemo(_ui.pathLineEdit->text()))
	{
		_ui.paintWidget->DemoData = &_demoPlayer.DemoData;
		_ui.paintWidget->DisplayDemo = true;
		_ui.progressSlider->setValue(0);
		_ui.playButton->setText("Pause");
		_paused = false;
		_demoPlayer.PlayDemo();
	}
	else
	{
		LogError("Demo file not found.");
	}
}

void Gui::StopButtonPressed()
{
	_paused = true;
	_ui.playButton->setText("Play");
	_demoPlayer.StopDemo();
}

void Gui::UpdateProgressSlider(float progress)
{
	if(!_ui.progressSlider->hasFocus())
	{
		_ui.progressSlider->setValue(progress * _ui.progressSlider->maximum());
	}
}

void Gui::TimeScaleChanged(double editValue)
{
	const float newValue = (float)editValue;
	_demoPlayer._timeScale = _ui.reverseCheckBox->isChecked() ? newValue : -newValue;
}

void Gui::ShowClockChanged(int state)
{
	_ui.paintWidget->ShowClock = (state == Qt::Checked);
}

void Gui::ShowScoreChanged(int state)
{
	_ui.paintWidget->ShowScore = (state == Qt::Checked);
}

void Gui::ShowHudChanged(int state)
{
	_ui.paintWidget->ShowHud = (state == Qt::Checked);
}

void Gui::ProgressSliderValueChanged(int editValue)
{
	// This needs to run only when the user is changing the value using the slider.
	if(!_ui.progressSlider->hasFocus())
	{
		return;
	}

	if(_demoPlayer.DemoData.Demo == NULL)
	{
		return;
	}
	
	const float progressPc = editValue / (float) _ui.progressSlider->maximum(); 
	_demoPlayer._elapsedTime = _demoPlayer._gameStartElapsed + progressPc * _demoPlayer._gameLength;

	// Make sure the visualization is updated.
	if(!_demoPlayer._timer.isActive())
	{
		_demoPlayer.Update();
	}
}

void Gui::ReverseTimeChanged(int)
{
	if(_ui.reverseCheckBox->isChecked())
	{
		_ui.timeScaleDoubleSpinBox->setPrefix("-");
		_demoPlayer._timeScale = -_demoPlayer._timeScale;
	}
	else
	{
		_ui.timeScaleDoubleSpinBox->setPrefix("");
		_demoPlayer._timeScale = -_demoPlayer._timeScale;
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
		_ui.paintWidget->DisplayDemo = false;
		_ui.paintWidget->BackgroundMessage = "Loading... 0%";
		_ui.paintWidget->repaint();
		pathList.append(urlList.at(i).toLocalFile());
		_progressTimer.restart();
		LoadDemo(pathList[0]);
	}
}

void Gui::dragEnterEvent(QDragEnterEvent* event)
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

void Gui::DemoFinished()
{
	_demoPlayer.PauseDemo();
	_paused = true;
	_ui.playButton->setText("Play");
}

void Gui::OnProgress(float progress)
{
	// We don't upgrade the progress more than 10x a second.
	if(_progressTimer.elapsed() < 100)
	{
		return;
	}

	QString message;
	message.sprintf("Loading... %d%%", (int)(100.0f * progress));
	_ui.paintWidget->BackgroundMessage = message;
	_ui.paintWidget->repaint();

	_progressTimer.restart();
}

void Gui::OnMessage(int logLevel, const char* message)
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

void Gui::ProcessCommandLine(int argumentCount, char** arguments)
{
	// Keep stuff for further processing.
	_argumentCount = argumentCount;
	_arguments = arguments;

	// Try opening a demo file right now.
	if(argumentCount == 2 && QFile::exists(arguments[1]))
	{
		LoadDemo(arguments[1]);
	}
}

void Gui::OnLoadDemoClicked()
{
	const QString title = "Open demo...";
	const QString directory = QDir::current().path();
	const QString filePath = QFileDialog::getOpenFileName(
		this,
		title,
		directory,
		tr("QL demo (*.dm_73);;Q3 demo (*.dm_68)")
		);

	if(filePath.isEmpty())
	{
		return;
	}

	_ui.paintWidget->DisplayDemo = false;
	_ui.paintWidget->repaint();
	LoadDemo(filePath);
}

void Gui::OnQuitClicked()
{
	close();
}

void Gui::OnAboutClicked()
{
	const QString text = 
		QString("Created by:\n- Memento_Mori (2D viewer)\n- myT (core lib, build scripts)\n\nVersion: %1\nBuilt: %2")
		.arg(U2DDV_version)
		.arg(__DATE__);
	QMessageBox::about(this, QString("Uber 2D Demo Viewer"), text);
}

void Gui::OnLogWindowClicked()
{
	if(_ui.dockWidget->isHidden())
	{
		_ui.dockWidget->show();
	}
	else
	{
		_ui.dockWidget->hide();
	}
}
