#pragma once

#include "Common.h"
#include "DataStream.h"
#include <string>
#include <vector>
#include <sstream>
#include <chrono>

#ifdef WINDOWS
#include <libpq/libpq-fe.h>
#elif defined POSIX
#include <libpq-fe.h>
#endif

/*
this should probably be changed to an interface with a specific postgres implementation later.
*/

namespace Utilities {
	namespace SQLDatabase {
		class Connection;

		struct Exception {
			std::string what;

			exported Exception(std::string what) : what(what) {}
		};

		class Query {
			static const uint8 MAX_PARAMETERS = 25;

			const Connection* parentConnection;
			std::string queryString;
			uint8 currentParameterIndex;
			int8* parameterValues[MAX_PARAMETERS];
			int32 parameterLengths[MAX_PARAMETERS];
			int32 parameterFormats[MAX_PARAMETERS];

			PGresult* baseResult;
			int32 rowCount;
			int32 currentRow;
			int32 currentColumn;

			public:
				Query(const Query& other) = delete;
				Query& operator=(const Query& other) = delete;
				Query& operator=(Query&& other) = delete;

				exported Query(std::string query = "", const Connection* connection = nullptr);
				exported Query(Query&& other);
				exported ~Query();

				exported void setQueryString(std::string query);
				exported void resetParameters();
				exported void resetResult();
				exported void addParameter(std::string& parameter);
				exported void addParameter(const uint8* parameter, uint32 length);
				exported void addParameter(const Utilities::DataStream& parameter);
				exported void addParameter(float64 parameter);
				exported void addParameter(uint64 parameter);
				exported void addParameter(uint32 parameter);
				exported void addParameter(uint16 parameter);
				exported void addParameter(bool parameter);
				exported uint32 execute(const Connection* connection = nullptr);
				exported uint32 getRowCount() const;
				exported bool advanceToNextRow();
				exported bool isCurrentColumnNull() const;
				exported std::string getString(int32 column);
				exported Utilities::DataStream getDataStream(int32 column);
				exported uint8* getBytes(int32 column, uint8* buffer, uint32 bufferSize);
				exported float64 getFloat64(int32 column);
				exported uint64 getUInt64(int32 column);
				exported uint32 getUInt32(int32 column);
				exported uint16 getUInt16(int32 column);
				exported bool getBool(int32 column);
				exported std::string getString(std::string columnName);
				exported Utilities::DataStream getDataStream(std::string columnName);
				exported uint8* getBytes(std::string columnName, uint8* buffer, uint32 bufferSize);
				exported float64 getFloat64(std::string columnName);
				exported uint64 getUInt64(std::string columnName);
				exported uint32 getUInt32(std::string columnName);
				exported uint16 getUInt16(std::string columnName);
				exported bool getBool(std::string columnName);
				exported std::string getString();
				exported Utilities::DataStream getDataStream();
				exported uint8* getBytes(uint8* buffer, uint32 bufferSize);
				exported float64 getFloat64();
				exported uint64 getUInt64();
				exported uint32 getUInt32();
				exported uint16 getUInt16();
				exported bool getBool();
		};

		class Connection {
			PGconn* baseConnection;
			bool isConnected;

			public:
				struct Parameters {
					std::string host;
					std::string port;
					std::string database;
					std::string username;
					std::string password;
				};

				Connection(const Connection& other) = delete;
				Connection& operator=(const Connection& other) = delete;
				Connection(Connection&& other) = delete;
				Connection& operator=(Connection&& other) = delete;

				friend class Query;

				exported Connection(const int8* host, const int8* port, const int8* database, const int8* username, const int8* password);
				exported Connection(const Parameters& parameters);
				exported ~Connection();
				exported bool getIsConnected() const;
				exported Query newQuery(std::string queryString = "") const;
		};

		struct IDBObject {
			exported IDBObject();
			exported virtual ~IDBObject();

			exported operator bool();

			uint64 id;
			bool valid;
		};

		template<typename T> class TableBinding {
			public:
				class ColumnDefinition {
					public:
						enum class DataType {
							UInt64,
								UInt32,
								UInt16,
								Float64,
								Boolean,
								String,
								DateTime,
								DataStream
						};

						exported ColumnDefinition(std::string name, bool updatable, uint64 T::*uint64Type) : name(name), updatable(updatable), type(DataType::UInt64) { this->value.uint64Type = uint64Type; };
						exported ColumnDefinition(std::string name, bool updatable, uint32 T::*uint32Type) : name(name), updatable(updatable), type(DataType::UInt32) { this->value.uint32Type = uint32Type; };
						exported ColumnDefinition(std::string name, bool updatable, uint16 T::*uint16Type) : name(name), updatable(updatable), type(DataType::UInt16) { this->value.uint16Type = uint16Type; };
						exported ColumnDefinition(std::string name, bool updatable, float64 T::*float64Type) : name(name), updatable(updatable), type(DataType::Float64) { this->value.float64Type = float64Type; };
						exported ColumnDefinition(std::string name, bool updatable, bool T::*booleanType) : name(name), updatable(updatable), type(DataType::Boolean) { this->value.booleanType = booleanType; };
						exported ColumnDefinition(std::string name, bool updatable, std::string T::*stringType) : name(name), updatable(updatable), type(DataType::String) { this->value.stringType = stringType; };
						exported ColumnDefinition(std::string name, bool updatable, datetime T::*dateTimeType) : name(name), updatable(updatable), type(DataType::DateTime) { this->value.dateTimeType = dateTimeType; };
						exported ColumnDefinition(std::string name, bool updatable, Utilities::DataStream T::*binaryType) : name(name), updatable(updatable), type(DataType::DataStream) { this->value.binaryType = binaryType; };

					private:
						std::string name;
						bool updatable;
						DataType type;

						union ValueTypes {
							uint64 T::*uint64Type;
							uint32 T::*uint32Type;
							uint16 T::*uint16Type;
							float64 T::*float64Type;
							bool T::*booleanType;
							std::string T::*stringType;
							datetime T::*dateTimeType;
							Utilities::DataStream T::*binaryType;
						} value;

						friend class TableBinding<T>;
				};

				exported TableBinding(std::string name, bool lockSelectedRow) {
					this->name = name;
					this->lockStatement = lockSelectedRow ? " FOR UPDATE;" : "";
				}

				exported void prepateStatements() {
					this->generateSelectById();
					this->generateUpdate();
					this->generateInsert();
					this->generateDelete();
				}

				exported void addColumnDefinition(ColumnDefinition ColumnDefinition) {
					this->columnDefinitions.push_back(ColumnDefinition);
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
				std::vector<ColumnDefinition> columnDefinitions;
				std::string name;
				std::string lockStatement;

				std::string selectByIdQueryString;
				std::string deleteQueryString;
				std::string insertQueryString;
				std::string updateQueryString;

				void addQueryParameter(ColumnDefinition& column, Query& query, T& object) const {
					switch (column.type) {
						case ColumnDefinition::DataType::UInt64: query.addParameter(object.*(column.value.uint64Type)); break;
						case ColumnDefinition::DataType::UInt32: query.addParameter(object.*(column.value.uint32Type)); break;
						case ColumnDefinition::DataType::UInt16: query.addParameter(object.*(column.value.uint16Type)); break;
						case ColumnDefinition::DataType::Float64: query.addParameter(object.*(column.value.float64Type)); break;
						case ColumnDefinition::DataType::Boolean: query.addParameter(object.*(column.value.booleanType)); break;
						case ColumnDefinition::DataType::String: query.addParameter(object.*(column.value.stringType)); break;
						case ColumnDefinition::DataType::DateTime: query.addParameter(static_cast<uint64>(std::chrono::duration_cast<std::chrono::milliseconds>(epoch - (object.*(column.value.dateTimeType))).count())); break;
						case ColumnDefinition::DataType::DataStream: query.addParameter(object.*(column.value.binaryType)); break;
					}
				}

				void setObjectField(ColumnDefinition& column, Query& query, T& object) const {
					switch (column.type) {
						case ColumnDefinition::DataType::UInt64: object.*(column.value.uint64Type) = query.getUInt64(column.name); break;
						case ColumnDefinition::DataType::UInt32: object.*(column.value.uint32Type) = query.getUInt32(column.name); break;
						case ColumnDefinition::DataType::UInt16: object.*(column.value.uint16Type) = query.getUInt16(column.name); break;
						case ColumnDefinition::DataType::Float64: object.*(column.value.float64Type) = query.getFloat64(column.name); break;
						case ColumnDefinition::DataType::Boolean: object.*(column.value.booleanType) = query.getBool(column.name); break;
						case ColumnDefinition::DataType::String: object.*(column.value.stringType) = query.getString(column.name); break;
						case ColumnDefinition::DataType::DateTime: object.*(column.value.dateTimeType) = epoch + std::chrono::milliseconds(query.getUInt64(column.name)); break;
						case ColumnDefinition::DataType::DataStream: object.*(column.value.binaryType) = query.getDataStream(column.name); break;
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

					uint32 columnIndex = 0;
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

					uint32 columnIndex = 0;
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
