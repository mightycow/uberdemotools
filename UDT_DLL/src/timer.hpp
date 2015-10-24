#pragma once


#include "uberdemotools.h"
#include "macros.hpp"


struct udtTimerImpl;

struct udtTimer
{
public:
	udtTimer();
	~udtTimer();

	void	Start();
	void	Stop();
	void	Reset();   // Stops the timer and resets the elapsed time.
	void	Restart(); // Resets the elapsed time and restarts the timer.
	bool	IsRunning() const;

	u64		GetElapsedSec() const;
	u64		GetElapsedMs() const;
	u64		GetElapsedUs() const;

	void	SetElapsedSec(u64 uElapsedSec);
	void	SetElapsedMs(u64 uElapsedMs);
	void	SetElapsedUs(u64 uElapsedUs);

private:
	UDT_NO_COPY_SEMANTICS(udtTimer);

	udtTimerImpl* _data;
};
