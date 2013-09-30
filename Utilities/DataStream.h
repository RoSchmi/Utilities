#pragma once

#include "Common.h"
#include <string>
#include <chrono>

namespace util {
	/**
	* Seekable stream access to a buffer. Offers more safety than
	* util::Array by disallowing reading past the end of what has been
	* written.
	*
	* Additionally, includes length-prefixed string parsing.
	*/
	class exported data_stream {
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
			class read_past_end_exception { };

			data_stream();
			data_stream(uint8* exisitingBuffer, word length);
			data_stream(const uint8* exisitingBuffer, word length);
			data_stream(data_stream&& other);
			data_stream(const data_stream& other);
			~data_stream();

			data_stream& operator=(data_stream&& other);
			data_stream& operator=(const data_stream& other);

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
			void write(const data_stream& toWrite);

			/**
			* Writes the number of milliseconds since the epoch as a uint64 to the stream.
			*/
			void write(const date_time& toWrite);

			/**
			* Read @a count bytes, starting at the cursor, into @a buffer.
			*/
			void read(uint8* buffer, word count);

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
			date_time readTimePoint();

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

			data_stream& operator<<(cstr rhs);
			data_stream& operator<<(const std::string& rhs);
			data_stream& operator<<(const data_stream& rhs);
			data_stream& operator<<(const date_time& rhs);

			data_stream& operator>>(std::string& rhs);
			data_stream& operator>>(date_time& rhs);

			data_stream& operator<<(bool rhs);
			data_stream& operator<<(float32 rhs);
			data_stream& operator<<(float64 rhs);
			data_stream& operator<<(uint8 rhs);
			data_stream& operator<<(uint16 rhs);
			data_stream& operator<<(uint32 rhs);
			data_stream& operator<<(uint64 rhs);
			data_stream& operator<<(int8 rhs);
			data_stream& operator<<(int16 rhs);
			data_stream& operator<<(int32 rhs);
			data_stream& operator<<(int64 rhs);

			data_stream& operator>>(bool& rhs);
			data_stream& operator>>(float32& rhs);
			data_stream& operator>>(float64& rhs);
			data_stream& operator>>(uint8& rhs);
			data_stream& operator>>(uint16& rhs);
			data_stream& operator>>(uint32& rhs);
			data_stream& operator>>(uint64& rhs);
			data_stream& operator>>(int8& rhs);
			data_stream& operator>>(int16& rhs);
			data_stream& operator>>(int32& rhs);
			data_stream& operator>>(int64& rhs);
	};
}
