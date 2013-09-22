#include "SQLDatabase.h"
#include "Socket.h"
#include <cstring>

using namespace Utilities::SQLDatabase;

Connection::Connection(const int8* host, const int8* port, const int8* database, const int8* username, const int8* password) {
	const int8* keywords[] = {"host", "port", "dbname", "user", "password", nullptr};
	const int8* values[] = {host, port, database, username, password, nullptr};
	
	this->baseConnection = PQconnectdbParams(keywords, values, false);

	if (PQstatus(this->baseConnection) != CONNECTION_OK) {
		std::string error = PQerrorMessage(this->baseConnection);
		PQfinish(this->baseConnection);
		throw Exception(error);
	}
	else {
		this->isConnected = true;
	}
}

Connection::Connection(const Parameters& parameters) : Connection(parameters.host.c_str(), parameters.port.c_str(), parameters.database.c_str(), parameters.username.c_str(), parameters.password.c_str()) {

}

Connection::~Connection() {
	if (this->isConnected)
		PQfinish(this->baseConnection);
}

bool Connection::getIsConnected() const {
	return this->isConnected;
}


Query Connection::newQuery(std::string queryString) const {
	return Query(queryString, this);
}



Query::Query(std::string query, const Connection* connection) : parentConnection(connection) {
	this->queryString = query;
	this->currentParameterIndex = 0;
	this->baseResult = nullptr;
	this->rowCount = 0;
	this->currentRow = -1;
	this->currentColumn = 0;
}

Query::Query(Query&& other) {
	this->parentConnection = other.parentConnection;
	this->queryString = other.queryString;
	this->currentParameterIndex = other.currentParameterIndex;
	this->baseResult = other.baseResult;
	this->rowCount = other.rowCount;
	this->currentRow = other.currentRow;
	this->currentColumn = other.currentColumn;

	for (uint8 i = 0; i < other.currentParameterIndex; i++) {
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

Query::~Query() {
	this->resetParameters();
	this->resetResult();
}

void Query::setQueryString(std::string query) {
	this->queryString = query;
}

void Query::resetParameters() {
	for (uint8 i = 0; i < this->currentParameterIndex; i++)
		delete[] this->parameterValues[i];

	this->currentParameterIndex = 0;
}

void Query::resetResult() {
	this->rowCount = 0;
	this->currentColumn = 0;
	this->currentRow = -1;

	if (this->baseResult) {
		PQclear(this->baseResult);
		this->baseResult = nullptr;
	}
}

void Query::addParameter(std::string& parameter) {
	this->parameterLengths[this->currentParameterIndex] = 0; //ignored for text types
	this->parameterFormats[this->currentParameterIndex] = 0; //text type
	this->parameterValues[this->currentParameterIndex] = new int8[parameter.size() + 1];
	memcpy(this->parameterValues[this->currentParameterIndex], parameter.c_str(), parameter.size() + 1);

	this->currentParameterIndex++;
}

void Query::addParameter(const uint8* parameter, uint32 length) {
	this->parameterLengths[this->currentParameterIndex] = length; 
	this->parameterFormats[this->currentParameterIndex] = 1; //binary type
	this->parameterValues[this->currentParameterIndex] = new int8[length];
	memcpy(this->parameterValues[this->currentParameterIndex], parameter, length);

	this->currentParameterIndex++;
}

void Query::addParameter(const Utilities::DataStream& parameter) {
	this->addParameter(parameter.getBuffer(), static_cast<uint32>(parameter.getLength()));
}

void Query::addParameter(float64 parameter) {
	uint64 networkInt = Utilities::Net::hostToNetworkInt64(*(uint64*)&parameter);
	this->addParameter((uint8*)&networkInt, sizeof(parameter));
}

void Query::addParameter(uint64 parameter) {
	int64 networkparameter = Utilities::Net::hostToNetworkInt64(parameter);
	this->addParameter((uint8*)&networkparameter, sizeof(parameter));
}

void Query::addParameter(uint32 parameter) {
	int32 networkparameter = Utilities::Net::hostToNetworkInt32(parameter);
	this->addParameter((uint8*)&networkparameter, sizeof(parameter));
}

void Query::addParameter(uint16 parameter) {
	int16 networkparameter = Utilities::Net::hostToNetworkInt16(parameter);
	this->addParameter((uint8*)&networkparameter, sizeof(parameter));
}

void Query::addParameter(bool parameter) {
	this->addParameter((uint8*)&parameter, sizeof(parameter));
}

uint32 Query::execute(const Connection* connection) {
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
		throw Exception(PQresultErrorField(this->baseResult, PG_DIAG_SQLSTATE));
	}

	return this->rowCount;
}

uint32 Query::getRowCount() const {
	return this->rowCount;
}

bool Query::advanceToNextRow() {
	this->currentRow++;
	this->currentColumn = 0;

	return this->currentRow < this->rowCount;
}

bool Query::isCurrentColumnNull() const {
	return PQgetisnull(this->baseResult, this->currentRow, this->currentColumn) == 1;
}

std::string Query::getString(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return "";

	return location;
}

Utilities::DataStream Query::getDataStream(int32 column) {
	return DataStream(reinterpret_cast<uint8*>(PQgetvalue(this->baseResult, this->currentRow, column)), PQgetlength(this->baseResult, this->currentRow, column));
}

uint8* Query::getBytes(int32 column, uint8* buffer, uint32 bufferSize) {
	uint32 length = static_cast<uint32>(PQgetlength(this->baseResult, this->currentRow, column));
	int8* temporary = PQgetvalue(this->baseResult, this->currentRow, column);
	memcpy(buffer, temporary, bufferSize > length ? length : bufferSize);
	return buffer;
}

float64 Query::getFloat64(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return 0;

	uint64 hostInt = Utilities::Net::networkToHostInt64( *(uint64*)location );
	return *(float64*)&hostInt;
}

uint64 Query::getUInt64(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return 0;

	return Utilities::Net::networkToHostInt64( *(uint64*)location );
}

uint32 Query::getUInt32(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return 0;

	return Utilities::Net::networkToHostInt32( *(uint32*)location );
}

uint16 Query::getUInt16(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return 0;

	return Utilities::Net::networkToHostInt16( *(uint16*)location );
}

bool Query::getBool(int32 column) {
	int8* location = PQgetvalue(this->baseResult, this->currentRow, column);
	if (!location) return false;

	return (*location) != 0;
}

std::string Query::getString(std::string columnName) {
	return this->getString(PQfnumber(this->baseResult, columnName.c_str()));
}

Utilities::DataStream Query::getDataStream(std::string columnName) {
	return this->getDataStream(PQfnumber(this->baseResult, columnName.c_str()));
}

uint8* Query::getBytes(std::string columnName, uint8* buffer, uint32 bufferSize) {
	return this->getBytes(PQfnumber(this->baseResult, columnName.c_str()), buffer, bufferSize);
}

float64 Query::getFloat64(std::string columnName) {
	return this->getFloat64(PQfnumber(this->baseResult, columnName.c_str()));
}

uint64 Query::getUInt64(std::string columnName) {
	return this->getUInt64(PQfnumber(this->baseResult, columnName.c_str()));
}

uint32 Query::getUInt32(std::string columnName) {
	return this->getUInt32(PQfnumber(this->baseResult, columnName.c_str()));
}

uint16 Query::getUInt16(std::string columnName) {
	return this->getUInt16(PQfnumber(this->baseResult, columnName.c_str()));
}

bool Query::getBool(std::string columnName) {
	return this->getBool(PQfnumber(this->baseResult, columnName.c_str()));
}

std::string Query::getString() {
	return this->getString(this->currentColumn++);
}

Utilities::DataStream Query::getDataStream() {
	return this->getDataStream(this->currentColumn++);
}

uint8* Query::getBytes(uint8* buffer, uint32 bufferSize) {
	return this->getBytes(this->currentColumn++, buffer, bufferSize);
}

float64 Query::getFloat64() {
	return this->getFloat64(this->currentColumn++);
}

uint64 Query::getUInt64() {
	return this->getUInt64(this->currentColumn++);
}

uint32 Query::getUInt32() {
	return this->getUInt32(this->currentColumn++);
}

uint16 Query::getUInt16() {
	return this->getUInt16(this->currentColumn++);
}

bool Query::getBool() {
	return this->getBool(this->currentColumn++);
}

IDBObject::IDBObject() {
	this->valid = true;
	this->id = 0;
}

IDBObject::~IDBObject() {

}

IDBObject::operator bool() {
	return this->valid;
}