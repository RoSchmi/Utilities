#include "SQLDatabase.h"

#include "Socket.h"

#include <cstring>
#include <stdexcept>
#include <chrono>

using namespace std;
using namespace util;
using namespace util::sql;

db_object::db_object() {
	this->valid = true;
	this->id = 0;
}

db_object::~db_object() {

}

db_object::operator bool() {
	return this->valid;
}

db_exception::db_exception(string what) {
	this->what = what;
}

connection::connection(string host, string port, string database, string username, string password) {
	cstr keywords[] = { "host", "port", "dbname", "user", "password", nullptr };
	cstr values[] = { host.c_str(), port.c_str(), database.c_str(), username.c_str(), password.c_str(), nullptr };
	
	this->baseConnection = PQconnectdbParams(keywords, values, false);

	if (PQstatus(this->baseConnection) != CONNECTION_OK) {
		string error = PQerrorMessage(this->baseConnection);
		PQfinish(this->baseConnection);
		throw db_exception(error);
	}
	else {
		this->isConnected = true;
	}
}

connection::connection(const parameters& parameters) : connection(parameters.host, parameters.port, parameters.database, parameters.username, parameters.password) {

}

connection::~connection() {
	if (this->isConnected)
		PQfinish(this->baseConnection);
}

bool connection::getIsConnected() const {
	return this->isConnected;
}


query connection::newQuery(string queryString) const {
	return query(queryString, this);
}



query::query(string query, const connection* connection) : parentConnection(connection) {
	this->queryString = query;
	this->currentParameterIndex = 0;
	this->baseResult = nullptr;
	this->rowCount = 0;
	this->currentRow = -1;
	this->currentColumn = 0;
}

query::query(query&& other) {
	this->parentConnection = other.parentConnection;
	this->queryString = other.queryString;
	this->currentParameterIndex = other.currentParameterIndex;
	this->baseResult = other.baseResult;
	this->rowCount = other.rowCount;
	this->currentRow = other.currentRow;
	this->currentColumn = other.currentColumn;

	for (int32 i = 0; i < other.currentParameterIndex; i++) {
		this->parameterValues[i] = other.parameterValues[i];
		this->parameterLengths[i] = other.parameterLengths[i];
		this->parameterFormats[i] = other.parameterFormats[i];

		other.parameterValues[i] = nullptr;
		other.parameterLengths[i] = 0;
		other.parameterFormats[i] = 0;
	}

	other.currentParameterIndex = 0;
	other.queryString = "";
	other.parentConnection = nullptr;
	
	other.resetResult();
}

query::~query() {
	this->resetParameters();
	this->resetResult();
}

void query::setQueryString(string query) {
	this->queryString = query;
}

void query::resetParameters() {
	for (uint8 i = 0; i < this->currentParameterIndex; i++)
		delete[] this->parameterValues[i];

	this->currentParameterIndex = 0;
}

void query::resetResult() {
	this->rowCount = 0;
	this->currentColumn = 0;
	this->currentRow = -1;

	if (this->baseResult) {
		PQclear(this->baseResult);
		this->baseResult = nullptr;
	}
}

void query::addParameter(const string& parameter) {
	this->parameterLengths[this->currentParameterIndex] = 0; //ignored for text types
	this->parameterFormats[this->currentParameterIndex] = 0; //text type
	this->parameterValues[this->currentParameterIndex] = new int8[parameter.size() + 1];
	memcpy(this->parameterValues[this->currentParameterIndex], parameter.c_str(), parameter.size() + 1);

	this->currentParameterIndex++;
}

void query::addParameter(const uint8* parameter, int32 length) {
	this->parameterLengths[this->currentParameterIndex] = length; 
	this->parameterFormats[this->currentParameterIndex] = 1; //binary type
	this->parameterValues[this->currentParameterIndex] = new int8[length];
	memcpy(this->parameterValues[this->currentParameterIndex], parameter, length);

	this->currentParameterIndex++;
}

void query::addParameter(const util::data_stream& parameter) {
	this->addParameter(parameter.getBuffer(), static_cast<uint32>(parameter.getLength()));
}

void query::addParameter(float64 parameter) {
	uint64 networkInt = util::net::hostToNetworkInt64(*(uint64*)&parameter);
	this->addParameter((uint8*)&networkInt, sizeof(parameter));
}

void query::addParameter(uint64 parameter) {
	int64 networkparameter = util::net::hostToNetworkInt64(parameter);
	this->addParameter((uint8*)&networkparameter, sizeof(parameter));
}

void query::addParameter(uint32 parameter) {
	int32 networkparameter = util::net::hostToNetworkInt32(parameter);
	this->addParameter((uint8*)&networkparameter, sizeof(parameter));
}

void query::addParameter(uint16 parameter) {
	int16 networkparameter = util::net::hostToNetworkInt16(parameter);
	this->addParameter((uint8*)&networkparameter, sizeof(parameter));
}

void query::addParameter(bool parameter) {
	this->addParameter((uint8*)&parameter, sizeof(parameter));
}

word query::execute(const connection* connection) {
	PGconn* toUse = connection ? connection->baseConnection : this->parentConnection->baseConnection;

	this->resetResult();

	this->baseResult = PQexecParams(toUse, this->queryString.c_str(), this->currentParameterIndex, nullptr, this->parameterValues, this->parameterLengths, this->parameterFormats, 1);
	ExecStatusType resultType = PQresultStatus(this->baseResult);
	if (resultType == PGRES_TUPLES_OK) {
		this->rowCount = PQntuples(this->baseResult);
		this->currentRow = -1;
		this->currentColumn = 0;
	}
	else if (resultType == PGRES_COMMAND_OK) {
		this->rowCount = atoi(PQcmdTuples(this->baseResult));
	}
	else {
		throw runtime_error(PQresultErrorField(this->baseResult, PG_DIAG_SQLSTATE));
	}

	return static_cast<word>(this->rowCount);
}

word query::getRowCount() const {
	return static_cast<word>(this->rowCount);
}

bool query::advanceToNextRow() {
	this->currentRow++;
	this->currentColumn = 0;

	return this->currentRow < this->rowCount;
}

bool query::isCurrentColumnNull() const {
	return PQgetisnull(this->baseResult, this->currentRow, this->currentColumn) == 1;
}

string query::getString(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return "";

	return location;
}

util::data_stream query::getDataStream(int32 column) {
	return data_stream(reinterpret_cast<uint8*>(PQgetvalue(this->baseResult, this->currentRow, column)), PQgetlength(this->baseResult, this->currentRow, column));
}

uint8* query::getBytes(int32 column, uint8* buffer, word bufferSize) {
	int32 length = PQgetlength(this->baseResult, this->currentRow, column);
	int8* temporary = PQgetvalue(this->baseResult, this->currentRow, column);
	memcpy(buffer, temporary, static_cast<int32>(bufferSize) > length ? length : bufferSize);
	return buffer;
}

float64 query::getFloat64(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return 0;

	uint64 hostInt = util::net::networkToHostInt64( *reinterpret_cast<uint64*>(location));
	return *(float64*)&hostInt;
}

uint64 query::getUInt64(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return 0;

	return util::net::networkToHostInt64(*reinterpret_cast<uint64*>(location));
}

uint32 query::getUInt32(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return 0;

	return util::net::networkToHostInt32(*reinterpret_cast<uint32*>(location));
}

uint16 query::getUInt16(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return 0;

	return util::net::networkToHostInt16(*reinterpret_cast<uint16*>(location));
}

bool query::getBool(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return false;

	return (*location) != 0;
}

string query::getString(string columnName) {
	return this->getString(PQfnumber(this->baseResult, columnName.c_str()));
}

util::data_stream query::getDataStream(string columnName) {
	return this->getDataStream(PQfnumber(this->baseResult, columnName.c_str()));
}

uint8* query::getBytes(string columnName, uint8* buffer, word bufferSize) {
	return this->getBytes(PQfnumber(this->baseResult, columnName.c_str()), buffer, bufferSize);
}

float64 query::getFloat64(string columnName) {
	return this->getFloat64(PQfnumber(this->baseResult, columnName.c_str()));
}

uint64 query::getUInt64(string columnName) {
	return this->getUInt64(PQfnumber(this->baseResult, columnName.c_str()));
}

uint32 query::getUInt32(string columnName) {
	return this->getUInt32(PQfnumber(this->baseResult, columnName.c_str()));
}

uint16 query::getUInt16(string columnName) {
	return this->getUInt16(PQfnumber(this->baseResult, columnName.c_str()));
}

bool query::getBool(string columnName) {
	return this->getBool(PQfnumber(this->baseResult, columnName.c_str()));
}

string query::getString() {
	return this->getString(this->currentColumn++);
}

util::data_stream query::getDataStream() {
	return this->getDataStream(this->currentColumn++);
}

uint8* query::getBytes(uint8* buffer, word bufferSize) {
	return this->getBytes(this->currentColumn++, buffer, bufferSize);
}

float64 query::getFloat64() {
	return this->getFloat64(this->currentColumn++);
}

uint64 query::getUInt64() {
	return this->getUInt64(this->currentColumn++);
}

uint32 query::getUInt32() {
	return this->getUInt32(this->currentColumn++);
}

uint16 query::getUInt16() {
	return this->getUInt16(this->currentColumn++);
}

bool query::getBool() {
	return this->getBool(this->currentColumn++);
}

query& query::operator<<(const string& parameter) {
	this->addParameter(parameter);
	return *this;
}

query& query::operator<<(const data_stream& parameter) {
	this->addParameter(parameter);
	return *this;
}

query& query::operator<<(const date_time& parameter) {
	this->addParameter(static_cast<uint64>(chrono::duration_cast<chrono::milliseconds>(epoch - parameter).count()));
	return *this;
}

query& query::operator<<(float64 parameter) {
	this->addParameter(parameter);
	return *this;
}

query& query::operator<<(float32 parameter) {
	this->addParameter(parameter);
	return *this;
}

query& query::operator<<(uint64 parameter) {
	this->addParameter(parameter);
	return *this;
}

query& query::operator<<(uint32 parameter) {
	this->addParameter(parameter);
	return *this;
}

query& query::operator<<(uint16 parameter) {
	this->addParameter(parameter);
	return *this;
}

query& query::operator<<(uint8 parameter) {
	this->addParameter(static_cast<uint16>(parameter));
	return *this;
}

query& query::operator<<(int64 parameter) {
	this->addParameter(static_cast<uint64>(parameter));
	return *this;
}

query& query::operator<<(int32 parameter) {
	this->addParameter(static_cast<uint32>(parameter));
	return *this;
}

query& query::operator<<(int16 parameter) {
	this->addParameter(static_cast<uint16>(parameter));
	return *this;
}

query& query::operator<<(int8 parameter) {
	this->addParameter(static_cast<uint16>(parameter));
	return *this;
}

query& query::operator<<(bool parameter) {
	this->addParameter(parameter);
	return *this;
}

query& query::operator>>(string& parameter) {
	parameter = this->getString();
	return *this;
}

query& query::operator>>(data_stream& parameter) {
	parameter = this->getDataStream();
	return *this;
}

query& query::operator>>(date_time parameter) {
	parameter = epoch + chrono::milliseconds(this->getUInt64());
	return *this;
}

query& query::operator>>(uint64& parameter) {
	parameter = this->getUInt64();
	return *this;
}

query& query::operator>>(uint32& parameter) {
	parameter = this->getUInt32();
	return *this;
}

query& query::operator>>(uint16& parameter) {
	parameter = this->getUInt16();
	return *this;
}

query& query::operator>>(uint8& parameter) {
	parameter = static_cast<uint8>(this->getUInt16());
	return *this;
}

query& query::operator>>(int64& parameter) {
	parameter = static_cast<int64>(this->getUInt64());
	return *this;
}

query& query::operator>>(int32& parameter) {
	parameter = static_cast<int32>(this->getUInt32());
	return *this;
}

query& query::operator>>(int16& parameter) {
	parameter = static_cast<int16>(this->getUInt16());
	return *this;
}

query& query::operator>>(int8& parameter) {
	parameter = static_cast<int8>(this->getUInt16());
	return *this;
}

query& query::operator>>(bool& parameter) {
	parameter = static_cast<bool>(this->getBool());
	return *this;
}
