#include "timer.hpp"
#include "memory.hpp"


#if defined(UDT_WINDOWS)


#include <Windows.h>


/*
Here are the timers we recognize and support:
-----------------------
3.57 MHz (3,579,545 Hz)
ACPI PMT
-----------------------
1.19 MHz (1,193,182 Hz)
8245 PIT
-----------------------
*/

#define	   FREQUENCY_ACPI_PMT    3579545ull
#define	   FREQUENCY_PIT_8245    1193182ull


typedef u64(*GetElapsedTicksFuncPtr)();
typedef u64(*GetFrequencyFuncPtr)();

static bool IsQpcAvailable()
{
	LARGE_INTEGER freq;
	const BOOL iQpcUsable = QueryPerformanceFrequency(&freq);

	return iQpcUsable != FALSE && (freq.QuadPart == FREQUENCY_ACPI_PMT || freq.QuadPart == FREQUENCY_PIT_8245);
}

static u64 GetElapsedTicksQpc()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return (u64)counter.QuadPart;
}

static u64 GetFrequencyQpc()
{
	LARGE_INTEGER counter;
	QueryPerformanceFrequency(&counter);

	return (u64)counter.QuadPart;
}

static u64 GetElapsedTicksTgt()
{
	return (u64)timeGetTime();
}

static u64 GetFrequencyTgt()
{
	return 1000;
}


struct udtTimerImpl
{
	u64 GetElapsedTicks()
	{
		return Running ? ((*GetElapsedTicksFunc)() + ElapsedTime - StartTime) : ElapsedTime;
	}

	u64 Frequency;
	u64 StartTime;
	u64	ElapsedTime;
	GetElapsedTicksFuncPtr GetElapsedTicksFunc;
	GetFrequencyFuncPtr GetFrequencyFunc;
	bool Running;
};


udtTimer::udtTimer()
{
	_data = (udtTimerImpl*)udt_malloc(sizeof(udtTimerImpl));
	const bool bIsHighPerf = IsQpcAvailable();
	_data->GetFrequencyFunc = bIsHighPerf ? &GetFrequencyQpc : &GetFrequencyTgt;
	_data->GetElapsedTicksFunc = bIsHighPerf ? &GetElapsedTicksQpc : &GetElapsedTicksTgt;
	_data->Frequency = (*_data->GetFrequencyFunc)();
	_data->StartTime = 0;
	_data->ElapsedTime = 0;
	_data->Running = false;
}

udtTimer::~udtTimer()
{
	free(_data);
}

void udtTimer::Start()
{
	_data->Running = true;
	_data->StartTime = (*_data->GetElapsedTicksFunc)();
}

void udtTimer::Stop()
{
	_data->Running = false;
	_data->ElapsedTime += (*_data->GetElapsedTicksFunc)() - _data->StartTime;
}

void udtTimer::Reset()
{
	_data->Running = false;
	_data->ElapsedTime = 0;
}

void udtTimer::Restart()
{
	_data->ElapsedTime = 0;
	Start();
}

bool udtTimer::IsRunning() const
{
	return _data->Running;
}

u64	udtTimer::GetElapsedSec() const
{
	return _data->GetElapsedTicks() / _data->Frequency;
}

u64 udtTimer::GetElapsedMs() const
{
	return (1000ull * _data->GetElapsedTicks()) / _data->Frequency;
}

u64 udtTimer::GetElapsedUs() const
{
	return (1000000ull * _data->GetElapsedTicks()) / _data->Frequency;
}

void udtTimer::SetElapsedSec(u64 uElapsedSec)
{
	_data->ElapsedTime = (u64)((f32)uElapsedSec * (f32)_data->Frequency);
}

void udtTimer::SetElapsedMs(u64 uElapsedMs)
{
	_data->ElapsedTime = (u64)(((f32)uElapsedMs / 1000.0f) * (f32)_data->Frequency);
}

void udtTimer::SetElapsedUs(u64 uElapsedUs)
{
	_data->ElapsedTime = (u64)(((f32)uElapsedUs / 1000000.0f) * (f32)_data->Frequency);
}


#else


#include <time.h>
#include <stdlib.h>


static u64 GetElapsedTimeNs(const timespec start, const timespec end)
{
	timespec temp;
	if(end.tv_nsec - start.tv_nsec < 0)
	{
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	}
	else
	{
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}

	return ((u64)temp.tv_sec * (u64)1000000000) + (u64)temp.tv_nsec;
}

struct udtTimerImpl
{
	void GetAbsoluteTime(timespec* timeSpec) const
	{
		clock_gettime(ClockId, timeSpec);
	}

	u64 GetElapsedTimeNs() const
	{
		timespec endTime;
		GetAbsoluteTime(&endTime);

		return ::GetElapsedTimeNs(StartTime, endTime);
	}

	clockid_t ClockId;
	timespec StartTime;
	u64 ElapsedNs;
	bool Running;
};


udtTimer::udtTimer()
{
	_data = (udtTimerImpl*)udt_malloc(sizeof(udtTimerImpl));
	_data->ClockId = CLOCK_MONOTONIC;
	_data->StartTime.tv_sec = 0;
	_data->StartTime.tv_nsec = 0;
	_data->ElapsedNs = 0;
	_data->Running = false;
}

udtTimer::~udtTimer()
{
	free(_data);
}

void udtTimer::Start()
{
	_data->Running = true;
	_data->GetAbsoluteTime(&_data->StartTime);
}

void udtTimer::Stop()
{
	_data->Running = false;
	_data->ElapsedNs += _data->GetElapsedTimeNs();
}

void udtTimer::Reset()
{
	_data->Running = false;
	_data->ElapsedNs = 0;
}

void udtTimer::Restart()
{
	_data->ElapsedNs = 0;
	Start();
}

bool udtTimer::IsRunning() const
{
	return _data->Running;
}

u64	udtTimer::GetElapsedSec() const
{
	if(!_data->Running)
	{
		return _data->ElapsedNs / (u64)1000000000;
	}

	return (_data->GetElapsedTimeNs() + _data->ElapsedNs) / (u64)1000000000;
}

u64 udtTimer::GetElapsedMs() const
{
	if(!_data->Running)
	{
		return _data->ElapsedNs / (u64)1000000;
	}

	return (_data->GetElapsedTimeNs() + _data->ElapsedNs) / (u64)1000000;
}

u64 udtTimer::GetElapsedUs() const
{
	if(!_data->Running)
	{
		return _data->ElapsedNs / (u64)1000;
	}

	return (_data->GetElapsedTimeNs() + _data->ElapsedNs) / (u64)1000;
}

void udtTimer::SetElapsedSec(u64 uElapsedSec)
{
	_data->ElapsedNs = uElapsedSec * (u64)1000000000;
}

void udtTimer::SetElapsedMs(u64 uElapsedMs)
{
	_data->ElapsedNs = uElapsedMs * (u64)1000000;
}

void udtTimer::SetElapsedUs(u64 uElapsedUs)
{
	_data->ElapsedNs = uElapsedUs * (u64)1000;
}


#endif
