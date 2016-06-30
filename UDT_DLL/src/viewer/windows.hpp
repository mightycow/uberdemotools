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
	data->EntryPoint = userEntryPoint;
	data->UserData = userData;
	CreateThread(nullptr, 0, &ThreadEntryPoint, data, 0, nullptr);
}

void Platform_CreateCriticalSection(CriticalSectionId& csRef)
{
	CRITICAL_SECTION* const cs = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(cs);
	csRef = cs;
}

void Platform_ReleaseCriticalSection(CriticalSectionId cs)
{
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
