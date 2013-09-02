#pragma once

#include "Common.h"

namespace Utilities {
	class exported DateTime {
		uint64 milliseconds;

		public:
			DateTime();
			DateTime(uint64 milliseconds);
			DateTime(const DateTime& other);
			DateTime& operator=(const DateTime& other);

			DateTime& operator=(uint64 milliseconds);
			operator uint64();

			uint64 getMilliseconds() const;
			
			DateTime& addMilliseconds(int64 milliseconds);
			DateTime& addSeconds(int64 seconds);
			DateTime& addMinutes(int64 minutes);
			DateTime& addHours(int64 hours);
			DateTime& addDays(int64 days);

			/**
			 * @returns a new DateTime for now
			 */
			static DateTime now();
			static uint64 getMillisecondsSinceEpoch();
	};
}
