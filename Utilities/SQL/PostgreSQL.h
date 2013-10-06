#pragma once

#include <string>

#include <libpq-fe.h>

#include "../Common.h"
#include "Database.h"

namespace util {
	namespace sql {
		namespace postgres {

			class connection;

			class query : public sql::query {
				static const word MAX_PARAMETERS = 25;

				int32 para_index;
				int8* para_values[MAX_PARAMETERS];
				int32 para_lengths[MAX_PARAMETERS];
				int32 para_formats[MAX_PARAMETERS];

				PGresult* result;

				public:
					query(const query& other) = delete;
					query& operator=(const query& other) = delete;
					query& operator=(query&& other) = delete;

					exported query(std::string query_str = "", connection* conn = nullptr);
					exported query(query&& other);
					exported virtual ~query();

					exported void reset_result();
					exported void reset_paras();
					exported virtual void reset() override;
					exported virtual void execute(sql::connection* conn = nullptr) override;
					exported virtual bool is_column_null() const override;

					exported virtual void add_para(const uint8* para, int32 length) override;
					exported virtual void add_para(const std::string& para) override;
					exported virtual void add_para(const date_time& parameter) override;
					exported virtual void add_para(const data_stream& parameter) override;
					exported virtual void add_para(cstr para) override;
					exported virtual void add_para(float64 para) override;
					exported virtual void add_para(float32 parameter) override;
					exported virtual void add_para(uint64 para) override;
					exported virtual void add_para(uint32 para) override;
					exported virtual void add_para(uint16 para) override;
					exported virtual void add_para(uint8 para) override;
					exported virtual void add_para(int64 para) override;
					exported virtual void add_para(int32 para) override;
					exported virtual void add_para(int16 para) override;
					exported virtual void add_para(int8 para) override;
					exported virtual void add_para(bool para) override;

					exported virtual uint8* get_bytes(uint8* buffer, word count) override;
					exported virtual std::string get_string() override;
					exported virtual date_time get_date_time() override;
					exported virtual data_stream get_data_stream() override;
					exported virtual float64 get_float64() override;
					exported virtual float32 get_float32() override;
					exported virtual uint64 get_uint64() override;
					exported virtual uint32 get_uint32() override;
					exported virtual uint16 get_uint16() override;
					exported virtual uint8 get_uint8() override;
					exported virtual int64 get_int64() override;
					exported virtual int32 get_int32() override;
					exported virtual int16 get_int16() override;
					exported virtual int8 get_int8() override;
					exported virtual bool get_bool() override;

					exported virtual uint8* get_bytes(std::string column, uint8* buffer, word count) override;
					exported virtual std::string get_string(std::string column) override;
					exported virtual date_time get_date_time(std::string column) override;
					exported virtual data_stream get_data_stream(std::string column) override;
					exported virtual float64 get_float64(std::string column) override;
					exported virtual float32 get_float32(std::string column) override;
					exported virtual uint64 get_uint64(std::string column) override;
					exported virtual uint32 get_uint32(std::string column) override;
					exported virtual uint16 get_uint16(std::string column) override;
					exported virtual uint8 get_uint8(std::string column) override;
					exported virtual int64 get_int64(std::string column) override;
					exported virtual int32 get_int32(std::string column) override;
					exported virtual int16 get_int16(std::string column) override;
					exported virtual int8 get_int8(std::string column) override;
					exported virtual bool get_bool(std::string column) override;

					exported virtual uint8* get_bytes(word column, uint8* buffer, word count) override;
					exported virtual std::string get_string(word column) override;
					exported virtual date_time get_date_time(word column) override;
					exported virtual data_stream get_data_stream(word column) override;
					exported virtual float64 get_float64(word column) override;
					exported virtual float32 get_float32(word column) override;
					exported virtual uint64 get_uint64(word column) override;
					exported virtual uint32 get_uint32(word column) override;
					exported virtual uint16 get_uint16(word column) override;
					exported virtual uint8 get_uint8(word column) override;
					exported virtual int64 get_int64(word column) override;
					exported virtual int32 get_int32(word column) override;
					exported virtual int16 get_int16(word column) override;
					exported virtual int8 get_int8(word column) override;
					exported virtual bool get_bool(word column) override;
			};

			class connection : public sql::connection {
				PGconn* base_connection;

				public:
					connection(const connection& other) = delete;
					connection& operator=(const connection& other) = delete;
					connection(connection&& other) = delete;
					connection& operator=(connection&& other) = delete;

					friend class query;

					exported connection(const parameters& parameters);
					exported virtual ~connection();

					exported query new_query(std::string query_str = "");
					exported query operator<<(std::string query_str);
			};
		}
	}
}
