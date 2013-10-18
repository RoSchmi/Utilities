#pragma once

#include <string>
#include <type_traits>

#include "Common.h"

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
		word written;
		uint8* buffer;

		static const word minimum_size = 32;
		static const word growth = 2;
		typedef uint16 string_length_type;

		public:
			/**
			* Thrown when operations on uninitialized memory would occur
			*/
			class read_past_end_exception {};

			/**
			* Thrown when a string read by read_utf8 is not valid utf8.
			*/
			class string_not_utf8 {};

			data_stream();
			data_stream(uint8* data, word length);
			data_stream(const uint8* data, word length);
			data_stream(data_stream&& other);
			data_stream(const data_stream& other);
			~data_stream();

			data_stream& operator=(data_stream&& other);
			data_stream& operator=(const data_stream& other);

			void resize(word size);

			/**
			* @returns read-only reference to internal buffer
			*/
			const uint8* data() const;

			/**
			* @returns read-only reference to internal buffer offset by the cursor
			*/
			const uint8* data_at_cursor() const;

			/**
			* @returns farthest offset that has been written to
			*/
			word size() const;

			/**
			* @returns the position of the cursor
			*/
			word position() const;

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
			void write(const std::string& data);

			/**
			* Writes the contents of @a toWrite to the buffer
			*/
			void write(const data_stream& data);

			/**
			* Writes the number of milliseconds since the epoch as a uint64 to the stream.
			*/
			void write(const date_time& data);

			/**
			* Read @a count bytes, starting at the cursor, into @a buffer.
			*/
			void read(uint8* data, word count);

			/**
			* Returns @a count bytes, starting at the cursor.
			*/
			const uint8* read(word count);

			/**
			* Read a string from the stream. A string is considered two
			* bytes, the length, followed by that many bytes of data.
			*/
			std::string read_string();

			/**
			* Read a string from the stream. A string is considered two
			* bytes, the length, followed by that many bytes of data.
			* Makes sure that the read data is avlid utf-8.
			*/
			std::string read_utf8();

			/**
			* Read a DateTime from the stream. Considered a uint64 of milliseconds.
			*/
			date_time read_date_time();

			/**
			* Write a value of arbitrary type to the buffer. This is
			* inherrently non-portable past compiler/architecture boundaries,
			* as it just copies sizeof(T) bytes from a pointer to the passed
			* value.
			*/
			template <typename T> void write(T data) {
				static_assert(std::is_arithmetic<T>::value, "data_stream::write<T> must be an arithmetic type.");
				this->write(reinterpret_cast<const uint8*>(&data), sizeof(T));
			}

			/**
			* Read an arbitrary value of (hopefully) type T from the buffer.
			* This is inherrently non-portable past compiler/architecture
			* boundaries, as it just casts sizeof(T) bytes to a T.
			*/
			template <typename T> T read() {
				static_assert(std::is_arithmetic<T>::value, "data_stream::read<T> must be an arithmetic type.");
				return *reinterpret_cast<const T*>(this->read(sizeof(T)));
			}

			template<typename T> data_stream& operator<<(T rhs) {
				static_assert(std::is_arithmetic<T>::value, "data_stream::operator<< of T must be an arithmetic type.");
				this->write<T>(rhs);
				return *this;
			}

			template<typename T> data_stream& operator>>(T& rhs) {
				static_assert(std::is_arithmetic<T>::value, "data_stream::operator>> of T must be an arithmetic type.");
				rhs = this->read<T>();
				return *this;
			}

			data_stream& operator<<(cstr rhs);
			data_stream& operator<<(const std::string& rhs);
			data_stream& operator<<(const data_stream& rhs);
			data_stream& operator<<(const date_time& rhs);

			data_stream& operator>>(std::string& rhs);
			data_stream& operator>>(date_time& rhs);
	};
}
