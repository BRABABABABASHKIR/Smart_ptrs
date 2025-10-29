#pragma once

#include "compressed_pair.h"

#include <cstddef>

template <typename T>
struct DefaultDeleter {
    DefaultDeleter() = default;

    template <typename U>
    DefaultDeleter(DefaultDeleter<U>&) {
    }

    template <typename U>
    DefaultDeleter(DefaultDeleter<U>&&) {
    }

    template <typename U>
    DefaultDeleter& operator=(DefaultDeleter<U>&) {
        return *this;
    }

    template <typename U>
    DefaultDeleter& operator=(DefaultDeleter<U>&&) {
        return *this;
    }

    void operator()(T* p) {
        delete p;
    }
};

template <typename T>
struct DefaultDeleter<T[]> {
    DefaultDeleter() = default;

    template <typename U>
    DefaultDeleter(DefaultDeleter<U[]>&) {
    }

    template <typename U>
    DefaultDeleter(DefaultDeleter<U[]>&&) {
    }

    template <typename U>
    DefaultDeleter& operator=(DefaultDeleter<U[]>&) {
        return *this;
    }

    template <typename U>
    DefaultDeleter& operator=(DefaultDeleter<U[]>&&) {
        return *this;
    }

    void operator()(T* p) {
        delete[] p;
    }
};

template <typename T, typename Deleter = DefaultDeleter<T>>
class UniquePtr {
public:
    using ComPair = CompressedPair<T*, Deleter>;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : holder_(ComPair(ptr, Deleter())) {
    }
    UniquePtr(T* ptr, Deleter deleter) : holder_(ComPair(ptr, std::move(deleter))) {
    }

    template <typename U, typename D>
    UniquePtr(UniquePtr<U, D>&& other) noexcept {
        holder_.GetFirst() = other.Release();
        holder_.GetSecond() = Deleter(std::move(other.GetDeleter()));
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        Reset();
        Swap(other);
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ~UniquePtr() {
        GetDeleter()(Get());
    }

    T* Release() {
        T* tmp = Get();
        holder_.GetFirst() = nullptr;
        return tmp;
    }
    void Reset(T* ptr = nullptr) {
        if (ptr == Get()) {
            return;
        }

        T* tmp = Get();
        holder_.GetFirst() = ptr;
        GetDeleter()(tmp);
    }
    void Swap(UniquePtr& other) {
        std::swap(holder_, other.holder_);
    }

    T* Get() const {
        return holder_.GetFirst();
    }
    Deleter& GetDeleter() {
        return holder_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return holder_.GetSecond();
    }
    explicit operator bool() const {
        return Get() != nullptr;
    }

    template <typename U = T, typename = std::enable_if_t<!std::is_void_v<U>>>
    U& operator*() const {
        return *Get();
    }
    T* operator->() const {
        return Get();
    }

private:
    ComPair holder_;
};

template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    using ComPair = CompressedPair<T*, Deleter>;

    explicit UniquePtr(T* ptr = nullptr) : holder_(ComPair(ptr, Deleter())) {
    }
    UniquePtr(T* ptr, Deleter deleter) : holder_(ComPair(ptr, std::move(deleter))) {
    }

    template <typename U, typename D>
    UniquePtr(UniquePtr<U, D>&& other) noexcept {
        holder_.GetFirst() = other.Release();
        holder_.GetSecond() = Deleter(std::move(other.GetDeleter()));
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        Reset();
        Swap(other);
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ~UniquePtr() {
        GetDeleter()(Get());
    }

    T* Release() {
        T* tmp = Get();
        holder_.GetFirst() = nullptr;
        return tmp;
    }
    void Reset(T* ptr = nullptr) {
        if (ptr == Get()) {
            return;
        }

        T* tmp = Get();
        holder_.GetFirst() = ptr;
        GetDeleter()(tmp);
    }
    void Swap(UniquePtr& other) {
        std::swap(holder_, other.holder_);
    }

    T* Get() const {
        return holder_.GetFirst();
    }
    Deleter& GetDeleter() {
        return holder_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return holder_.GetSecond();
    }
    explicit operator bool() const {
        return Get() != nullptr;
    }

    T& operator*() const {
        return *Get();
    }
    T* operator->() const {
        return Get();
    }

    T& operator[](size_t ind) const {
        return *(Get() + ind);
    }

private:
    ComPair holder_;
};
