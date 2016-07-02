void Platform_DebugPrint(const char* format, ...)
{
	char msg[256];
	va_list args;
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);
	printf("\n");
	printf(msg);
	printf("\n");
}

struct ThreadData
{
	Platform_ThreadFunc EntryPoint;
	void* UserData;
};

void* ThreadEntryPoint(void* userData)
{
	const ThreadData& data = *(const ThreadData*)userData;
	(*data.EntryPoint)(data.UserData);
	free(userData);

	return (void*)0;
}

void Platform_NewThread(Platform_ThreadFunc userEntryPoint, void* userData)
{
	pthread_t thread;
	ThreadData* const data = (ThreadData*)malloc(sizeof(ThreadData));
	if(data == nullptr)
	{
		Platform_FatalError("Failed to allocate %d bytes for thread data", (int)sizeof(ThreadData));
	}
	data->EntryPoint = userEntryPoint;
	data->UserData = userData;
	pthread_create(&thread, nullptr, &ThreadEntryPoint, data);
}

void Platform_CreateCriticalSection(CriticalSectionId& cs)
{
	// @NOTE: The process sharing mode is set to PTHREAD_PROCESS_PRIVATE by default (what we want for this app).
	pthread_mutex_t* const mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	if(mutex == nullptr)
	{
		Platform_FatalError("Failed to allocate %d bytes for critical section data", (int)sizeof(pthread_mutex_t));
	}
	pthread_mutex_init(mutex, nullptr);
	cs = mutex;
}

void Platform_ReleaseCriticalSection(CriticalSectionId cs)
{
	if(cs == InvalidCriticalSectionId)
	{
		return;
	}

	pthread_mutex_destroy((pthread_mutex_t*)cs);
	free(cs);
}

void Platform_EnterCriticalSection(CriticalSectionId cs)
{
	pthread_mutex_lock((pthread_mutex_t*)cs);
}

void Platform_LeaveCriticalSection(CriticalSectionId cs)
{
	pthread_mutex_unlock((pthread_mutex_t*)cs);
}

void Platform_PrintError(const char* format, ...)
{
	char msg[256];
	va_list args;
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);
	fprintf(stderr, "Error: ");
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
}

static void PrintStackTrace(int skipCount)
{
	void* addresses[16];
	const int symbolCount = backtrace(addresses, UDT_COUNT_OF(addresses));
	if(skipCount >= symbolCount)
	{
		return;
	}
	
	backtrace_symbols_fd(addresses, symbolCount, 2);
}

void Platform_FatalError(const char* format, ...)
{
	char msg[256];
	va_list args;
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	fprintf(stderr, "Fatal error: ");
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
	fprintf(stderr, "Full stack trace:\n");
	PrintStackTrace(0);
	fprintf(stderr, "\n");
	exit(666);
}
