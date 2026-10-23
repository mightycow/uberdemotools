#include "sblz/sblz.h"
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

static char g_file_buffer[16384];
static char g_mangled[4096];
static char g_demangled[4096];

static void CrashSignalHandler(int signum)
{
	pid_t pid = fork();
	if(pid < 0)
	{
		_exit(EXIT_FAILURE);
	}

	if(pid == 0)
	{
		//
		// child process
		//

		constexpr const int MaxAddresses = 128;
		void* addresses[MaxAddresses] = {};
		const int addressCount = backtrace(addresses, MaxAddresses);
		if(addressCount <= 0)
		{
			return;
		}

		int fd = open("./stack_trace.txt", O_CREAT | O_WRONLY);
		backtrace_symbols_fd(addresses, addressCount, fd);
		close(fd);
		fd = -1;

		fd = open("./stack_trace.txt", O_RDONLY);
		read(fd, g_file_buffer, sizeof(g_file_buffer));
		close(fd);
		fd = -1;

		const char* header = "backtrace_symbols_fd call stack:\n";
		write(STDERR_FILENO, header, strlen(header));

		const size_t l = strlen(g_file_buffer);
		write(STDERR_FILENO, g_file_buffer, l);

		header = "\nDemangled call stack:\n";
		write(STDERR_FILENO, header, strlen(header));

		const char* s = g_file_buffer;
		char* d = g_mangled;
		bool symbol = false;
		while(*s != '\0')
		{
			const char c = *s++;
			if(c == '(')
			{
				d = g_mangled;
				symbol = true;
			}
			else if(c == ')' || c == '+' || c == '\n' || c == '\r')
			{
				if(symbol)
				{
					symbol = false;
					*d++ = '\0';
					if(g_mangled[0] != '\0')
					{
						if(sblz::itanium::Demangle(g_mangled, g_demangled, sizeof(g_demangled)))
						{
							write(STDERR_FILENO, g_demangled, strlen(g_demangled));
						}
						else
						{
							write(STDERR_FILENO, g_mangled, strlen(g_mangled));
						}
						write(STDERR_FILENO, "\n", 1);
					}
				}
			}
			else
			{
				if(symbol)
				{
					*d++ = c;
				}
			}
		}

		_exit(EXIT_SUCCESS);
	}
	else
	{
		// parent process

		// run our own handler in the child process
		int status = 0;
		waitpid(pid, &status, 0);

		// now let the system take a stab at handling the signal
		signal(signum, SIG_DFL);
		raise(signum);
	}	
}

void InitSignalHandlers()
{
	struct sigaction action = {};
	action.sa_handler = &CrashSignalHandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	sigaction(SIGSEGV, &action, NULL); // invalid memory reference
	sigaction(SIGABRT, &action, NULL); // IOT trap (a synonym for SIGABRT)
	sigaction(SIGBUS,  &action, NULL); // bus error (bad memory access)
	sigaction(SIGFPE,  &action, NULL); // fatal arithmetic error
	sigaction(SIGILL,  &action, NULL); // illegal instruction
}
