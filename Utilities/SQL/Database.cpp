#include "Database.h"

using namespace std;
using namespace util;
using namespace util::sql;

db_object<>::db_object() {
	this->valid = true;
}

db_object<>::operator bool() {
	return this->valid;
}

db_exception::db_exception(string message) {
	this->message = message;
}

connection::connection() {
	this->is_connected = false;
	this->is_committed = false;
}

bool connection::connected() const {
	return this->is_connected;
}

bool connection::committed() const {
	return this->is_committed;
}

query::query(string query_str, connection* conn) {
	this->conn = conn;
	this->query_str = query_str;
	this->row_count = 0;
	this->row = -1;
	this->column = 0;
	this->executed = false;
}

query::query(query&& other) {
	this->conn = other.conn;
	this->query_str = other.query_str;
	this->executed = other.executed;
	this->row_count = other.row_count;
	this->row = other.row;
	this->column = other.column;

	other.query_str = "";
	other.conn = nullptr;
}

word query::rows() const {
	return this->row_count;
}

bool query::advance_row() {
	this->row++;
	this->column = 0;

	return this->row < static_cast<sword>(this->row_count);
}

query& query::operator<<(const string& para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(const date_time& para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(const data_stream& para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(cstr para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(float64 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(float32 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(uint64 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(uint32 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(uint16 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(uint8 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(int64 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(int32 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(int16 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(int8 para) {
	this->add_para(para);
	return *this;
}

query& query::operator<<(bool para) {
	this->add_para(para);
	return *this;
}

query& query::operator>>(string& para) {
	para = this->get_string();
	return *this;
}

query& query::operator>>(date_time& para) {
	para = this->get_date_time();
	return *this;
}

query& query::operator>>(data_stream& para) {
	para = this->get_data_stream();
	return *this;
}

query& query::operator>>(float64& para) {
	para = this->get_float64();
	return *this;
}

query& query::operator>>(float32& para) {
	para = this->get_float32();
	return *this;
}

query& query::operator>>(uint64& para) {
	para = this->get_uint64();
	return *this;
}

query& query::operator>>(uint32& para) {
	para = this->get_uint32();
	return *this;
}

query& query::operator>>(uint16& para) {
	para = this->get_uint16();
	return *this;
}

query& query::operator>>(uint8& para) {
	para = this->get_uint8();
	return *this;
}

query& query::operator>>(int64& para) {
	para = this->get_int64();
	return *this;
}

query& query::operator>>(int32& para) {
	para = this->get_int32();
	return *this;
}

query& query::operator>>(int16& para) {
	para = this->get_int16();
	return *this;
}

query& query::operator>>(int8& para) {
	para = this->get_int8();
	return *this;
}

query& query::operator>>(bool& para) {
	para = this->get_bool();
	return *this;
}
