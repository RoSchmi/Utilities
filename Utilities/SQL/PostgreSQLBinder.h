#pragma once

#include <string>
#include <vector>
#include <sstream>

#include "../Common.h"
#include "TableBinder.h"
#include "PostgreSQL.h"

namespace util {
	namespace sql {
		namespace postgres {
			template<typename T, typename PType = uint64, typename PName = "id"> class table_binder : public sql::table_binder<T, postgres::connection, postgres::query, PType, PName> {
				std::string lock_stmt;

				virtual void generate_select_by_id() override {
					this->select_by_id_query.query_str = "SELECT * FROM " + this->name + " WHERE " + PName + " = $1" + this->lock_stmt;
				}

				virtual void generate_delete() override {
					this->delete_query.query_str = "DELETE FROM " + this->name + " WHERE Id = $1;";
				}

				virtual void generate_insert() override {
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

					word column_index = 0;
					for (auto i : this->defs) {
						if (!isFirst)
							query << ", ";
						else
							isFirst = false;
						query << "$" << ++column_index;
					}

					query << ");";

					this->insert_query.query_str = query.str();
				}

				virtual void generate_update() override {
					std::stringstream query;
					query << "UPDATE " << this->name << " SET ";

					word column_index = 0;
					bool isFirst = true;
					for (auto i : this->defs) {
						if (i.updatable) {
							if (!isFirst)
								query << ", ";
							else
								isFirst = false;

							query << i.name << " = $" << ++column_index;
						}
					}

					query << " WHERE Id = $" << column_index + 1;

					this->update_query.query_str = query.str();
				}

				public:
					exported table_binder(postgres::connection& conn, std::string name, bool lock_row = true) : sql::table_binder<T, postgres::connection, postgres::query, PType, PName>(conn, name) {
						this->lock_stmt = lock_row ? " FOR UPDATE;" : "";
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
			};
		}
	}
}
