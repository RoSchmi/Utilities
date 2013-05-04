#pragma once

#include "Common.h"
#include <string>
#include "Array.h"

namespace Utilities {
	class exported DataStream {
		uint32 allocation;
		uint32 cursor;
		uint32 farthestWrite;
		uint8* buffer;
		
		static const uint32 MINIMUM_SIZE = 32;

		void resize(uint32 newSize);
		
		DataStream(const DataStream& other);
		DataStream& operator=(const DataStream& other);

		public:

			class ReadPastEndException {};
		
			DataStream();
			DataStream(uint8* exisitingBuffer, uint32 length, bool copy = true);
			DataStream(DataStream&& other);
			DataStream& operator=(DataStream&& other);
			~DataStream();

			const uint8* getBuffer() const;
			uint32 getLength() const;
			uint32 getEOF() const;

			void reset();
			bool seek(uint32 position);
			void adopt(uint8* buffer, uint32 length);
			void write(const uint8* bytes, uint32 count);
			void write(Utilities::Array& array);
			void write(std::string& toWrite);
			void read(uint8* buffer, uint32 count);
			uint8* read(uint32 count);
			Utilities::Array readArray(uint32 count);
			std::string readString();

			template <typename T> void write(T toWrite) {
				this->write((uint8*)&toWrite, sizeof(T));
			}

			template <typename T> T read() {
				return *(T*)this->read(sizeof(T));
			}
	};
}
