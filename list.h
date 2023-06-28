#pragma once

#include <iostream>
#include <iterator>
#include <tuple>

template <typename T, typename Allocator = std::allocator<T>>
class List {
  private:
    struct BaseNode {
        BaseNode* next = nullptr;
        BaseNode* prev = nullptr;
        BaseNode() {}
        BaseNode(const T& val) {
            std::ignore = val;
        }
    };

    struct ListNode : BaseNode {
        T val;
        ListNode() {}
        ListNode(const T& val)
            : val(val) {}
    };

    using AllocTraits = std::allocator_traits<Allocator>;
    using NodeTraits = typename AllocTraits::template rebind_traits<ListNode>;
    using NodeAlloc = typename AllocTraits::template rebind_alloc<ListNode>;

    [[no_unique_address]] NodeAlloc alloc_;
    BaseNode base_;
    size_t size_;

    void clear() {
        BaseNode* head = base_.next;
        for (size_t i = 0; i < size_; ++i) {
            BaseNode* next_head = head->next;
            NodeTraits::destroy(alloc_, static_cast<ListNode*>(head));
            NodeTraits::deallocate(alloc_, static_cast<ListNode*>(head), 1);
            head = next_head;
        }
        size_ = 0;
    }

    void swap(List& other) {
        std::swap(base_.next, other.base_.next);
        std::swap(base_.prev, other.base_.prev);
        std::swap(size_, other.size_);
        if (size_ != 0) {
            base_.next->prev = &base_;
            base_.prev->next = &base_;
        }
        if (other.size_ != 0) {
            other.base_.next->prev = &other.base_;
            other.base_.prev->next = &other.base_;
        }
    }

  public:
    template <bool is_const>
    class common_iterator {
      public:
        using value_type = T;
        using reference = std::conditional_t<is_const, T const&, T&>;
        using difference_type = ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        using base_node_pointer = std::conditional_t<is_const, const BaseNode*, BaseNode*>;
        using list_node_pointer = std::conditional_t<is_const, const ListNode*, ListNode*>;

        reference operator*() const {
            auto list_node = static_cast<list_node_pointer>(node_);
            return list_node->val;
        }

        common_iterator& operator++() {
            node_ = node_->next;
            return *this;
        }

        common_iterator operator++(int) {
            auto copy = *this;
            operator++();
            return copy;
        }

        common_iterator& operator--() {
            node_ = node_->prev;
            return *this;
        }

        common_iterator operator--(int) {
            auto copy = *this;
            operator--();
            return copy;
        }

        bool operator==(const common_iterator& other) const {
            return node_ == other.node_;
        }

        bool operator!=(const common_iterator& other) const {
            return !operator==(other);
        }

        operator common_iterator<true>() const {
            return common_iterator<true>(node_);
        }

      private:
        base_node_pointer node_;

        base_node_pointer get_node() const {
            return node_;
        }
        explicit common_iterator(base_node_pointer node)
            : node_(node) {}
        friend List;
    };

    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    List()
        : List(Allocator()) {}

    List(Allocator alloc)
        : alloc_(alloc), size_(0) {
        base_.next = &base_;
        base_.prev = &base_;
    }

    List(size_t n)
        : List(n, Allocator()) {}

    List(size_t n, Allocator alloc)
        : alloc_(alloc), size_(0) {
        ListNode* new_node = nullptr;
        try {
            BaseNode* tail = &base_;
            tail->prev = tail;
            tail->next = tail;
            for (size_t i = 0; i < n; ++i) {
                new_node = NodeTraits::allocate(alloc_, 1);
                NodeTraits::construct(alloc_, new_node);
                tail->next = new_node;
                new_node->prev = tail;
                base_.prev = new_node;
                new_node->next = &base_;
                tail = new_node;
                ++size_;
            }
        } catch (...) {
            NodeTraits::deallocate(alloc_, new_node, 1);
            clear();
            throw;
        }
    }

    List(size_t n, const T& val)
        : List(n, val, Allocator()) {}

    List(size_t n, const T& val, Allocator alloc)
        : alloc_(alloc), size_(0) {
        ListNode* new_node = nullptr;
        try {
            BaseNode* tail = &base_;
            tail->prev = tail;
            tail->next = tail;
            for (size_t i = 0; i < n; ++i) {
                new_node = NodeTraits::allocate(alloc_, 1);
                NodeTraits::construct(alloc_, new_node, val);
                tail->next = new_node;
                new_node->prev = tail;
                base_.prev = new_node;
                new_node->next = &base_;
                tail = new_node;
                ++size_;
            }
        } catch (...) {
            NodeTraits::deallocate(alloc_, new_node, 1);
            clear();
            throw;
        }
    }

    List(const List& other)
        : alloc_(AllocTraits::select_on_container_copy_construction(other.get_allocator())),
          size_(0) {
        ListNode* new_node = nullptr;
        try {
            BaseNode* tail = &base_;
            tail->prev = tail;
            tail->next = tail;
            auto iter = other.cbegin();
            for (size_t i = 0; i < other.size_; ++i) {
                new_node = NodeTraits::allocate(alloc_, 1);
                NodeTraits::construct(alloc_, new_node, *iter);
                tail->next = new_node;
                new_node->prev = tail;
                base_.prev = new_node;
                new_node->next = &base_;
                tail = new_node;
                ++size_;
                ++iter;
            }
        } catch (...) {
            NodeTraits::deallocate(alloc_, new_node, 1);
            clear();
            throw;
        }
    }

    ~List() {
        clear();
    }

    List& operator=(const List& other) {
        if (AllocTraits::propagate_on_container_copy_assignment::value) {
            alloc_ = other.get_allocator();
        }
        List copy(alloc_);
        auto other_iter = other.cbegin();
        for (size_t i = 0; i < other.size_; ++i) {
            copy.push_back(*other_iter);
            ++other_iter;
        }
        swap(copy);
        return *this;
    }

    Allocator get_allocator() const {
        return Allocator(alloc_);
    }

    void push_back(const T& val) {
        insert(end(), val);
    }
    void push_front(const T& val) {
        insert(begin(), val);
    };
    void pop_back() {
        erase(--end());
    };
    void pop_front() {
        erase(begin());
    };

    void insert(const_iterator iter, const T& val) {
        auto node = const_cast<BaseNode*>(iter.get_node());  // NOLINT
        auto prev_node = node->prev;
        ListNode* new_node = nullptr;
        try {
            new_node = NodeTraits::allocate(alloc_, 1);
            NodeTraits::construct(alloc_, new_node, val);
            prev_node->next = static_cast<BaseNode*>(new_node);
            new_node->prev = prev_node;
            node->prev = static_cast<BaseNode*>(new_node);
            new_node->next = node;
            ++size_;
        } catch (...) {
            NodeTraits::deallocate(alloc_, new_node, 1);
            throw;
        }
    }

    void erase(const_iterator iter) {
        auto node = const_cast<BaseNode*>(iter.get_node());  // NOLINT
        auto prev_node = node->prev;
        auto next_node = node->next;
        prev_node->next = next_node;
        next_node->prev = prev_node;
        NodeTraits::destroy(alloc_, static_cast<ListNode*>(node));
        NodeTraits::deallocate(alloc_, static_cast<ListNode*>(node), 1);
        --size_;
    }

    iterator begin() {
        return iterator(base_.next);
    }
    const_iterator cbegin() const {
        return const_iterator(base_.next);
    }
    const_iterator begin() const {
        return cbegin();
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }
    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator rbegin() const {
        return crbegin();
    }

    iterator end() {
        return iterator(&base_);
    }
    const_iterator cend() const {
        return const_iterator(&base_);
    };
    const_iterator end() const {
        return cend();
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    };
    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    };
    const_reverse_iterator rend() const {
        return crend();
    };

    size_t size() const {
        return size_;
    }
};
