void Platform_DebugPrint(const char* format, ...)
{
	char msg[256];
	va_list args;
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);
	OutputDebugStringA("\n");
	OutputDebugStringA(msg);
	OutputDebugStringA("\n");
}

struct ThreadData
{
	Platform_ThreadFunc EntryPoint;
	void* UserData;
};

static DWORD WINAPI ThreadEntryPoint(_In_ LPVOID lpParameter)
{
	const ThreadData& data = *(const ThreadData*)lpParameter;
	(*data.EntryPoint)(data.UserData);
	free(lpParameter);

	return 0;
}

void Platform_NewThread(Platform_ThreadFunc userEntryPoint, void* userData)
{
	ThreadData* const data = (ThreadData*)malloc(sizeof(ThreadData));
	if(data == nullptr)
	{
		Platform_FatalError("Failed to allocate %d bytes for thread data", (int)sizeof(ThreadData));
	}
	data->EntryPoint = userEntryPoint;
	data->UserData = userData;
	CreateThread(nullptr, 0, &ThreadEntryPoint, data, 0, nullptr);
}

void Platform_CreateCriticalSection(CriticalSectionId& csRef)
{
	CRITICAL_SECTION* const cs = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
	if(cs == nullptr)
	{
		Platform_FatalError("Failed to allocate %d bytes for critical section data", (int)sizeof(CRITICAL_SECTION));
	}
	InitializeCriticalSection(cs);
	csRef = cs;
}

void Platform_ReleaseCriticalSection(CriticalSectionId cs)
{
	if(cs == InvalidCriticalSectionId)
	{
		return;
	}

	DeleteCriticalSection((CRITICAL_SECTION*)cs);
	free(cs);
}

void Platform_EnterCriticalSection(CriticalSectionId cs)
{
	EnterCriticalSection((CRITICAL_SECTION*)cs);
}

void Platform_LeaveCriticalSection(CriticalSectionId cs)
{
	LeaveCriticalSection((CRITICAL_SECTION*)cs);
}

static void ResetCurrentDirectory(const char* exePath)
{
	udtVMLinearAllocator& alloc = udtThreadLocalAllocators::GetTempAllocator();
	udtVMScopedStackAllocator allocScope(alloc);

	udtString folderPath;
	if(!udtPath::GetFolderPath(folderPath, alloc, udtString::NewConstRef(exePath)))
	{
		return;
	}

	wchar_t* const folderPathWide = udtString::ConvertToUTF16(alloc, folderPath);
	if(folderPathWide == nullptr)
	{
		return;
	}

	SetCurrentDirectoryW(folderPathWide);
}

void Platform_PrintError(const char* format, ...)
{
	char msg[256];
	va_list args;
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);
	MessageBoxA(nullptr, msg, "UDT 2D Viewer - Error", MB_OK | MB_ICONERROR);
}

void Platform_FatalError(const char* format, ...)
{
	// @TODO: Write a mini-dump file?
	// @TODO: Write a call-stack file?
	char msg[256];
	va_list args;
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);
	MessageBoxA(nullptr, msg, "UDT 2D Viewer - Fatal Error", MB_OK | MB_ICONERROR);
	exit(666);
}

static int Win32ExceptionFilter()
{
	return EXCEPTION_EXECUTE_HANDLER;
}

static int Win32ExceptionHandler(int code)
{
	char msg[256];
	sprintf(msg, "A Win32 exception with code %08X was thrown.\nShutting down.", (int)code);
	OutputDebugStringA(msg);
	
	return 666;
}
