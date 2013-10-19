#pragma once

#include "Common.h"

#include <utility>
#include <type_traits>

namespace util {
	template<typename T> class optional {
		T type;
		bool is_set;

		public:
			optional& operator=(const optional& other) = delete;
			optional& operator=(optional&& other) = delete;
			optional(const optional& other) = delete;
			optional(optional&& other) = delete;

			exported optional() {
				this->is_set = false;
			}

			exported optional(const T& obj) : type(obj) {
				this->is_set = true;
			}

			exported optional(T&& obj) : type(std::move(obj)) {
				this->is_set = true;
			}

			exported void operator=(const T& obj) {
				this->type = obj;
				this->is_set = true;
			}

			exported void operator=(T&& obj) {
				this->type = std::move(obj);
				this->is_set = true;
			}

			exported void set(const T& obj) {
				this->type = obj;
				this->is_set = true;
			}

			exported void set(T&& obj) {
				this->type = std::move(obj);
				this->is_set = true;
			}

			exported T value() {
				return this->type;
			}

			exported T&& extract() {
				this->is_set = false;
				return std::move(this->type);
			}

			exported void clear() {
				this->is_set = false;
				this->type = T();
			}

			exported bool valid() const {
				return this->is_set;
			}

			exported operator bool() const {
				return this->is_set;
			}

			exported operator T() {
				return this->type;
			}

			exported T& operator*() {
				return this->type;
			}

			exported T* operator->() {
				return &this->type;
			}
	};

	template<typename T> class optional<T&> {
		typename std::remove_reference<T>::type* type;;

		public:
			optional& operator=(const optional& other) = delete;
			optional& operator=(optional&& other) = delete;
			optional(const optional& other) = delete;
			optional(optional&& other) = delete;

			exported optional() {
				this->type = nullptr;
			}

			exported optional(T& obj) : type(&obj) {

			}

			exported void operator=(T& obj) {
				this->type = &obj;
			}

			exported void set(T& obj) {
				this->type = &obj;
			}

			exported void clear() {
				this->type = nullptr;
			}

			exported T& value() {
				return *this->type;
			}

			exported bool valid() const {
				return this->type != nullptr;
			}

			exported operator bool() const {
				return this->type != nullptr;
			}

			exported operator T() {
				return this->type;
			}

			exported T& operator*() {
				return *this->type;
			}

			exported T* operator->() {
				return this->type;
			}
	};
}
