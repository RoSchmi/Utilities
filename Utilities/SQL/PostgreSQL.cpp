#include "PostgreSQL.h"

#include "../Net/Socket.h"

#include <cstring>
#include <utility>

using namespace std;
using namespace util;
using namespace util::sql::postgres;

connection::connection(const sql::connection::parameters& paras) {
	cstr keywords[] = { "host", "port", "dbname", "user", "password", nullptr };
	cstr values[] = { paras.host.c_str(), paras.port.c_str(), paras.database.c_str(), paras.username.c_str(), paras.password.c_str(), nullptr };

	this->base_connection = PQconnectdbParams(keywords, values, false);

	if (PQstatus(this->base_connection) != CONNECTION_OK) {
		string error = PQerrorMessage(this->base_connection);
		PQfinish(this->base_connection);
		throw db_exception(error);
	}
	else {
		this->is_connected = true;
	}
}

connection::~connection() {
	if (this->is_connected)
		PQfinish(this->base_connection);
}

query connection::new_query(string query_str) {
	return query(query_str, this);
}

query connection::operator<<(string query_str) {
	return this->new_query(query_str);
}

query::query(string query_str, connection* conn) : sql::query(query_str, conn) {
	this->para_index = 0;
	this->result = nullptr;
}

query::query(query&& other) : sql::query(move(other)) {
	this->para_index = other.para_index;
	this->result = other.result;

	for (word i = 0; i < static_cast<word>(other.para_index); i++) {
		this->para_values[i] = other.para_values[i];
		this->para_lengths[i] = other.para_lengths[i];
		this->para_formats[i] = other.para_formats[i];

		other.para_values[i] = nullptr;
		other.para_lengths[i] = 0;
		other.para_formats[i] = 0;
	}

	other.para_index = 0;
	
	other.reset_result();
}

query::~query() {
	this->reset_paras();
	this->reset_result();
}

void query::reset() {
	this->reset_paras();
	this->reset_result();
}

void query::reset_paras() {
	for (uint8 i = 0; i < this->para_index; i++)
		delete[] this->para_values[i];

	this->para_index = 0;
}

void query::reset_result() {
	this->row_count  = 0;
	this->column = 0;
	this->row = -1;

	if (this->result) {
		PQclear(this->result);
		this->result = nullptr;
	}
}

void query::execute(sql::connection* conn) {
	PGconn* toUse = reinterpret_cast<connection*>(conn ? conn : this->conn)->base_connection;

	this->reset_result();

	this->result = PQexecParams(toUse, this->query_str.c_str(), this->para_index, nullptr, this->para_values, this->para_lengths, this->para_formats, 1);
	ExecStatusType resultType = PQresultStatus(this->result);
	if (resultType == PGRES_TUPLES_OK) {
		this->row_count = PQntuples(this->result);
		this->row = -1;
		this->column = 0;
		this->executed = true;
	}
	else if (resultType == PGRES_COMMAND_OK) {
		this->row_count = atoi(PQcmdTuples(this->result));
	}
	else {
		throw db_exception(PQresultErrorField(this->result, PG_DIAG_SQLSTATE));
	}
}

bool query::is_column_null() const {
	return PQgetisnull(this->result, static_cast<const int>(this->row), static_cast<const int>(this->column)) == 1;
}

void query::add_para(const uint8* para, int32 length) {
	this->para_lengths[this->para_index] = length; 
	this->para_formats[this->para_index] = 1; //binary type
	this->para_values[this->para_index] = new int8[length];
	memcpy(this->para_values[this->para_index], para, length);

	this->para_index++;
}

void query::add_para(const string& para) {
	this->para_lengths[this->para_index] = 0; //ignored for text types
	this->para_formats[this->para_index] = 0; //text type
	this->para_values[this->para_index] = new int8[para.size() + 1];
	memcpy(this->para_values[this->para_index], para.c_str(), para.size() + 1);

	this->para_index++;
}

void query::add_para(const date_time& para) {
	this->add_para(since_epoch(para));
}

void query::add_para(float64 para) {
	uint64 networkInt = util::net::host_to_net_int64(*(uint64*)&para);
	this->add_para((uint8*)&networkInt, sizeof(para));
}

void query::add_para(float32 para) {
	this->add_para(static_cast<float64>(para));
}

void query::add_para(uint64 para) {
	int64 network_para = net::host_to_net_int64(para);
	this->add_para(reinterpret_cast<uint8*>(&network_para), sizeof(para));
}

void query::add_para(uint32 para) {
	int32 network_para = net::host_to_net_int32(para);
	this->add_para(reinterpret_cast<uint8*>(&network_para), sizeof(para));
}

void query::add_para(uint16 para) {
	int16 network_para = net::host_to_net_int16(para);
	this->add_para(reinterpret_cast<uint8*>(&network_para), sizeof(para));
}

void query::add_para(uint8 para) {
	this->add_para(static_cast<uint8>(para));
}

void query::add_para(int64 para) {
	this->add_para(static_cast<int64>(para));
}

void query::add_para(int32 para) {
	this->add_para(static_cast<int32>(para));
}

void query::add_para(int16 para) {
	this->add_para(static_cast<int16>(para));
}

void query::add_para(int8 para) {
	this->add_para(static_cast<int8>(para));
}

void query::add_para(bool para) {
	this->add_para((uint8*)&para, sizeof(para));
}

uint8* query::get_bytes(word column, uint8* buffer, word count) {
	if (!this->executed) { this->execute(); this->advance_row(); }
	int length = PQgetlength(this->result, static_cast<int>(this->row), static_cast<int>(column));
	char* temporary = PQgetvalue(this->result, static_cast<int>(this->row), static_cast<int>(column));
	memcpy(buffer, temporary, count > static_cast<word>(length) ? static_cast<word>(length) : count);
	return buffer;
}

string query::get_string(word column) {
	if (!this->executed) { this->execute(); this->advance_row(); }
	char* location = PQgetvalue(this->result, static_cast<int>(this->row), static_cast<int>(column));
	if (!location) return "";

	return location;
}

date_time query::get_date_time(word column) {
	return from_epoch(this->get_uint64(column));
}

data_stream query::get_data_stream(word column) {
	if (!this->executed) { this->execute(); this->advance_row(); }
	int length = PQgetlength(this->result, static_cast<int>(this->row), static_cast<int>(column));
	const char* location = PQgetvalue(this->result, static_cast<int>(this->row), static_cast<int>(column));
	if (location && length > 0)
		return data_stream(reinterpret_cast<const uint8*>(location), length);
	else
		return data_stream();
}

float64 query::get_float64(word column) {
	if (!this->executed) { this->execute(); this->advance_row(); }
	char* location = PQgetvalue(this->result, static_cast<int>(this->row), static_cast<int>(column));
	if (!location) return 0;

	uint64 hostInt = util::net::net_to_host_int64(*reinterpret_cast<uint64*>(location));
	return *reinterpret_cast<float64*>(&hostInt);
}

float32 query::get_float32(word column) {
	return static_cast<float32>(this->get_float64(column));
}

uint64 query::get_uint64(word column) {
	if (!this->executed) { this->execute(); this->advance_row(); }
	char* location = PQgetvalue(this->result, static_cast<int>(this->row), static_cast<int>(column));
	if (!location) return 0;

	return util::net::net_to_host_int64(*reinterpret_cast<uint64*>(location));
}

uint32 query::get_uint32(word column) {
	if (!this->executed) { this->execute(); this->advance_row(); }
	char* location = PQgetvalue(this->result, static_cast<int>(this->row), static_cast<int>(column));
	if (!location) return 0;

	return util::net::net_to_host_int32(*reinterpret_cast<uint32*>(location));
}

uint16 query::get_uint16(word column) {
	if (!this->executed) { this->execute(); this->advance_row(); }
	char* location = PQgetvalue(this->result, static_cast<int>(this->row), static_cast<int>(column));
	if (!location) return 0;

	return util::net::net_to_host_int16(*reinterpret_cast<uint16*>(location));
}

uint8 query::get_uint8(word column) {
	return static_cast<uint8>(this->get_uint16(column));
}

int64 query::get_int64(word column) {
	return static_cast<int64>(this->get_uint64(column));
}

int32 query::get_int32(word column) {
	return static_cast<int32>(this->get_uint32(column));
}

int16 query::get_int16(word column) {
	return static_cast<int16>(this->get_uint16(column));
}

int8 query::get_int8(word column) {
	return static_cast<int8>(this->get_uint8(column));
}

bool query::get_bool(word column) {
	if (!this->executed) { this->execute(); this->advance_row(); }
	char* location = PQgetvalue(this->result, static_cast<int>(this->row), static_cast<int>(column));
	return location ? *location > 0 : false;
}

uint8* query::get_bytes(string column, uint8* buffer, word count) {
	return this->get_bytes(PQfnumber(this->result, column.c_str()), buffer, count);
}

string query::get_string(string column) {
	return this->get_string(PQfnumber(this->result, column.c_str()));
}

date_time query::get_date_time(string column) {
	return this->get_date_time(PQfnumber(this->result, column.c_str()));
}

data_stream query::get_data_stream(string column) {
	return this->get_data_stream(PQfnumber(this->result, column.c_str()));
}

float64 query::get_float64(string column) {
	return this->get_float64(PQfnumber(this->result, column.c_str()));
}

float32 query::get_float32(string column) {
	return static_cast<float32>(this->get_float64(PQfnumber(this->result, column.c_str())));
}

uint64 query::get_uint64(string column) {
	return this->get_uint64(PQfnumber(this->result, column.c_str()));
}

uint32 query::get_uint32(string column) {
	return this->get_uint32(PQfnumber(this->result, column.c_str()));
}

uint16 query::get_uint16(string column) {
	return this->get_uint16(PQfnumber(this->result, column.c_str()));
}

uint8 query::get_uint8(string column) {
	return this->get_uint8(PQfnumber(this->result, column.c_str()));
}

int64 query::get_int64(string column) {
	return this->get_int64(PQfnumber(this->result, column.c_str()));
}

int32 query::get_int32(string column) {
	return this->get_int32(PQfnumber(this->result, column.c_str()));
}

int16 query::get_int16(string column) {
	return this->get_int16(PQfnumber(this->result, column.c_str()));
}

int8 query::get_int8(string column) {
	return this->get_int8(PQfnumber(this->result, column.c_str()));
}

bool query::get_bool(string column) {
	return this->get_bool(PQfnumber(this->result, column.c_str()));
}

uint8* query::get_bytes(uint8* buffer, word count) {
	return this->get_bytes(this->column++, buffer, count);
}

string query::get_string() {
	return this->get_string(this->column++);
}

date_time query::get_date_time() {
	return this->get_date_time(this->column++);
}

data_stream query::get_data_stream() {
	return this->get_data_stream(this->column++);
}

float64 query::get_float64() {
	return this->get_float64(this->column++);
}

float32 query::get_float32() {
	return this->get_float32(this->column++);
}

uint64 query::get_uint64() {
	return this->get_uint64(this->column++);
}

uint32 query::get_uint32() {
	return this->get_uint32(this->column++);
}

uint16 query::get_uint16() {
	return this->get_uint16(this->column++);
}

uint8 query::get_uint8() {
	return this->get_uint8(this->column++);
}

int64 query::get_int64() {
	return this->get_int64(this->column++);
}

int32 query::get_int32() {
	return this->get_int32(this->column++);
}

int16 query::get_int16() {
	return this->get_int16(this->column++);
}

int8 query::get_int8() {
	return this->get_int8(this->column++);
}

bool query::get_bool() {
	return this->get_bool(this->column++);
}
