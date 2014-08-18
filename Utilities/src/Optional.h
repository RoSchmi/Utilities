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

			optional() {
				this->is_set = false;
			}

			optional(const T& obj) : type(obj) {
				this->is_set = true;
			}

			optional(T&& obj) : type(std::move(obj)) {
				this->is_set = true;
			}

			void operator=(const T& obj) {
				this->type = obj;
				this->is_set = true;
			}

			void operator=(T&& obj) {
				this->type = std::move(obj);
				this->is_set = true;
			}

			void set(const T& obj) {
				this->type = obj;
				this->is_set = true;
			}

			void set(T&& obj) {
				this->type = std::move(obj);
				this->is_set = true;
			}

			T value() {
				return this->type;
			}

			T&& extract() {
				this->is_set = false;
				return std::move(this->type);
			}

			void clear() {
				this->is_set = false;
				this->type = T();
			}

			bool valid() const {
				return this->is_set;
			}

			operator bool() const {
				return this->is_set;
			}

			operator T() {
				return this->type;
			}

			T& operator*() {
				return this->type;
			}

			T* operator->() {
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

			optional() {
				this->type = nullptr;
			}

			optional(T& obj) : type(&obj) {

			}

			void operator=(T& obj) {
				this->type = &obj;
			}

			void set(T& obj) {
				this->type = &obj;
			}

			void clear() {
				this->type = nullptr;
			}

			T& value() {
				return *this->type;
			}

			bool valid() const {
				return this->type != nullptr;
			}

			operator bool() const {
				return this->type != nullptr;
			}

			operator T() {
				return this->type;
			}

			T& operator*() {
				return *this->type;
			}

			T* operator->() {
				return this->type;
			}
	};
}
