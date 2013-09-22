#pragma once

#include "Common.h"
#include <string>
#include <chrono>

namespace Utilities {
	/**
	 * Seekable stream access to a buffer. Offers more safety than
	 * Utilities::Array by disallowing reading past the end of what has been
	 * written.
	 *
	 * Additionally, includes length-prefixed string parsing.
	 */
	class exported DataStream {
		word allocation;
		word cursor;
		word farthestWrite;
		uint8* buffer;
		
		static const word MINIMUM_SIZE = 32;

		void resize(word newSize);

		public:
			/**
			 * Thrown when operations on uninitialized memory would occur
			 */
			class ReadPastEndException { };

			DataStream();
			DataStream(const uint8* exisitingBuffer, word length);
			DataStream(DataStream&& other);
			DataStream(const DataStream& other);
			~DataStream();

			DataStream& operator=(DataStream&& other);
			DataStream& operator=(const DataStream& other);

			/**
			 * @returns read-only reference to internal buffer
			 */
			const uint8* getBuffer() const;

			/**
			* @returns read-only reference to internal buffer offset by the cursor
			*/
			const uint8* getBufferAtCursor() const;

			/**
			 * @returns farthest offset considered initialized
			 */
			word getLength() const;

			/**
			 * @returns true if the cursor is past the end of initialized
			 * memory
			 */
			bool isEOF() const;

			/**
			 * Resets the steam. Sets the cursor to the beginning of the
			 * buffer and consider the entire buffer uninitialized
			 */
			void reset();

			/**
			 * Seek to @a position bytes from the start of the buffer
			 */
			void seek(word position);

			/**
			 * Reinitialize stream to use @a buffer, considering it
			 * initialized up to offset @a length
			 */
			void adopt(uint8* buffer, word length);

			/**
			 * Write @a count bytes from @a bytes to the current location in
			 * the stream, reallocating a larger buffer if need be
			 */
			void write(const uint8* bytes, word count);

			/**
			* Write @a count bytes from @a bytes to the current location in
			* the stream, reallocating a larger buffer if need be
			*/
			void write(const int8* bytes, word count);

			/**
			* Write the C style string @data as bytes to the current location in
			* the stream, reallocating a larger buffer if need be
			*/
			void write(cstr data);

			/**
			 * Writes the contents of @a toWrite to the buffer
			 */
			void write(const std::string& toWrite);

			/**
			 * Writes the contents of @a toWrite to the buffer
			 */
			void write(const DataStream& toWrite);

			/**
			 * Writes the number of milliseconds since the epoch as a uint64 to the stream.
			 */
			void write(const datetime& toWrite);

			/**
			 * Read @a count bytes, starting at the cursor, into @a buffer.
			 */
			void read(uint8* buffer, uint32 count);

			/**
			* Returns @a count bytes, starting at the cursor.
			*/
			const uint8* read(word count);

			/**
			 * Read a string from the stream. A string is considered two
			 * bytes, the length, followed by that many bytes of data.
			 */
			std::string readString();

			/**
			* Read a DateTime from the stream. Considered a uint64 of milliseconds.
			*/
			datetime readTimePoint();

			/**
			 * Write a value of arbitrary type to the buffer. This is
			 * inherrently non-portable past compiler/architecture boundaries,
			 * as it just copies sizeof(T) bytes from a pointer to the passed
			 * value.
			 */
			template <typename T> void write(T toWrite) {
				this->write(reinterpret_cast<uint8*>(&toWrite), sizeof(T));
			}

			/**
			 * Read an arbitrary value of (hopefully) type T from the buffer.
			 * This is inherrently non-portable past compiler/architecture
			 * boundaries, as it just casts sizeof(T) bytes to a T.
			 */
			template <typename T> T read() {
				return *reinterpret_cast<const T*>(this->read(sizeof(T)));
			}
	};
}
