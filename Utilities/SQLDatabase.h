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

namespace util {
	namespace sql {
		struct db_object {
			exported db_object();
			exported virtual ~db_object();

			exported operator bool();

			uint64 id;
			bool valid;
		};

		class connection;

		class db_exception {
			public:
				std::string what;

				exported db_exception(std::string what);
		};

		class query {
			static const word MAX_PARAMETERS = 25;

			const connection* parentConnection;
			std::string queryString;
			int32 currentParameterIndex;
			int8* parameterValues[MAX_PARAMETERS];
			int32 parameterLengths[MAX_PARAMETERS];
			int32 parameterFormats[MAX_PARAMETERS];

			PGresult* baseResult;
			int32 rowCount;
			int32 currentRow;
			int32 currentColumn;
			bool executed;

			public:
				query(const query& other) = delete;
				query& operator=(const query& other) = delete;
				query& operator=(query&& other) = delete;

				exported query(std::string query = "", const connection* connection = nullptr);
				exported query(query&& other);
				exported ~query();

				exported void setQueryString(std::string query);
				exported void resetParameters();
				exported void resetResult();
				exported word execute(const connection* connection = nullptr);
				exported word getRowCount() const;
				exported bool advanceToNextRow();
				exported bool isCurrentColumnNull() const;

				exported void addParameter(const std::string& parameter);
				exported void addParameter(const uint8* parameter, int32 length);
				exported void addParameter(const data_stream& parameter);
				exported void addParameter(float64 parameter);
				exported void addParameter(uint64 parameter);
				exported void addParameter(uint32 parameter);
				exported void addParameter(uint16 parameter);
				exported void addParameter(bool parameter);

				exported query& operator<<(const std::string& parameter);
				exported query& operator<<(const data_stream& parameter);
				exported query& operator<<(const date_time& parameter);
				exported query& operator<<(float64 parameter);
				exported query& operator<<(float32 parameter);
				exported query& operator<<(uint64 parameter);
				exported query& operator<<(uint32 parameter);
				exported query& operator<<(uint16 parameter);
				exported query& operator<<(uint8 parameter);
				exported query& operator<<(int64 parameter);
				exported query& operator<<(int32 parameter);
				exported query& operator<<(int16 parameter);
				exported query& operator<<(int8 parameter);
				exported query& operator<<(bool parameter);

				exported query& operator>>(std::string& parameter);
				exported query& operator>>(data_stream& parameter);
				exported query& operator>>(date_time parameter);
				exported query& operator>>(uint64& parameter);
				exported query& operator>>(uint32& parameter);
				exported query& operator>>(uint16& parameter);
				exported query& operator>>(uint8& parameter);
				exported query& operator>>(int64& parameter);
				exported query& operator>>(int32& parameter);
				exported query& operator>>(int16& parameter);
				exported query& operator>>(int8& parameter);
				exported query& operator>>(bool& parameter);

				exported std::string getString(int32 column);
				exported data_stream getDataStream(int32 column);
				exported uint8* getBytes(int32 column, uint8* buffer, word bufferSize);
				exported float64 getFloat64(int32 column);
				exported uint64 getUInt64(int32 column);
				exported uint32 getUInt32(int32 column);
				exported uint16 getUInt16(int32 column);
				exported bool getBool(int32 column);

				exported std::string getString(std::string columnName);
				exported data_stream getDataStream(std::string columnName);
				exported uint8* getBytes(std::string columnName, uint8* buffer, word bufferSize);
				exported float64 getFloat64(std::string columnName);
				exported uint64 getUInt64(std::string columnName);
				exported uint32 getUInt32(std::string columnName);
				exported uint16 getUInt16(std::string columnName);
				exported bool getBool(std::string columnName);

				exported std::string getString();
				exported data_stream getDataStream();
				exported uint8* getBytes(uint8* buffer, word bufferSize);
				exported float64 getFloat64();
				exported uint64 getUInt64();
				exported uint32 getUInt32();
				exported uint16 getUInt16();
				exported bool getBool();
		};

		class connection {
			PGconn* baseConnection;
			bool isConnected;

			public:
				struct parameters {
					std::string host;
					std::string port;
					std::string database;
					std::string username;
					std::string password;
				};

				connection(const connection& other) = delete;
				connection& operator=(const connection& other) = delete;
				connection(connection&& other) = delete;
				connection& operator=(connection&& other) = delete;

				friend class query;

				exported connection(std::string host, std::string port, std::string database, std::string username, std::string password);
				exported connection(const parameters& parameters);
				exported ~connection();

				exported bool getIsConnected() const;
				exported query newQuery(std::string queryString = "") const;

				exported query operator<<(const std::string& parameter);
		};
	}
}
