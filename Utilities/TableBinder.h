#pragma once

#include "Common.h"
#include "DataStream.h"
#include "SQLDatabase.h"

#include <string>
#include <vector>
#include <sstream>
#include <chrono>

namespace util {
	namespace sql {
		template<typename T> class table_binder {
			public:
				class column_definition {
					public:
						enum class data_type {
							uint64,
							uint32,
							uint16,
							float64,
							bool,
							string,
							date_time,
							data_stream
						};

						exported column_definition(std::string name, bool updatable, uint64 T::*uint64Type) : name(name), updatable(updatable), type(data_type::uint64) { this->value.uint64Type = uint64Type; };
						exported column_definition(std::string name, bool updatable, uint32 T::*uint32Type) : name(name), updatable(updatable), type(data_type::uint32) { this->value.uint32Type = uint32Type; };
						exported column_definition(std::string name, bool updatable, uint16 T::*uint16Type) : name(name), updatable(updatable), type(data_type::uint16) { this->value.uint16Type = uint16Type; };
						exported column_definition(std::string name, bool updatable, float64 T::*float64Type) : name(name), updatable(updatable), type(data_type::float64) { this->value.float64Type = float64Type; };
						exported column_definition(std::string name, bool updatable, bool T::*booleanType) : name(name), updatable(updatable), type(data_type::bool) { this->value.booleanType = booleanType; };
						exported column_definition(std::string name, bool updatable, std::string T::*stringType) : name(name), updatable(updatable), type(data_type::string) { this->value.stringType = stringType; };
						exported column_definition(std::string name, bool updatable, date_time T::*dateTimeType) : name(name), updatable(updatable), type(data_type::date_time) { this->value.dateTimeType = dateTimeType; };
						exported column_definition(std::string name, bool updatable, util::data_stream T::*binaryType) : name(name), updatable(updatable), type(data_type::data_stream) { this->value.binaryType = binaryType; };

					private:
						std::string name;
						bool updatable;
						data_type type;

						union value_types {
							uint64 T::*uint64Type;
							uint32 T::*uint32Type;
							uint16 T::*uint16Type;
							float64 T::*float64Type;
							bool T::*booleanType;
							std::string T::*stringType;
							date_time T::*dateTimeType;
							util::data_stream T::*binaryType;
						} value;

						friend class table_binder<T>;
				};

				exported table_binder(std::string name, bool lockSelectedRow) {
					this->name = name;
					this->lockStatement = lockSelectedRow ? " FOR UPDATE;" : "";
				}

				exported void prepateStatements() {
					this->generateSelectById();
					this->generateUpdate();
					this->generateInsert();
					this->generateDelete();
				}

				exported void addColumnDefinition(column_definition column_definition) {
					this->columnDefinitions.push_back(column_definition);
				}

				template<typename U> exported T executeSelectSingleByField(const Connection& db, std::string field, U value) const {
					auto query = db.newQuery("SELECT * FROM " + this->name + " WHERE " + field + " = $1" + this->lockStatement);
					query.addParameter(value);
					return this->fillObjectFromQuery(query);
				}

				template<typename U> exported std::vector<T> executeSelectManyByField(const Connection& db, std::string field, U value) const {
					auto query = db.newQuery("SELECT * FROM " + this->name + " WHERE " + field + " = $1" + this->lockStatement);
					query.addParameter(value);
					return this->fillObjectsFromQuery(query);
				}

				exported T executeSelectById(const Connection& db, uint64 id) const {
					auto query = db.newQuery(this->selectByIdQueryString);
					query.addParameter(id);
					return this->fillObjectFromQuery(query);
				}

				exported void executeDelete(const Connection& db, T& object) const {
					auto query = db.newQuery(this->deleteQueryString);
					query.addParameter(object.id);
					query.execute();
				}

				exported void executeInsert(const Connection& db, T& object) const {
					auto query = db.newQuery(this->insertQueryString);

					for (auto i : this->columnDefinitions)
						this->addQueryParameter(i, query, object);

					query.execute();
				}

				exported void executeUpdate(const Connection& db, T& object) const {
					auto query = db.newQuery(this->updateQueryString);

					for (auto i : this->columnDefinitions)
					if (i.updatable)
						this->addQueryParameter(i, query, object);

					query.addParameter(object.id);
					query.execute();
				}

				exported T fillObjectFromQuery(Query& query) const {
					T result;

					query.execute();
					if (query.advanceToNextRow()) {
						for (auto i : this->columnDefinitions) {
							this->setObjectField(i, query, result);
						}
					}
					else {
						result.valid = false;
					}

					return result;
				}

				exported std::vector<T> fillObjectsFromQuery(Query& query) const {
					std::vector<T> result;
					query.execute();

					while (query.advanceToNextRow()) {
						T current;

						for (auto i : this->columnDefinitions)
							this->setObjectField(i, query, current);

						result.push_back(current);
					}

					return result;
				}

			private:
				std::vector<column_definition> columnDefinitions;
				std::string name;
				std::string lockStatement;

				std::string selectByIdQueryString;
				std::string deleteQueryString;
				std::string insertQueryString;
				std::string updateQueryString;

				void addQueryParameter(column_definition& column, Query& query, T& object) const {
					switch (column.type) {
						case column_definition::data_type::uint64: query.addParameter(object.*(column.value.uint64Type)); break;
						case column_definition::data_type::uint32: query.addParameter(object.*(column.value.uint32Type)); break;
						case column_definition::data_type::uint16: query.addParameter(object.*(column.value.uint16Type)); break;
						case column_definition::data_type::float64: query.addParameter(object.*(column.value.float64Type)); break;
						case column_definition::data_type::bool: query.addParameter(object.*(column.value.booleanType)); break;
						case column_definition::data_type::string: query.addParameter(object.*(column.value.stringType)); break;
						case column_definition::data_type::date_time: query.addParameter(static_cast<uint64>(std::chrono::duration_cast<std::chrono::milliseconds>(epoch - (object.*(column.value.dateTimeType))).count())); break;
						case column_definition::data_type::data_stream: query.addParameter(object.*(column.value.binaryType)); break;
					}
				}

				void setObjectField(column_definition& column, Query& query, T& object) const {
					switch (column.type) {
						case column_definition::data_type::uint64: object.*(column.value.uint64Type) = query.getUInt64(column.name); break;
						case column_definition::data_type::uint32: object.*(column.value.uint32Type) = query.getUInt32(column.name); break;
						case column_definition::data_type::uint16: object.*(column.value.uint16Type) = query.getUInt16(column.name); break;
						case column_definition::data_type::float64: object.*(column.value.float64Type) = query.getFloat64(column.name); break;
						case column_definition::data_type::bool: object.*(column.value.booleanType) = query.getBool(column.name); break;
						case column_definition::data_type::string: object.*(column.value.stringType) = query.getString(column.name); break;
						case column_definition::data_type::date_time: object.*(column.value.dateTimeType) = epoch + std::chrono::milliseconds(query.getUInt64(column.name)); break;
						case column_definition::data_type::data_stream: object.*(column.value.binaryType) = query.getDataStream(column.name); break;
					}
				}

				void generateSelectById() {
					this->selectByIdQueryString = "SELECT * FROM " + this->name + " WHERE Id = $1" + this->lockStatement;
				}

				void generateDelete() {
					this->deleteQueryString = "DELETE FROM " + this->name + " WHERE Id = $1;";
				}

				void generateInsert() {
					std::stringstream query;
					query << "INSERT INTO " << this->name << " (";

					bool isFirst = true;
					for (auto i : this->columnDefinitions) {
						if (!isFirst)
							query << ", ";
						else
							isFirst = false;
						query << i.name;
					}

					query << ") VALUES (";
					isFirst = true;

					word columnIndex = 0;
					for (auto i : this->columnDefinitions) {
						if (!isFirst)
							query << ", ";
						else
							isFirst = false;
						query << "$" << ++columnIndex;
					}

					query << ");";

					this->insertQueryString = query.str();
				}

				void generateUpdate() {
					std::stringstream query;
					query << "UPDATE " << this->name << " SET ";

					word columnIndex = 0;
					bool isFirst = true;
					for (auto i : this->columnDefinitions) {
						if (i.updatable) {
							if (!isFirst)
								query << ", ";
							else
								isFirst = false;
							query << i.name << " = $" << ++columnIndex;
						}
					}

					query << " WHERE Id = $" << columnIndex + 1;

					this->updateQueryString = query.str();
				}
		};
	}
}
