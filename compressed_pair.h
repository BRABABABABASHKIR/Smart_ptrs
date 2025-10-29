#pragma once

#include <type_traits>
#include <utility>

template <typename F, char type_name, bool flag = std::is_empty_v<F> && !std::is_final_v<F>>
class Node;

template <typename F, char type_name>
class Node<F, type_name, true> : public F {
public:
    Node() {
        F();
    }
    Node(const F&) {
        F();
    };
    Node(F&&) {
        F();
    }

    F& GetNode() {
        return *this;
    }
    const F& GetNode() const {
        return *this;
    }
};

template <typename S, char type_name>
class Node<S, type_name, false> {
public:
    Node() {
        S();
    }
    Node(const S& node) : node_(node){};
    Node(S&& node) : node_(std::move(node)){};

    S& GetNode() {
        return node_;
    }
    const S& GetNode() const {
        return node_;
    }

private:
    S node_;
};

template <typename F, typename S>
class CompressedPair : Node<F, 'F'>, Node<S, 'S'> {
public:
    CompressedPair() : Node<F, 'F'>(F()), Node<S, 'S'>(S()) {
    }
    CompressedPair(const F& first, const S& second) : Node<F, 'F'>(first), Node<S, 'S'>(second) {
    }
    CompressedPair(F&& first, S&& second)
            : Node<F, 'F'>(std::move(first)), Node<S, 'S'>(std::move(second)) {
    }
    CompressedPair(const F& first, S&& second)
            : Node<F, 'F'>(first), Node<S, 'S'>(std::move(second)) {
    }
    CompressedPair(F&& first, const S& second)
            : Node<F, 'F'>(std::move(first)), Node<S, 'S'>(second) {
    }

    F& GetFirst() {
        return Node<F, 'F'>::GetNode();
    }
    S& GetSecond() {
        return Node<S, 'S'>::GetNode();
    }

    const F& GetFirst() const {
        return Node<F, 'F'>::GetNode();
    }
    const S& GetSecond() const {
        return Node<S, 'S'>::GetNode();
    }
};