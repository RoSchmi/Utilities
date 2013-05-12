#pragma once

#include "Common.h"

namespace Utilities {
	/**
	 * A dynamically-allocated automatically expanding buffer.
	 */
	class exported Array {
		uint8* buffer;
		uint32 allocation;
		uint32 furthestWrite;
		
		static const uint32 MINIMUM_SIZE = 32;
		
		void resize(uint32 newSize);
		void recalculateSize(uint32 givenSize);

		public:

			/**
			 * Create an empty Array
			 *
			 * @warning the array is *not* zero-initialized. don't read
			 * past what you write
			 */
			Array(uint32 size = 0);

			/**
			 * Create an Array, copying @a size bytes from @a data
			 */
			Array(const uint8* data, uint32 size);
			
			/**
			 * Create a new Array copying from another array.
			 */
			Array(const Array& other);
			
			/**
			 * Create a new Array from a temporary r-value reference.
			 */
			Array(Array&& other);
			
			/**
			 * Copy an existing Array into this Array.
			 */
			Array& operator=(const Array& other);
			
			/**
			 * Move an existing r-value reference Array into this Array.
			 */
			Array& operator=(Array&& other);

			~Array();
			
			/**
			 * Returns the size of the Array
			 */
			uint32 getSize() const;

			/**
			 * @return a const pointer to the memory area that backs this Array
			 */
			const uint8* getBuffer() const;

			/**
			 * @return a pointer to the memory area that backs this Array
			 */
			uint8* getBuffer();
			
			/**
			 * Resizes the array to be at least @size big
			 */
			void ensureSize(uint32 size);

			/**
			 * @return a pointer offset from the start by @a position
			 */
			const uint8* read(uint32 position, uint32 amount);

			/**
			 * Read @a amount bytes starting at @a position into @a
			 * targetBuffer
			 */
			void readTo(uint32 position, uint32 amount, uint8* targetBuffer);

			/**
			 * Write @a amount bytes from @a data into the array at @a
			 * position.
			 */
			void write(const uint8* data, uint32 position, uint32 amount);
			void write(const int8* data, uint32 position, uint32 amount);

			/**
			 * Write @a amount bytes from @a source into the array at @a position.
			 */
			void write(const Array& source, uint32 position, uint32 amount = 0);
	};
}
