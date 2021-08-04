#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:
	RawMemory() = default;
	
	explicit RawMemory(size_t capacity)
			: buffer_(Allocate(capacity))
			, capacity_(capacity) {
	}
	
	RawMemory(const RawMemory&) = delete;
	
	RawMemory& operator=(const RawMemory& rhs) = delete;
	
	RawMemory(RawMemory&& other) noexcept
			: capacity_(other.capacity_)
	{
		std::uninitialized_move_n(other.buffer_, capacity_, buffer_);
	}
	
	RawMemory& operator=(RawMemory&& rhs) noexcept {
		capacity_ = rhs.capacity_;
		std::uninitialized_move_n(rhs.buffer_, capacity_, buffer_);
		return *this;
	}
	
	~RawMemory() {
		Deallocate(buffer_);
	}
	
	T* operator+(size_t offset) noexcept {
		// Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
		assert(offset <= capacity_);
		return buffer_ + offset;
	}
	
	const T* operator+(size_t offset) const noexcept {
		return const_cast<RawMemory&>(*this) + offset;
	}
	
	const T& operator[](size_t index) const noexcept {
		return const_cast<RawMemory&>(*this)[index];
	}
	
	T& operator[](size_t index) noexcept {
		assert(index < capacity_);
		return buffer_[index];
	}
	
	void Swap(RawMemory& other) noexcept {
		std::swap(buffer_, other.buffer_);
		std::swap(capacity_, other.capacity_);
	}
	
	const T* GetAddress() const noexcept {
		return buffer_;
	}
	
	T* GetAddress() noexcept {
		return buffer_;
	}
	
	size_t Capacity() const {
		return capacity_;
	}

private:
	// Выделяет сырую память под n элементов и возвращает указатель на неё
	static T* Allocate(size_t n) {
		return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
	}
	
	// Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
	static void Deallocate(T* buf) noexcept {
		operator delete(buf);
	}
	
	T* buffer_ = nullptr;
	size_t capacity_ = 0;
};



template <typename T>
class SimpleVector {
public:
	
	using iterator = T*;
	using const_iterator = const T*;
	
	iterator begin() noexcept {
		return data_.GetAddress();
	}
	iterator end() noexcept {
		return data_ + size_;
	}
	const_iterator begin() const noexcept {
		return data_.GetAddress();
	}
	const_iterator end() const noexcept {
		return data_ + size_;
	}
	const_iterator cbegin() const noexcept {
		return data_.GetAddress();
	}
	const_iterator cend() const noexcept {
		return data_ + size_;
	}
	
	SimpleVector() noexcept = default;
	
	explicit SimpleVector(size_t size)
			: data_(size)
			, size_(size)
	{
		std::uninitialized_value_construct_n(data_.GetAddress(), size_);
	}
	
	SimpleVector(const SimpleVector& other)
			: data_(other.size_)
			, size_(other.size_)
	{
		std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
	}
	
	SimpleVector(SimpleVector&& other) noexcept {
		Swap(other);
	}
	
	SimpleVector& operator=(const SimpleVector& rhs) {
		if (this != &rhs) {
			if (rhs.size_ > data_.Capacity()) {
				SimpleVector<T> rhs_copy = rhs;
				Swap(rhs_copy);
			} else {
				if(size_ > rhs.size_) {
					std::destroy_n(data_.GetAddress(), size_ - rhs.size_);
					for(size_t i = 0; i < rhs.size_; ++i) {
						data_[i] = rhs.data_[i];
					}
				}
				else {
					for(size_t i = 0; i < size_; ++i) {
						data_[i] = rhs.data_[i];
					}
					std::uninitialized_copy_n(rhs.data_.GetAddress(), rhs.size_ - size_, data_.GetAddress());
				}
				size_ = rhs.size_;
			}
		}
		return *this;
	}
	SimpleVector& operator=(SimpleVector&& rhs) noexcept {
		if (this != &rhs) {
			Swap(rhs);
		}
		return *this;
	}
	
	size_t Size() const noexcept {
		return size_;
	}
	
	size_t Capacity() const noexcept {
		return data_.Capacity();
	}
	
	const T& operator[](size_t index) const noexcept {
		return const_cast<SimpleVector&>(*this)[index];
	}
	
	T& operator[](size_t index) noexcept {
		assert(index < size_);
		return data_[index];
	}
	
	void Swap(SimpleVector& other) noexcept {
		data_.Swap(other.data_);
		std::swap(size_, other.size_);
	}
	
	void Resize(size_t new_size) {
		if(size_ > new_size) {
			std::destroy_n(data_.GetAddress(), size_ - new_size);
		}
		else if (size_ < new_size){
			Reserve(new_size);
			std::uninitialized_value_construct_n(data_.GetAddress(), new_size - size_);
		}
		size_ = new_size;
	}
	
	void PushBack(const T& value) {
		if(size_ != Capacity()) {
				std::uninitialized_copy_n(&value, 1, data_ + size_);
		}
		else {
			RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
			std::uninitialized_copy_n(&value, 1, new_data + size_);
			if constexpr (std::is_copy_constructible_v<T> && !std::is_nothrow_move_constructible_v<T>) {
				std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
			}
			else {
				std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
			}
			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
		}
		++size_;
	}
	void PushBack(T&& value){
		if (size_ != Capacity()) {
			std::uninitialized_move_n(&value, 1, data_ + size_);
		}
		else {
			RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
			std::uninitialized_move_n(&value, 1, new_data + size_);
			if constexpr (std::is_copy_constructible_v<T> && !std::is_nothrow_move_constructible_v<T>) {
				std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
			}
			else {
				std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
			}
			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
		}
		++size_;
	}
	
	template <typename... Args>
	T& EmplaceBack(Args&&... args) {
		if (size_ != Capacity()) {
			new(data_ + size_) T(std::forward<Args>(args)...);
		}
		else {
			RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
			new(new_data + size_) T(std::forward<Args>(args)...);
			if constexpr (std::is_copy_constructible_v<T> && !std::is_nothrow_move_constructible_v<T>) {
				std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
			}
			else {
				std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
			}
			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
		}
		++size_;
		return this->operator[](size_ - 1);
	}
	
	void PopBack() {
		std::destroy_at(data_.GetAddress());
		--size_;
	}
	
	iterator Insert(const_iterator pos, const T& value) {
		return Emplace(pos, value);
	}
	iterator Insert(const_iterator pos, T&& value) {
		return Emplace(pos, std::move(value));
	}
	
	template <typename... Args>
	iterator Emplace(const_iterator pos, Args&&... args) {
		size_t dist_to_pos = std::distance(cbegin(), pos);
		assert(dist_to_pos >= 0);

		if(size_ < Capacity()) {
			if(dist_to_pos < size_) {
				T t(std::forward<Args>(args)...);
				new(end()) T(std::forward<T>(data_[size_ - 1]));
				std::move_backward(begin() + dist_to_pos, end() - 1, end());
				data_[dist_to_pos] = std::forward<T>(t);
			}
			else {
				new(data_ + dist_to_pos) T(std::forward<Args>(args)...);
			}
		}
		else {
			RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
			try {
				new(new_data + dist_to_pos) T(std::forward<Args>(args)...);
			}
			catch (...) {
				new_data.~RawMemory();
				throw;
			}
			if constexpr (!std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>) {
				std::uninitialized_move_n(data_.GetAddress(), dist_to_pos, new_data.GetAddress());
			}
			else {
				try {
					std::uninitialized_copy_n(data_.GetAddress(), dist_to_pos, new_data.GetAddress());
				}
				catch(...) {
					std::destroy_n(new_data + dist_to_pos, 1);
					throw;
				}
			}
			try {
				if constexpr (!std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>) {
					std::uninitialized_move_n(begin() + dist_to_pos, size_ - dist_to_pos, new_data + dist_to_pos + 1);
				}
				else {
					std::uninitialized_copy_n(begin() + dist_to_pos, size_ - dist_to_pos, new_data + dist_to_pos + 1);
				}
			}
			catch(...) {
				std::destroy_n(new_data.GetAddress(), dist_to_pos);
				throw;
			}
			std::destroy_n(data_.GetAddress(), size_);
			data_.Swap(new_data);
		}
		++size_;
		return data_ + dist_to_pos;
	}
	
	iterator Erase(const_iterator pos) {
		size_t dist_to_pos = std::distance(cbegin(), pos);
		assert(dist_to_pos >= 0);
		std::move(begin() + dist_to_pos + 1, end(), begin() + dist_to_pos);
		std::destroy_n(end() - 1, 1);
		--size_;
		return begin() + dist_to_pos;
	}

	~SimpleVector() {
		std::destroy_n(data_.GetAddress(), size_);
	}
	
	void Reserve(size_t new_capacity) {
		if (new_capacity <= data_.Capacity()) {
			return;
		}
		RawMemory<T> new_data(new_capacity);
		if constexpr (!std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>) {
			std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
		}
		else {
			std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
		}
		std::destroy_n(data_.GetAddress(), size_);
		data_.Swap(new_data);
	}

private:
	RawMemory<T> data_;
	size_t size_ = 0;
};