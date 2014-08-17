#pragma once

#include <string>
#include <string>
#include <vector>
#include <type_traits>

#include "../Common.h"
#include "../DataStream.h"

namespace util {
	namespace sql {
		template<typename I> struct object {
			exported object() : id(I()), valid(false) {}
			exported virtual ~object() = 0;

			exported operator bool() { return this->valid; }

			I id;
			bool valid;
		};

		template<typename I> object<I>::~object() = default;

		class db_exception {
			public:
				exported db_exception(std::string message);

				std::string message;
		};

		class synchronization_exception {
			public:
				exported synchronization_exception();
		};

		class connection;
		template<typename T, typename C, typename I> class table_binder;

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

				typedef connection connection_type;

				exported query(std::string query_str = "", connection* conn = nullptr);
				exported query(query&& other);
				exported virtual ~query() = 0;

				exported virtual void reset() = 0;
				exported virtual void execute(connection* conn = nullptr) = 0;
				exported virtual word rows() const;
				exported virtual bool advance_row();
				exported virtual bool is_column_null() const = 0;

				exported virtual void add_para(const uint8* para, word length) = 0;
				exported virtual void add_para(const std::string& para) = 0;
				exported virtual void add_para(const date_time& para) = 0;
				exported virtual void add_para(const data_stream& para) = 0;
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

		class connection {
			protected:
				bool is_connected;
				bool is_committed;

			public:
				struct parameters {
					std::string host;
					std::string port;
					std::string database;
					std::string username;
					std::string password;
				};

				enum class isolation_level {
					read_uncommitted,
					read_committed,
					repeatable_read,
					serializable
				};

				typedef query query_type;
				template<typename T, typename I> using binder_type = table_binder<T, connection, I>;

				exported connection();
				exported virtual ~connection() = 0;

				exported bool connected() const;
				exported bool committed() const;
				exported virtual void begin_transaction(isolation_level level = isolation_level::serializable) = 0;
				exported virtual void rollback_transaction() = 0;
				exported virtual void commit_transaction() = 0;
		};

		template<typename T, typename C, typename I> class table_binder {
			static_assert(std::is_base_of<connection, C>::value && !std::is_same<connection, C>::value, "typename C must derive from, but not be, util::sql::connection.");

			public:
				typedef C connection_type;
				typedef T object_type;
				typedef I id_type;

				class column_definition {
					public:
						enum class data_type {
							uint64,
								uint32,
								uint16,
								uint8,
								int64,
								int32,
								int16,
								int8,
								float64,
								float32,
								boolean,
								string,
								date_time,
								data_stream
						};

						exported column_definition(std::string name, bool updatable, uint64 T::*uint64_type) : name(name), updatable(updatable), type(data_type::uint64) { this->value.uint64_type = uint64_type; };
						exported column_definition(std::string name, bool updatable, uint32 T::*uint32_type) : name(name), updatable(updatable), type(data_type::uint32) { this->value.uint32_type = uint32_type; };
						exported column_definition(std::string name, bool updatable, uint16 T::*uint16_type) : name(name), updatable(updatable), type(data_type::uint16) { this->value.uint16_type = uint16_type; };
						exported column_definition(std::string name, bool updatable, uint8 T::*uint8_type) : name(name), updatable(updatable), type(data_type::uint8) { this->value.uint8_type = uint8_type; };
						exported column_definition(std::string name, bool updatable, int64 T::*int64_type) : name(name), updatable(updatable), type(data_type::int64) { this->value.int64_type = int64_type; };
						exported column_definition(std::string name, bool updatable, int32 T::*int32_type) : name(name), updatable(updatable), type(data_type::int32) { this->value.int32_type = int32_type; };
						exported column_definition(std::string name, bool updatable, int16 T::*int16_type) : name(name), updatable(updatable), type(data_type::int16) { this->value.int16_type = int16_type; };
						exported column_definition(std::string name, bool updatable, int8 T::*int8_type) : name(name), updatable(updatable), type(data_type::int8) { this->value.int8_type = int8_type; };
						exported column_definition(std::string name, bool updatable, float64 T::*float64_type) : name(name), updatable(updatable), type(data_type::float64) { this->value.float64_type = float64_type; };
						exported column_definition(std::string name, bool updatable, float32 T::*float32_type) : name(name), updatable(updatable), type(data_type::float32) { this->value.float32_type = float32_type; };
						exported column_definition(std::string name, bool updatable, bool T::*boolean_type) : name(name), updatable(updatable), type(data_type::boolean) { this->value.boolean_type = boolean_type; };
						exported column_definition(std::string name, bool updatable, std::string T::*string_type) : name(name), updatable(updatable), type(data_type::string) { this->value.string_type = string_type; };
						exported column_definition(std::string name, bool updatable, date_time T::*date_time_type) : name(name), updatable(updatable), type(data_type::date_time) { this->value.date_time_type = date_time_type; };
						exported column_definition(std::string name, bool updatable, util::data_stream T::*binary_type) : name(name), updatable(updatable), type(data_type::data_stream) { this->value.binary_type = binary_type; };

						const std::string name;
						const bool updatable;
						const data_type type;

					private:
						union value_types {
							uint64 T::*uint64_type;
							uint32 T::*uint32_type;
							uint16 T::*uint16_type;
							uint8 T::*uint8_type;
							int64 T::*int64_type;
							int32 T::*int32_type;
							int16 T::*int16_type;
							int8 T::*int8_type;
							float64 T::*float64_type;
							float32 T::*float32_type;
							bool T::*boolean_type;
							std::string T::*string_type;
							date_time T::*date_time_type;
							util::data_stream T::*binary_type;
						} value;

						friend class table_binder<T, C, I>;
				};

				exported table_binder(C& conn, std::string name) : db(conn), select_by_id_query("", &conn), update_query("", &conn), insert_query("", &conn), delete_query("", &conn) {
					this->name = name;
				}

				exported virtual ~table_binder() = 0;

				exported void prepare_stmts() {
					this->generate_select_by_id();
					this->generate_update();
					this->generate_insert();
					this->generate_delete();
				}

				exported void add_def(column_definition def) {
					this->defs.push_back(def);
				}

				exported T select_by_id(I id) {
					this->select_by_id_query.reset();
					this->select_by_id_query.add_para(id);
					return this->fill_one(this->select_by_id_query);
				}

				exported void remove(T& obj) {
					this->delete_query.reset();
					this->delete_query.add_para(obj.id);
					this->delete_query.execute();
				}

				exported void insert(T& obj) {
					this->insert_query.reset();

					for (auto i : this->defs)
						this->add_para(i, this->insert_query, obj);

					this->insert_query.execute();
				}

				exported void update(T& obj) {
					this->update_query.reset();

					for (auto i : this->defs)
						if (i.updatable)
							this->add_para(i, this->update_query, obj);

					this->update_query.add_para(obj.id);
					this->update_query.execute();
				}

				exported T fill_one(typename C::query_type& qry) {
					T result;

					qry.execute();
					if (qry.advance_row()) {
						for (auto i : this->defs)
							this->set_value(i, qry, result);
						result.valid = true;
					}
					else {
						result.valid = false;
					}

					return result;
				}

				exported std::vector<T> fill(typename C::query_type& qry) {
					std::vector<T> result;

					qry.execute();
					while (qry.advance_row()) {
						T current;

						for (auto i : this->defs)
							this->set_value(i, qry, current);

						current.valid = true;
						result.push_back(current);
					}

					return result;
				}

			protected:
				C& db;

				typename C::query_type select_by_id_query;
				typename C::query_type delete_query;
				typename C::query_type insert_query;
				typename C::query_type update_query;

				std::vector<column_definition> defs;
				std::string name;

				void add_para(column_definition& column, typename C::query_type& qry, T& obj) {
					switch (column.type) {
						case column_definition::data_type::uint64: qry.add_para(obj.*(column.value.uint64_type)); break;
						case column_definition::data_type::uint32: qry.add_para(obj.*(column.value.uint32_type)); break;
						case column_definition::data_type::uint16: qry.add_para(obj.*(column.value.uint16_type)); break;
						case column_definition::data_type::uint8: qry.add_para(obj.*(column.value.uint8_type)); break;
						case column_definition::data_type::int64: qry.add_para(obj.*(column.value.int64_type)); break;
						case column_definition::data_type::int32: qry.add_para(obj.*(column.value.int32_type)); break;
						case column_definition::data_type::int16: qry.add_para(obj.*(column.value.int16_type)); break;
						case column_definition::data_type::int8: qry.add_para(obj.*(column.value.int8_type)); break;
						case column_definition::data_type::float64: qry.add_para(obj.*(column.value.float64_type)); break;
						case column_definition::data_type::float32: qry.add_para(obj.*(column.value.float32_type)); break;
						case column_definition::data_type::boolean: qry.add_para(obj.*(column.value.boolean_type)); break;
						case column_definition::data_type::string: qry.add_para(obj.*(column.value.string_type)); break;
						case column_definition::data_type::date_time: qry.add_para(since_epoch(obj.*(column.value.date_time_type))); break;
						case column_definition::data_type::data_stream: qry.add_para(obj.*(column.value.binary_type)); break;
					}
				}

				void set_value(column_definition& column, typename C::query_type& qry, T& obj) {
					switch (column.type) {
						case column_definition::data_type::uint64: obj.*(column.value.uint64_type) = qry.get_uint64(column.name); break;
						case column_definition::data_type::uint32: obj.*(column.value.uint32_type) = qry.get_uint32(column.name); break;
						case column_definition::data_type::uint16: obj.*(column.value.uint16_type) = qry.get_uint16(column.name); break;
						case column_definition::data_type::uint8: obj.*(column.value.uint8_type) = qry.get_uint8(column.name); break;
						case column_definition::data_type::int64: obj.*(column.value.int64_type) = qry.get_int64(column.name); break;
						case column_definition::data_type::int32: obj.*(column.value.int32_type) = qry.get_int32(column.name); break;
						case column_definition::data_type::int16: obj.*(column.value.int16_type) = qry.get_int16(column.name); break;
						case column_definition::data_type::int8: obj.*(column.value.int8_type) = qry.get_int8(column.name); break;
						case column_definition::data_type::float64: obj.*(column.value.float64_type) = qry.get_float64(column.name); break;
						case column_definition::data_type::float32: obj.*(column.value.float32_type) = qry.get_float32(column.name); break;
						case column_definition::data_type::boolean: obj.*(column.value.boolean_type) = qry.get_bool(column.name); break;
						case column_definition::data_type::string: obj.*(column.value.string_type) = qry.get_string(column.name); break;
						case column_definition::data_type::date_time: obj.*(column.value.date_time_type) = from_epoch(qry.get_uint64(column.name)); break;
						case column_definition::data_type::data_stream: obj.*(column.value.binary_type) = qry.get_data_stream(column.name); break;
					}
				}

				virtual void generate_select_by_id() = 0;
				virtual void generate_delete() = 0;
				virtual void generate_insert() = 0;
				virtual void generate_update() = 0;
		};

		template<typename T, typename C, typename I> table_binder<T, C, I>::~table_binder() {}
	}
}
