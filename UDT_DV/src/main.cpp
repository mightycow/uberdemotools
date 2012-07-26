#include "qt/gui.h"

#include <QtGui/QApplication>
#include <Windows.h>
#include <string>


static void ResetCurrentDirectory(const char* exePath)
{
	const std::string curExePath = exePath;
	const size_t idx = curExePath.rfind('\\');
	if(idx == std::string::npos)
	{
		return;
	}

	const std::string curDir = curExePath.substr(0, idx);
	SetCurrentDirectory(curDir.c_str());
}

int main(int argumentCount, char** arguments)
{
	ResetCurrentDirectory(arguments[0]);

	QApplication app(argumentCount, arguments);
	Gui gui;
	gui.ProcessCommandLine(argumentCount, arguments);
	gui.show();

	return app.exec();
}
