#ifndef INCLUDE_UTILITIES_TIME
#define INCLUDE_UTILITIES_TIME

#include "Common.h"

exported uint64 Time_Now(void);
exported uint64 Time_AddDays(uint64 time, int32 days);
exported uint64 Time_AddHours(uint64 time, int32 hours);
exported uint64 Time_AddMinutes(uint64 time, int32 minutes);
exported uint64 Time_AddSeconds(uint64 time, int32 seconds);

#endif
