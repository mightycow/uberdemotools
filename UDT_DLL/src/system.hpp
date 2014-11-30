#pragma once


#include "types.hpp"


// The output "coreCount" is only written to if the function is successful.
extern bool GetProcessorCoreCount(u32& coreCount);
