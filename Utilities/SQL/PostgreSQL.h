#pragma once

#include <string>
#include <sstream>

#include <libpq-fe.h>

#include "../Common.h"
#include "Database.h"

namespace util {
	namespace sql {
		namespace postgres {
			class connection;
			template<typename T, typename I> class table_binder;

			class query : public sql::query {
				static const word max_parameters = 25;

				int32 para_index;
				int8* para_values[max_parameters];
				int32 para_lengths[max_parameters];
				int32 para_formats[max_parameters];

				PGresult* result;

				public:
					query(const query& other) = delete;
					query& operator=(const query& other) = delete;
					query& operator=(query&& other) = delete;

					typedef connection connection_type;

					exported query(std::string query_str = "", connection* conn = nullptr);
					exported query(query&& other);
					exported virtual ~query();

					exported void reset_result();
					exported void reset_paras();
					exported virtual void reset() override;
					exported virtual void execute(sql::connection* conn = nullptr) override;
					exported virtual bool is_column_null() const override;

					exported virtual void add_para(const uint8* para, word length) override;
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

					typedef query query_type;
					template<typename T, typename I> using binder_type = table_binder<T, I>;

					exported connection(const parameters& parameters);
					exported virtual ~connection();

					exported query new_query(std::string query_str = "");
					exported query operator<<(std::string query_str);
					exported virtual void begin_transaction(isolation_level level) override;
					exported virtual void rollback_transaction() override;
					exported virtual void commit_transaction() override;
			};

			template<typename T, typename I> class table_binder : public sql::table_binder<T, connection, I> {
				std::string lock_stmt;

				virtual void generate_select_by_id() override {
					this->select_by_id_query.query_str = "SELECT * FROM " + this->name + " WHERE id = $1" + this->lock_stmt;
				}

				virtual void generate_delete() override {
					this->delete_query.query_str = "DELETE FROM " + this->name + " WHERE Id = $1;";
				}

				virtual void generate_insert() override {
					std::stringstream qry;
					qry << "INSERT INTO " << this->name << " (";

					bool isFirst = true;
					for (auto i : this->defs) {
						if (!isFirst)
							qry << ", ";
						else
							isFirst = false;
						qry << i.name;
					}

					qry << ") VALUES (";
					isFirst = true;

					word column_index = 0;
					for (auto i : this->defs) {
						if (!isFirst)
							qry << ", ";
						else
							isFirst = false;
						qry << "$" << ++column_index;
					}

					qry << ");";

					this->insert_query.query_str = qry.str();
				}

				virtual void generate_update() override {
					std::stringstream qry;
					qry << "UPDATE " << this->name << " SET ";

					word column_index = 0;
					bool isFirst = true;
					for (auto i : this->defs) {
						if (i.updatable) {
							if (!isFirst)
								qry << ", ";
							else
								isFirst = false;

							qry << i.name << " = $" << ++column_index;
						}
					}

					qry << " WHERE Id = $" << column_index + 1;

					this->update_query.query_str = qry.str();
				}

				public:
					exported table_binder(postgres::connection& conn, std::string name, bool lock_row = true) : sql::table_binder<T, connection, I>(conn, name) {
						this->lock_stmt = lock_row ? " FOR UPDATE;" : "";
					}

					exported virtual ~table_binder() {

					}

					template<typename U> exported T select_one_by_field(std::string field, U value) {
						query qry = this->db << ("SELECT * FROM " + this->name + " WHERE " + field + " = $1" + this->lock_stmt);
						qry << value;
						return this->fill_one(qry);
					}

					template<typename U> exported std::vector<T> select_by_field(std::string field, U value) {
						query qry = this->db << ("SELECT * FROM " + this->name + " WHERE " + field + " = $1" + this->lock_stmt);
						qry << value;
						return this->fill(qry);
					}
			};
		}
	}
}
