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
	data->EntryPoint = userEntryPoint;
	data->UserData = userData;
	pthread_create(&thread, nullptr, &ThreadEntryPoint, data);
}

void Platform_CreateCriticalSection(CriticalSectionId& cs)
{
	// @NOTE: The process sharing mode is set to PTHREAD_PROCESS_PRIVATE by default (what we want for this app).
	pthread_mutex_t* const mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, nullptr);
	cs = mutex;
}

void Platform_ReleaseCriticalSection(CriticalSectionId cs)
{
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