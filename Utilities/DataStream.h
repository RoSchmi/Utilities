#pragma once

#include "Common.h"
#include <string>
#include "Array.h"

namespace Utilities {
	/**
	 * Seakable stream access to a buffer. Offers more safety than
	 * Utilities::Array by disallowing reading past the end of what has been
	 * written.
	 *
	 * Additionally, includes length-prefixed string parsing.
	 */
	class exported DataStream {
		uint32 allocation;
		uint32 cursor;
		uint32 farthestWrite;
		uint8* buffer;
		
		static const uint32 MINIMUM_SIZE = 32;

		void resize(uint32 newSize);
		
		DataStream& operator=(const DataStream& other);

		public:

			/**
			 * Thrown when operations on uninitialized memory would occur
			 */
			class ReadPastEndException {};
		
			DataStream();

			/**
			 * Creates a datastream with the given @a existingBuffer. If @a
			 * copy is true, the data will be copied to a new, internal buffer
			 * @a length bytes long. Assumes the entier buffer is initialized
			 * (ie, ReadPastEndException will not be thrown on all operations
			 * occuring less then @a length bytes from the start
			 */
			DataStream(uint8* exisitingBuffer, uint32 length, bool copy = true);
			DataStream(DataStream&& other);
			DataStream& operator=(DataStream&& other);
			DataStream(const DataStream& other);
			~DataStream();

			/**
			 * @returns read-only reference to internal buffer
			 */
			const uint8* getBuffer() const;
			/**
			 * @returns farthest offset considered initialized
			 */
			uint32 getLength() const;
			/**
			 * @returns true if the cursor is past the end of initialized
			 * memory
			 */
			bool getEOF() const;

			/**
			 * Resets the steam. Sets the cursor to the beginning of the
			 * buffer and consider the entire buffer uninitialized
			 */
			void reset();
			/**
			 * Seek to @a position bytes from the start of the buffer
			 */
			void seek(uint32 position);
			/**
			 * Reinitialize stream to use @a buffer, considering it
			 * initialized up to offset @a length
			 */
			void adopt(uint8* buffer, uint32 length);
			/**
			 * Write @a count bytes from @a bytes to the current location in
			 * the stream, reallocating a larger buffer if need be
			 */
			void write(const uint8* bytes, uint32 count);
			/**
			 * As @write, but using the backing buffer and size from @a array
			 */
			void writeArray(const Utilities::Array& array);
			/**
			 * Writes @a toWrite to the buffer, prefixing the length of the
			 * string. This is the same format used by @a readString.
			 *
			 * @warning The length is only two bytes, maximum string length is
			 * 2^16
			 */
			void writeString(const std::string& toWrite);
			/**
			 * Read @a count bytes, starting at the cursor, into @a buffer.
			 */
			void read(uint8* buffer, uint32 count);
			/**
			 * Read @a count bytes from the stream safely.
			 * @returns a pointer into the buffer
			 */
			uint8* read(uint32 count);
			/**
			 * Create a Utilities::Array from the buffer, starting the cursor,
			 * for @a count bytes.
			 */
			Utilities::Array readArray(uint32 count);
			/**
			 * Read a string from the stream. A string is considered two
			 * bytes, the length, followed by that many bytes of data.
			 */
			std::string readString();

			/**
			 * Write a value of arbitrary type to the buffer. This is
			 * inherrently non-portable past compiler/architecture boundaries,
			 * as it just copies sizeof(T) bytes from a pointer to the passed
			 * value.
			 */
			template <typename T> void write(T toWrite) {
				this->write((uint8*)&toWrite, sizeof(T));
			}

			/**
			 * Read an arbitrary value of (hopefully) type T from the buffer.
			 * This is inherrently non-portable past compiler/architecture
			 * boundaries, as it just casts sizeof(T) bytes to a T.
			 */
			template <typename T> T read() {
				return *(T*)this->read(sizeof(T));
			}
	};
}
