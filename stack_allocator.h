#pragma once

#include <iostream>
#include <tuple>

template <size_t N>
class StackStorage {
  private:
    char data_[N];
    size_t cur_pos_ = 0;

  public:
    StackStorage(){};
    StackStorage(const StackStorage&) = delete;
    StackStorage& operator=(const StackStorage&) = delete;

    void* aligned_alloc(size_t count, size_t align) {
        size_t rem = reinterpret_cast<size_t>(data_ + cur_pos_) % align;
        cur_pos_ += (align - rem) % align;
        void* ptr = data_ + cur_pos_;
        cur_pos_ += count;
        return ptr;
    }
};

template <typename T, size_t N>
class StackAllocator {
  private:
    StackStorage<N>* storage_;

  public:
    using value_type = T;

    StackAllocator() = default;
    ~StackAllocator() = default;

    StackAllocator(StackStorage<N>& storage) noexcept
        : storage_(&storage) {}

    template <typename U>
    StackAllocator(const StackAllocator<U, N>& other) noexcept
        : storage_(other.get_storage()) {}

    template <typename U>
    StackAllocator& operator=(const StackAllocator<U, N>& other) {
        storage_ = other.get_storage();
        return *this;
    }

    T* allocate(size_t count) {
        void* ptr = storage_->aligned_alloc(sizeof(T) * count, alignof(T));
        return reinterpret_cast<T*>(ptr);
    }

    void deallocate(T* ptr, size_t cnt) {
        std::ignore = ptr;
        std::ignore = cnt;
    }

    StackStorage<N>* get_storage() const {
        return storage_;
    }

    template <typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };
};

template <typename T, size_t N, typename U, size_t M>
bool operator==(const StackAllocator<T, N>& lhs, const StackAllocator<U, M>& rhs) {
    return lhs.get_storage() == rhs.get_storage();
}

template <typename T, size_t N, typename U, size_t M>
bool operator!=(const StackAllocator<T, N>& lhs, const StackAllocator<U, M>& rhs) {
    return !(lhs == rhs);
}
