#pragma once

#include "Common.h"
#include <utility>

namespace Utilities {
	template<typename T> class exported MovableList {
		public:
			class exported Iterator {
				uint32 index;
				MovableList<T>& parent;

				Iterator(MovableList<T>& parent, uint32 index) : parent(parent) {
					this->index = index;
				};
				
				friend class MovableList;

				public:
					Iterator& operator++() {
						this->index++;
						return *this;
					}

					bool operator==(const Iterator& other) const {
						return other.index == this->index;
					}

					bool operator!=(const Iterator& other) const {
						return !(other == *this);
					}

					T& operator*() {
						return *this->parent.data[this->index];
					}
			};

		private:
			MovableList(const MovableList& other);
			MovableList& operator=(const MovableList& other);

			Iterator beginning;
			Iterator ending;

			T** data;
			uint32 size;
			uint32 count;

			void grow() {
				uint32 newSize = this->size * 2;
				T** newData = new T*[newSize];

				for (uint32 i = 0; i < this->count; ++i)
					newData[i] = this->data[i];

				delete[] this->data;

				this->size = newSize;
				this->data = newData;
			}

		public:
			MovableList() : beginning(*this, 0), ending(*this, 0) {
				this->size = 2;
				this->count = 0;
				this->data = new T*[this->size];
			}

			~MovableList() {
				for (uint32 i = 0; i < this->count; ++i)
					delete this->data[i];

				delete[] this->data;
			}

			MovableList(MovableList&& other) : beginning(*this, 0), ending(*this, 0) {
				*this = std::move(other);
			}

			MovableList& operator=(MovableList&& other) {
				this->data = other.data;
				this->size = other.size;
				this->count = other.count;
				this->ending.index = other.ending.index;

				other.count = 0;
				other.size = 0;
				other.data = nullptr;
				other.ending.index = 0;

				return *this;
			}

			void insert(T&& item) {
				if (this->count >= this->size)
					this->grow();

				this->data[this->count++] = new T(std::move(item));

				this->ending.index++;
			}

			T& operator[](uint32 index) {
				return *this->data[index];
			}

			Iterator begin() {
				return this->beginning;
			}

			Iterator end() {
				return this->ending;
			}

			uint32 getCount() const {
				return this->count;
			}
	};
}
