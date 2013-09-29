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

namespace Utilities {
	namespace SQLDatabase {
		struct IDBObject {
			exported IDBObject();
			exported virtual ~IDBObject();

			exported operator bool();

			uint64 id;
			bool valid;
		};

		class Connection;

		class Exception {
			public:
				std::string what;

				exported Exception(std::string what);
		};

		class Query {
			static const word MAX_PARAMETERS = 25;

			const Connection* parentConnection;
			std::string queryString;
			int32 currentParameterIndex;
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
				exported word execute(const Connection* connection = nullptr);
				exported word getRowCount() const;
				exported bool advanceToNextRow();
				exported bool isCurrentColumnNull() const;

				exported void addParameter(std::string& parameter);
				exported void addParameter(const uint8* parameter, int32 length);
				exported void addParameter(const Utilities::DataStream& parameter);
				exported void addParameter(float64 parameter);
				exported void addParameter(uint64 parameter);
				exported void addParameter(uint32 parameter);
				exported void addParameter(uint16 parameter);
				exported void addParameter(bool parameter);

				exported std::string getString(int32 column);
				exported Utilities::DataStream getDataStream(int32 column);
				exported uint8* getBytes(int32 column, uint8* buffer, word bufferSize);
				exported float64 getFloat64(int32 column);
				exported uint64 getUInt64(int32 column);
				exported uint32 getUInt32(int32 column);
				exported uint16 getUInt16(int32 column);
				exported bool getBool(int32 column);

				exported std::string getString(std::string columnName);
				exported Utilities::DataStream getDataStream(std::string columnName);
				exported uint8* getBytes(std::string columnName, uint8* buffer, word bufferSize);
				exported float64 getFloat64(std::string columnName);
				exported uint64 getUInt64(std::string columnName);
				exported uint32 getUInt32(std::string columnName);
				exported uint16 getUInt16(std::string columnName);
				exported bool getBool(std::string columnName);

				exported std::string getString();
				exported Utilities::DataStream getDataStream();
				exported uint8* getBytes(uint8* buffer, word bufferSize);
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

				exported Connection(std::string host, std::string port, std::string database, std::string username, std::string password);
				exported Connection(const Parameters& parameters);
				exported ~Connection();

				exported bool getIsConnected() const;
				exported Query newQuery(std::string queryString = "") const;
		};
	}
}
