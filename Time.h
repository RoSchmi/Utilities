#pragma once

#include "Common.h"

namespace Utilities {
	class exported DateTime {
		public:
			/// The date/time this represents, in milliseconds
			uint64 milliseconds;

			DateTime();
			DateTime(uint64 milliseconds);
			DateTime(const DateTime& other);
			DateTime& operator=(const DateTime& other);

			DateTime& operator=(uint64 milliseconds);
			operator uint64();

			uint64 getMilliseconds() const;
			
			DateTime& addMilliseconds(uint64 milliseconds);
			DateTime& addSeconds(uint64 seconds);
			DateTime& addMinutes(uint64 minutes);
			DateTime& addHours(uint64 hours);
			DateTime& addDays(uint64 days);

			/**
			 * @returns a new DateTime for now
			 */
			static DateTime now();
			static uint64 getMillisecondsSinceEpoch();
	};
};
