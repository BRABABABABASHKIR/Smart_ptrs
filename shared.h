#pragma once

#include "sw_fwd.h"

#include <cstddef>
#include <memory>

class ESFTBase {};

template <typename T>
class EnableSharedFromThis : public ESFTBase {
public:
    SharedPtr<T> SharedFromThis() {
        return weak_this_.Lock();
    }
    SharedPtr<const T> SharedFromThis() const {
        return weak_this_.Lock();
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this_;
    }

    WeakPtr<T>& GetWeak() {
        return weak_this_;
    }

private:
    WeakPtr<T> weak_this_;
};

class ControlBlockBase {
public:
    ControlBlockBase() : count_(1) {
    }

    void Increment() {
        ++count_;
    }
    void Decrement() {
        --count_;
        if (count_ == 0) {
            ++count_weak_;

            Destroy();

            --count_weak_;
        }
        if (count_ + count_weak_ == 0) {
            delete this;
        }
    }

    void IncrementWeak() {
        ++count_weak_;
    }
    void DecrementWeak() {
        --count_weak_;
        if (count_ + count_weak_ == 0) {
            delete this;
        }
    }

    size_t GetCount() {
        return count_;
    }
    size_t GetCountWeak() {
        return count_weak_;
    }

    virtual ~ControlBlockBase() = default;

    virtual void Destroy() = 0;
    virtual void* Get() = 0;

private:
    size_t count_ = 0;
    size_t count_weak_ = 0;
};

template <typename T>
class ControlBlockPointer : public ControlBlockBase {
public:
    ControlBlockPointer() : ptr_(nullptr) {
    }

    ControlBlockPointer(T* ptr) : ptr_(ptr) {
    }

    void* Get() override {
        return ptr_;
    }

    void Destroy() override {
        delete ptr_;
        ptr_ = nullptr;
    }

    ~ControlBlockPointer() override = default;

private:
    T* ptr_;
};

template <typename T>
class ControlBlockElement : public ControlBlockBase {
public:
    template <typename... Args>
    ControlBlockElement(Args&&... args) {
        T* place = reinterpret_cast<T*>(buffer_);
        std::construct_at(place, std::forward<Args>(args)...);
    }

    void* Get() override {
        return buffer_;
    }
    void Destroy() override {
        T* place = reinterpret_cast<T*>(buffer_);
        place->~T();
    }

    ~ControlBlockElement() override = default;

private:
    alignas(T) std::byte buffer_[sizeof(T)];
};

template <typename T>
class SharedPtr {
public:
    SharedPtr() {
        ptr_ = nullptr;
        block_ = nullptr;
    }
    SharedPtr(std::nullptr_t) : SharedPtr() {
    }

    template <typename Y>
    void InitWeakThis(EnableSharedFromThis<Y>* e) {
        e->GetWeak() = *this;
    }

    template <typename U>
    explicit SharedPtr(U* ptr) {
        ptr_ = ptr;
        block_ = new ControlBlockPointer<U>(ptr);

        if constexpr (std::is_convertible_v<U*, ESFTBase*>) {
            InitWeakThis(ptr_);
        }
    }

    SharedPtr(const SharedPtr& other) {
        block_ = other.GetBlock();
        if (block_ != nullptr) {
            block_->Increment();
        }
        ptr_ = other.Get();
    }
    template <typename U>
    SharedPtr(const SharedPtr<U>& other) {
        block_ = other.GetBlock();
        if (block_ != nullptr) {
            block_->Increment();
        }
        ptr_ = other.Get();
    }

    SharedPtr(SharedPtr&& other) {
        ptr_ = other.Get();
        block_ = other.GetBlock();
        other.MakeNull();
    }
    template <typename U>
    SharedPtr(SharedPtr<U>&& other) {
        ptr_ = other.Get();
        block_ = other.GetBlock();
        other.MakeNull();
    }

    SharedPtr(ControlBlockBase* block) {
        block_ = block;
        ptr_ = reinterpret_cast<T*>(block_->Get());
    }
    SharedPtr(ControlBlockBase* block, T* ptr) {
        block_ = block;
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<T*, ESFTBase*>) {
            InitWeakThis(ptr_);
        }
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        block_ = other.GetBlock();
        if (block_ != nullptr) {
            block_->Increment();
        }
        ptr_ = ptr;
    }

    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.GetBlock() == nullptr || other.GetBlock()->GetCount() == 0) {
            throw BadWeakPtr{};
        }

        block_ = other.GetBlock();
        if (block_ == nullptr) {
            ptr_ = nullptr;
            return;
        }
        ptr_ = reinterpret_cast<T*>(block_->Get());
        block_->Increment();
    }

    SharedPtr& operator=(const SharedPtr& other) {
        if (other.GetBlock() != nullptr) {
            other.GetBlock()->Increment();
        }
        if (block_ != nullptr) {
            block_->Decrement();
        }
        ptr_ = other.Get();
        block_ = other.GetBlock();
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        Reset();
        this->Swap(other);
        return *this;
    }

    ~SharedPtr() {
        if (block_ == nullptr) {
            return;
        }
        Reset();
    }

    void MakeNull() {
        this->ptr_ = nullptr;
        this->block_ = nullptr;
    }
    void Reset() {
        if (block_ != nullptr) {
            block_->Decrement();
        }
        this->ptr_ = nullptr;
        this->block_ = nullptr;
    }
    template <typename U>
    void Reset(U* ptr) {
        *this = SharedPtr(ptr);
    }

    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    }


    T* Get() const {
        return ptr_;
    }
    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    ControlBlockBase* GetBlock() const {
        return block_;
    }
    size_t UseCount() const {
        if (block_ == nullptr) {
            return 0;
        }
        return block_->GetCount();
    }
    explicit operator bool() const {
        return block_ != nullptr;
    }

private:
    T* ptr_;
    ControlBlockBase* block_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.GetBlock() == right.GetBlock() && left.Get() == right.Get();
}

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    ControlBlockElement<T>* cur_block = new ControlBlockElement<T>(std::forward<Args>(args)...);
    SharedPtr<T> cur_shared(cur_block, reinterpret_cast<T*>(cur_block->Get()));
    return cur_shared;
}
