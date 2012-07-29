#include "qt/gui.h"

#include <QtGui/QApplication>
#include <string>


#ifdef _WIN32
#	include <Windows.h>
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
#endif


int main(int argumentCount, char** arguments)
{
#ifdef _WIN32
	ResetCurrentDirectory(arguments[0]);
#endif

	QApplication app(argumentCount, arguments);
	Q_INIT_RESOURCE(U2DDV);
	app.setWindowIcon(QIcon(":/icon.png"));

	Gui gui;
	gui.ProcessCommandLine(argumentCount, arguments);
	gui.show();

	return app.exec();
}
