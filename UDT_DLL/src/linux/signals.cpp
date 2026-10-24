#include <execinfo.h> // backtrace
#include <unistd.h> // fork, pipe, ...
#include <signal.h>
#include <string.h>
#include <sys/wait.h> // waitpid
#include <stdlib.h> // EXIT_SUCCESS, ...
#include <stdint.h>

static char s_addr2line[4096];
static char s_exePath[4096];

static void Write(const char* msg)
{
	write(STDERR_FILENO, msg, strlen(msg));
}

static void Write(const char* msg, size_t length)
{
	write(STDERR_FILENO, msg, length);
}

static void WriteNewLine()
{
	write(STDERR_FILENO, "\n", 1);
}

static void WriteAddress(const void* address)
{
	if(address == NULL)
	{
		Write("0");
		return;
	}

	const char hexDigits[] = "0123456789ABCDEF";
	char hexString[17];
	uint64_t v = (uint64_t)address;
	for(int i = 0; i < 16; ++i)
	{
		hexString[15 - i] = hexDigits[v & 15];
		v >>= 4;
	}
	hexString[16] = '\0';

	// skip leading zero digits
	int offset = 0;
	for(; offset < 16; ++offset)
	{
		if(hexString[offset] != '0')
		{
			break;
		}
	}

	Write("0x");
	Write(hexString + offset);
}

static void AddressToHexString(char hexString[19], uint64_t address)
{
	const char hexDigits[] = "0123456789abcdef";
	hexString[0] = '0';
	hexString[1] = 'x';
	for(int i = 17; i >= 2; --i)
	{
		hexString[i] = hexDigits[address & 15];
		address >>= 4;
	}
	hexString[18] = '\0';
}

static bool ExecAddr2Line(char* readBuffer, size_t readBufferSize, uint64_t address)
{
	// anonymous pipe
	int fd[2];
	if(pipe(fd) < 0)
	{
		return false;
	}

	const pid_t pid = fork();
	if(pid < 0)
	{
		close(fd[0]);
		close(fd[1]);
		return false;
	}

	if(pid == 0)
	{
		//
		// child process
		//

		// we don't need read access
		close(fd[0]);

		// redirect stdout of addr2line into our pipe
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);

		char addressString[19];
		AddressToHexString(addressString, address);

		// @NOTE: "-e /proc/self/exe" does *not* work when running through WSL
		char* cmds[] =
		{
			// addr2line --functions --demangle=auto --exe=./UDT_cutter 0x10bb4f6
			(char*)"/usr/bin/addr2line",
			(char*)"--functions",
			(char*)"--demangle=auto",
			(char*)"-e", // same as "--exe=path"
			(char*)s_exePath,
			addressString,
			NULL
		};
		char* env[] = { NULL };
		execve(cmds[0], cmds, env);

		_exit(1);
	}
	else
	{
		//
		// parent process
		//

		// we don't need write access
		close(fd[1]);

		size_t readByteCount = 0;
		size_t totalByteCount = 0;
		while(totalByteCount < readBufferSize - 1)
		{
			readByteCount = read(fd[0], readBuffer + totalByteCount, readBufferSize - totalByteCount - 1);
			if(readByteCount <= 0)
			{
				break;
			}
			totalByteCount += readByteCount;
		}
		close(fd[0]);

		// wait for the child to be done
		int status;
		while(waitpid(pid, &status, 0) < 0)
		{
		}

		if(totalByteCount <= 0)
		{
			// nothing of use...
			return false;
		}

		if((size_t)totalByteCount >= readBufferSize)
		{
			// no space left for the NULL terminator
			return false;
		}

		readBuffer[totalByteCount] = '\0';
		if(readBuffer[0] == '?')
		{
			// addr2line failed to resolve the address
			return false;
		}
	}

	return true;
}

static void CrashSignalHandler(int signum, siginfo_t* info, void* context)
{
	const char* code = NULL;
	const char* desc = NULL;
	switch(signum)
	{
		case SIGSEGV: code = "SEGV"; desc = "invalid memory reference"; break;
		case SIGABRT: code = "ABRT"; desc = "IOT trap"; break;
		case SIGBUS: code = "BUS"; desc = "bus error (bad memory access)"; break;
		case SIGFPE: code = "FPE"; desc = "fatal arithmetic error"; break;
		case SIGILL: code = "ILL"; desc = "illegal instruction"; break;
		default: code = "???"; desc = "???"; break;
	}

	Write("Code: ");
	Write(code);
	WriteNewLine();

	Write("Desc: ");
	Write(desc);
	WriteNewLine();

#if defined(__x86_64__)
	ucontext_t* const ctx = (ucontext_t*)context;
	if(ctx != NULL && info != NULL)
	{
		const bool isPageFault =
			signum == SIGSEGV ||
			(signum == SIGBUS && info->si_code == BUS_ADRERR);
		if(isPageFault)
		{
			const unsigned long code = (unsigned long)ctx->uc_mcontext.gregs[REG_ERR];
			if(code & 2)
			{
				Write("Write error at ");
			}
			else
			{
				Write("Read error at ");
			}
			WriteAddress(info->si_addr);
			WriteNewLine();
		}
	}
#endif

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
			_exit(EXIT_FAILURE);
		}

		Write("\nCall stack:\n");
		for(int i = 0; i < addressCount; ++i)
		{
			if(!ExecAddr2Line(s_addr2line, sizeof(s_addr2line), (uint64_t)addresses[i]))
			{
				continue;
			}

			char* function = s_addr2line;
			size_t functionLength = 0;
			char* s = s_addr2line;
			while(*s != '\0' && *s != '\n')
			{
				functionLength++;
				s++;
			}
			char* fileAndLine = s + 1;
			char* lastSlash = fileAndLine + strlen(fileAndLine) - 1;
			while(lastSlash > fileAndLine)
			{
				if(*lastSlash == '/' || *lastSlash == '\\')
				{
					fileAndLine = lastSlash + 1;
					break;
				}
				lastSlash--;
			}
			size_t fileLength = 0;
			s = fileAndLine;
			while(*s != '\0' && *s != '\n')
			{
				fileLength++;
				s++;
			}
			Write(function, functionLength);
			Write(" at ");
			Write(fileAndLine, fileLength);
			WriteNewLine();
		}

		_exit(EXIT_SUCCESS);
	}
	else
	{
		//
		// parent process
		//

		// run our own handler in the child process
		int status = 0;
		waitpid(pid, &status, 0);

		// now let the system take a stab at handling the signal
		// e.g. generate a core dump
		Write("\nRaising signal again...\n");
		signal(signum, SIG_DFL);
		raise(signum);
	}	
}

void InitSignalHandlers(const char* fullExePath)
{
	struct sigaction action = {};
	action.sa_sigaction = &CrashSignalHandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &action, NULL); // invalid memory reference
	sigaction(SIGABRT, &action, NULL); // IOT trap (a synonym for SIGABRT)
	sigaction(SIGBUS,  &action, NULL); // bus error (bad memory access)
	sigaction(SIGFPE,  &action, NULL); // fatal arithmetic error
	sigaction(SIGILL,  &action, NULL); // illegal instruction

	strcpy(s_exePath, fullExePath);
}
