#pragma once

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <utility>
#include "array_ptr.h"

struct ReserveProxy {
    explicit ReserveProxy(size_t capacity_to_reserve)
        : capacity(capacity_to_reserve) {}

    size_t capacity;
};

inline ReserveProxy Reserve(size_t capacity_to_reserve) {
    return ReserveProxy(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(ReserveProxy proxy)
        : capacity_(proxy.capacity), data_(proxy.capacity) {}


    explicit SimpleVector(size_t size) : size_(size), capacity_(size), data_(size) {
        if (size > 0) {
            std::fill(data_.Get(), data_.Get() + size, Type());
        }
    }

    SimpleVector(size_t size, const Type& value) : size_(size), capacity_(size), data_(size) {
        if (size > 0) {
            std::fill(data_.Get(), data_.Get() + size, value);
        }
    }

    SimpleVector(std::initializer_list<Type> init) 
        : size_(init.size()), capacity_(init.size()), data_(init.size()) {
        if (size_ > 0) {
            std::copy(init.begin(), init.end(), data_.Get());
        }
    }

    SimpleVector(const SimpleVector& other) : size_(other.size_), capacity_(other.size_), data_(other.size_) {
        if (size_ > 0) {
            std::copy(other.begin(), other.end(), data_.Get());
        }
    }

    SimpleVector(SimpleVector&& other) noexcept 
        : size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          data_(other.data_.Release()) {}

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector temp(rhs);
            swap(temp);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        if (this != &rhs) {
            swap(rhs);
            rhs.Clear();
        }
        return *this;
    }

    ~SimpleVector() = default;

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> new_data(new_capacity);
            std::move(data_.Get(), data_.Get() + size_, new_data.Get());
            data_.swap(new_data);
            capacity_ = new_capacity;
        }
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& operator[](size_t index) noexcept {
        return data_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return data_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
            return;
        }

        if (new_size <= capacity_) {
            for (size_t i = size_; i < new_size; ++i) {
                data_[i] = Type();
            }
            size_ = new_size;
            return;
        }

        size_t new_capacity = std::max(new_size, capacity_ * 2);
        ArrayPtr<Type> new_data(new_capacity);
        
        std::move(data_.Get(), data_.Get() + size_, new_data.Get());
        for (size_t i = size_; i < new_size; ++i) {
            new_data[i] = Type();
        }

        data_.swap(new_data);
        size_ = new_size;
        capacity_ = new_capacity;
    }

    void PushBack(const Type& item) {
        if (size_ >= capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_data(new_capacity);
            
            std::move(data_.Get(), data_.Get() + size_, new_data.Get());
            new_data[size_] = item;

            data_.swap(new_data);
            capacity_ = new_capacity;
        } else {
            data_[size_] = item;
        }
        ++size_;
    }

    void PushBack(Type&& item) {
        if (size_ >= capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_data(new_capacity);
            
            std::move(data_.Get(), data_.Get() + size_, new_data.Get());
            new_data[size_] = std::move(item);

            data_.swap(new_data);
            capacity_ = new_capacity;
        } else {
            data_[size_] = std::move(item);
        }
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        return InsertImpl(pos, value);
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        return InsertImpl(pos, std::move(value));
    }

    void PopBack() noexcept {
        assert(size_ > 0);
        --size_;
    }

    Iterator Erase(ConstIterator pos) {
        size_t offset = pos - begin();
        std::move(data_.Get() + offset + 1, data_.Get() + size_, data_.Get() + offset);
        --size_;
        return begin() + offset;
    }

    void swap(SimpleVector& other) noexcept {
        data_.swap(other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    Iterator begin() noexcept { return data_.Get(); }
    Iterator end() noexcept { return data_.Get() + size_; }
    ConstIterator begin() const noexcept { return data_.Get(); }
    ConstIterator end() const noexcept { return data_.Get() + size_; }
    ConstIterator cbegin() const noexcept { return data_.Get(); }
    ConstIterator cend() const noexcept { return data_.Get() + size_; }

private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> data_;

    template <typename ValueType>
    Iterator InsertImpl(ConstIterator pos, ValueType&& value) {
        size_t offset = pos - begin();
        if (size_ >= capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_data(new_capacity);
            
            std::move(data_.Get(), data_.Get() + offset, new_data.Get());
            new_data[offset] = std::forward<ValueType>(value);
            std::move(data_.Get() + offset, data_.Get() + size_, new_data.Get() + offset + 1);

            data_.swap(new_data);
            capacity_ = new_capacity;
        } else {
            std::move_backward(data_.Get() + offset, data_.Get() + size_, data_.Get() + size_ + 1);
            data_[offset] = std::forward<ValueType>(value);
        }
        ++size_;
        return begin() + offset;
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize() && 
           std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}

