#pragma once

#include "shared.h"
#include "sw_fwd.h"

template <typename T>
class WeakPtr {
public:

    WeakPtr() {
        block_ = nullptr;
    }

    WeakPtr(const WeakPtr& other) {
        block_ = other.GetBlock();
        if (block_ != nullptr) {
            block_->IncrementWeak();
        }
    }
    WeakPtr(WeakPtr&& other) {
        block_ = other.GetBlock();
        other.MakeNull();
    }
    template <typename U>
    WeakPtr(const WeakPtr<U>& other) {
        block_ = other.GetBlock();
        if (block_) {
            block_->IncrementWeak();
        }
    }

    WeakPtr(const SharedPtr<T>& other) {
        block_ = other.GetBlock();
        if (block_ != nullptr) {
            block_->IncrementWeak();
        }
    }

    template <typename U>
    WeakPtr& operator=(const SharedPtr<U>& other) {
        Reset();
        block_ = other.GetBlock();
        if (block_ != nullptr) {
            block_->IncrementWeak();
        }
        return *this;
    }
    WeakPtr& operator=(const WeakPtr& other) {
        if (other.GetBlock() != nullptr) {
            other.GetBlock()->IncrementWeak();
        }
        if (GetBlock() != nullptr) {
            GetBlock()->DecrementWeak();
        }
        block_ = other.GetBlock();
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) {
        if (block_ != nullptr && GetBlock() != other.GetBlock()) {
            block_->DecrementWeak();
        }
        block_ = other.GetBlock();
        other.MakeNull();
        return *this;
    }

    ~WeakPtr() {
        if (block_ == nullptr) {
            return;
        }
        block_->DecrementWeak();
    }

    void MakeNull() {
        block_ = nullptr;
    }
    void Reset() {
        if (block_ != nullptr) {
            block_->DecrementWeak();
        }
        MakeNull();
    }
    void Swap(WeakPtr& other) {
        std::swap(this->block_, other.block_);
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
    bool Expired() const {
        return UseCount() == 0;
    }
    SharedPtr<T> Lock() const {
        SharedPtr<T> cur;
        if (block_ != nullptr && block_->GetCount() != 0) {
            block_->Increment();
            cur = SharedPtr<T>(block_, reinterpret_cast<T*>(block_->Get()));
        }
        return cur;
    }

private:
    ControlBlockBase* block_;
};
