#pragma once

#include "../Common.h"
#include "../DataStream.h"
#include "Database.h"
#include "PostgreSQL.h"

#include <string>
#include <vector>
#include <sstream>
#include <chrono>

namespace util {
	namespace sql {
		template<typename T, typename C = postgres::connection, typename Q = postgres::query, typename PType = uint64, typename PName = "id"> class table_binder {
			public:
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
							bool,
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
						exported column_definition(std::string name, bool updatable, bool T::*boolean_type) : name(name), updatable(updatable), type(data_type::bool) { this->value.boolean_type = boolean_type; };
						exported column_definition(std::string name, bool updatable, std::string T::*string_type) : name(name), updatable(updatable), type(data_type::string) { this->value.string_type = string_type; };
						exported column_definition(std::string name, bool updatable, date_time T::*dateTime_type) : name(name), updatable(updatable), type(data_type::date_time) { this->value.dateTime_type = dateTime_type; };
						exported column_definition(std::string name, bool updatable, util::data_stream T::*binary_type) : name(name), updatable(updatable), type(data_type::data_stream) { this->value.binary_type = binary_type; };

					private:
						std::string name;
						bool updatable;
						data_type type;

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
							date_time T::*dateTime_type;
							util::data_stream T::*binary_type;
						} value;

						friend class table_binder<T, C, Q>;
				};

				exported table_binder(C& conn, std::string name, bool lock_row = true) : db(conn), select_by_id_query("", &conn), update_query("", &conn), insert_query("", &conn), delete_query("", &conn) {
					this->name = name;
					this->lock_stmt = lock_row ? " FOR UPDATE;" : "";
				}

				exported void prepare_stmts() {
					this->generate_select_by_id();
					this->generate_update ();
					this->generate_insert();
					this->generate_delete();
				}

				exported void add_def(column_definition def) {
					this->defs.push_back(def);
				}

				template<typename U> exported T select_one_by_field(std::string field, U value) {
					auto query = Q("SELECT * FROM " + this->name + " WHERE " + field + " = $1" + this->lock_stmt, &this->db);
					query.add_para(value);
					return this->fill_one(query);
				}

				template<typename U> exported std::vector<T> select_by_field(std::string field, U value) {
					auto query = Q("SELECT * FROM " + this->name + " WHERE " + field + " = $1" + this->lock_stmt, &this->db);
					query.add_para(value);
					return this->fill(query);
				}

				exported T select_by_id(connection& db, PType id) {
					this->select_by_id_query.reset();
					this->select_by_id_query.add_para(id);
					return this->fill_one(this->select_by_id_query);
				}

				exported void remove(connection& db, T& object) {
					this->delete_query.reset();
					this->delete_query.add_para(id);
					this->delete_query.execute();
				}

				exported void insert(connection& db, T& object) {
					this->insert_query.reset();

					for (auto i : this->defs)
						this->add_para(i, this->insert_query, object);

					this->insert_query.execute();
				}

				exported void update(connection& db, T& object) {
					this->update_query.reset();

					for (auto i : this->defs)
						if (i.updatable)
							this->add_para(i, this->update_query, object);

					this->update_query.add_para(object.id);
					this->update_query.execute();
				}

				exported T fill_one(Q& query) {
					T result;

					query.execute();
					if (query.advance_row()) {
						for (auto i : this->defs) {
							this->set_value(i, query, result);
						}
					}
					else {
						result.valid = false;
					}

					return result;
				}

				exported std::vector<T> fill(Q& query) {
					std::vector<T> result;

					query.execute();
					while (query.advance_row()) {
						T current;

						for (auto i : this->defs)
							this->set_value(i, query, current);

						result.push_back(current);
					}

					return result;
				}

			private:
				C& db;

				Q select_by_id_query;
				Q delete_query;
				Q insert_query;
				Q update_query;

				std::vector<column_definition> defs;
				std::string name;
				std::string lock_stmt;

				void add_para(column_definition& column, Q& query, T& object) {
					switch (column.type) {
						case column_definition::data_type::uint64: query.add_para(object.*(column.value.uint64_type)); break;
						case column_definition::data_type::uint32: query.add_para(object.*(column.value.uint32_type)); break;
						case column_definition::data_type::uint16: query.add_para(object.*(column.value.uint16_type)); break;
						case column_definition::data_type::uint8: query.add_para(object.*(column.value.uint8_type)); break;
						case column_definition::data_type::int64: query.add_para(object.*(column.value.int64_type)); break;
						case column_definition::data_type::int32: query.add_para(object.*(column.value.int32_type)); break;
						case column_definition::data_type::int16: query.add_para(object.*(column.value.int16_type)); break;
						case column_definition::data_type::int8: query.add_para(object.*(column.value.int8_type)); break;
						case column_definition::data_type::float64: query.add_para(object.*(column.value.float64_type)); break;
						case column_definition::data_type::float32: query.add_para(object.*(column.value.float32_type)); break;
						case column_definition::data_type::bool: query.add_para(object.*(column.value.boolean_type)); break;
						case column_definition::data_type::string: query.add_para(object.*(column.value.string_type)); break;
						case column_definition::data_type::date_time: query.add_para(since_epoch(object.*(column.value.dateTime_type))); break;
						case column_definition::data_type::data_stream: query.add_para(object.*(column.value.binary_type)); break;
					}
				}

				void set_value(column_definition& column, Q& query, T& object) {
					switch (column.type) {
						case column_definition::data_type::uint64: object.*(column.value.uint64_type) = query.get_uint64(column.name); break;
						case column_definition::data_type::uint32: object.*(column.value.uint32_type) = query.get_uint32(column.name); break;
						case column_definition::data_type::uint16: object.*(column.value.uint16_type) = query.get_uint16(column.name); break;
						case column_definition::data_type::uint8: object.*(column.value.uint8_type) = query.get_uint8(column.name); break;
						case column_definition::data_type::int64: object.*(column.value.int64_type) = query.get_int64(column.name); break;
						case column_definition::data_type::int32: object.*(column.value.int32_type) = query.get_int32(column.name); break;
						case column_definition::data_type::int16: object.*(column.value.int16_type) = query.get_int16(column.name); break;
						case column_definition::data_type::int8: object.*(column.value.int8_type) = query.get_int8(column.name); break;
						case column_definition::data_type::float64: object.*(column.value.float64_type) = query.get_float64(column.name); break;
						case column_definition::data_type::float32: object.*(column.value.float32_type) = query.get_float32(column.name); break;
						case column_definition::data_type::bool: object.*(column.value.boolean_type) = query.get_bool(column.name); break;
						case column_definition::data_type::string: object.*(column.value.string_type) = query.get_string(column.name); break;
						case column_definition::data_type::date_time: object.*(column.value.dateTime_type) = from_epoch(query.get_uint64(column.name)); break;
						case column_definition::data_type::data_stream: object.*(column.value.binary_type) = query.get_data_stream(column.name); break;
					}
				}

				void generate_select_by_id() {
					this->select_by_id_query.query_str = "SELECT * FROM " + this->name + " WHERE " + PName + " = $1" + this->lock_stmt;
				}

				void generate_delete() {
					this->delete_query.query_str = "DELETE FROM " + this->name + " WHERE Id = $1;";
				}

				void generate_insert() {
					std::stringstream query;
					query << "INSERT INTO " << this->name << " (";

					bool isFirst = true;
					for (auto i : this->defs) {
						if (!isFirst)
							query << ", ";
						else
							isFirst = false;
						query << i.name;
					}

					query << ") VALUES (";
					isFirst = true;

					word columnIndex = 0;
					for (auto i : this->defs) {
						if (!isFirst)
							query << ", ";
						else
							isFirst = false;
						query << "$" << ++columnIndex;
					}

					query << ");";

					this->insert_query.query_str = query.str();
				}

				void generate_update() {
					std::stringstream query;
					query << "UPDATE " << this->name << " SET ";

					word columnIndex = 0;
					bool isFirst = true;
					for (auto i : this->defs) {
						if (i.updatable) {
							if (!isFirst)
								query << ", ";
							else
								isFirst = false;

							query << i.name << " = $" << ++columnIndex;
						}
					}

					query << " WHERE Id = $" << columnIndex + 1;

					this->update_query.query_str = query.str();
				}
		};
	}
}
