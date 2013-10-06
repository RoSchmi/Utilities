#pragma once

#include "../Common.h"
#include "../DataStream.h"

#include <string>
#include <chrono>

namespace util {
	namespace sql {
		template<typename PType = uint64> struct db_object {
			exported db_object();
			exported virtual ~db_object() = 0;

			exported operator bool();

			PType id;
			bool valid;
		};

		class db_exception {
			public:
				exported db_exception(std::string message);

				std::string message;
		};

		class connection {
			protected:
				bool is_connected;

			public:
				struct parameters {
					std::string host;
					std::string port;
					std::string database;
					std::string username;
					std::string password;
				};

				exported virtual ~connection() = 0;

				exported bool connected() const;
		};

		class query {
			protected:
				connection* conn;

				word row_count;
				sword row;
				sword column;
				bool executed;

			public:
				std::string query_str;

				query(const query& other) = delete;
				query& operator=(const query& other) = delete;
				query& operator=(query&& other) = delete;

				exported query(std::string query_str = "", connection* connection = nullptr);
				exported query(query&& other);
				exported virtual ~query() = 0;

				exported virtual void reset() = 0;
				exported virtual void execute(connection* connection = nullptr) = 0;
				exported virtual word rows() const;
				exported virtual bool advance_row();
				exported virtual bool is_column_null() const = 0;

				exported virtual void add_para(const uint8* para, int32 length) = 0;
				exported virtual void add_para(const std::string& para) = 0;
				exported virtual void add_para(const date_time& para);
				exported virtual void add_para(const data_stream& para);
				exported virtual void add_para(cstr para) = 0;
				exported virtual void add_para(float64 para) = 0;
				exported virtual void add_para(float32 para) = 0;
				exported virtual void add_para(uint64 para) = 0;
				exported virtual void add_para(uint32 para) = 0;
				exported virtual void add_para(uint16 para) = 0;
				exported virtual void add_para(uint8 para) = 0;
				exported virtual void add_para(int64 para) = 0;
				exported virtual void add_para(int32 para) = 0;
				exported virtual void add_para(int16 para) = 0;
				exported virtual void add_para(int8 para) = 0;
				exported virtual void add_para(bool para) = 0;

				exported virtual uint8* get_bytes(uint8* buffer, word count) = 0;
				exported virtual std::string get_string() = 0;
				exported virtual date_time get_date_time() = 0;
				exported virtual data_stream get_data_stream() = 0;
				exported virtual float64 get_float64() = 0;
				exported virtual float32 get_float32() = 0;
				exported virtual uint64 get_uint64() = 0;
				exported virtual uint32 get_uint32() = 0;
				exported virtual uint16 get_uint16() = 0;
				exported virtual uint8 get_uint8() = 0;
				exported virtual int64 get_int64() = 0;
				exported virtual int32 get_int32() = 0;
				exported virtual int16 get_int16() = 0;
				exported virtual int8 get_int8() = 0;
				exported virtual bool get_bool() = 0;

				exported virtual uint8* get_bytes(std::string column, uint8* buffer, word count) = 0;
				exported virtual std::string get_string(std::string column) = 0;
				exported virtual date_time get_date_time(std::string column) = 0;
				exported virtual data_stream get_data_stream(std::string column) = 0;
				exported virtual float64 get_float64(std::string column) = 0;
				exported virtual float32 get_float32(std::string column) = 0;
				exported virtual uint64 get_uint64(std::string column) = 0;
				exported virtual uint32 get_uint32(std::string column) = 0;
				exported virtual uint16 get_uint16(std::string column) = 0;
				exported virtual uint8 get_uint8(std::string column) = 0;
				exported virtual int64 get_int64(std::string column) = 0;
				exported virtual int32 get_int32(std::string column) = 0;
				exported virtual int16 get_int16(std::string column) = 0;
				exported virtual int8 get_int8(std::string column) = 0;
				exported virtual bool get_bool(std::string column) = 0;

				exported virtual uint8* get_bytes(word column, uint8* buffer, word count) = 0;
				exported virtual std::string get_string(word column) = 0;
				exported virtual date_time get_date_time(word column) = 0;
				exported virtual data_stream get_data_stream(word column) = 0;
				exported virtual float64 get_float64(word column) = 0;
				exported virtual float32 get_float32(word column) = 0;
				exported virtual uint64 get_uint64(word column) = 0;
				exported virtual uint32 get_uint32(word column) = 0;
				exported virtual uint16 get_uint16(word column) = 0;
				exported virtual uint8 get_uint8(word column) = 0;
				exported virtual int64 get_int64(word column) = 0;
				exported virtual int32 get_int32(word column) = 0;
				exported virtual int16 get_int16(word column) = 0;
				exported virtual int8 get_int8(word column) = 0;
				exported virtual bool get_bool(word column) = 0;

				exported query& operator<<(const std::string& para);
				exported query& operator<<(const date_time& para);
				exported query& operator<<(const data_stream& para);
				exported query& operator<<(cstr para);
				exported query& operator<<(float64 para);
				exported query& operator<<(float32 para);
				exported query& operator<<(uint64 para);
				exported query& operator<<(uint32 para);
				exported query& operator<<(uint16 para);
				exported query& operator<<(uint8 para);
				exported query& operator<<(int64 para);
				exported query& operator<<(int32 para);
				exported query& operator<<(int16 para);
				exported query& operator<<(int8 para);
				exported query& operator<<(bool para);

				exported query& operator>>(std::string& para);
				exported query& operator>>(date_time& para);
				exported query& operator>>(data_stream& para);
				exported query& operator>>(float64& para);
				exported query& operator>>(float32& para);
				exported query& operator>>(uint64& para);
				exported query& operator>>(uint32& para);
				exported query& operator>>(uint16& para);
				exported query& operator>>(uint8& para);
				exported query& operator>>(int64& para);
				exported query& operator>>(int32& para);
				exported query& operator>>(int16& para);
				exported query& operator>>(int8& para);
				exported query& operator>>(bool& para);
		};
	}
}
