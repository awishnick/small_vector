#pragma once

#include <cstddef>      // std::size_t, std::ptrdiff_t
#include <memory>       // std::allocator
#include <iostream>
template <class T, std::size_t SmallSize>
class small_vector_storage {
protected:
  T* small_begin() {
    return reinterpret_cast<T*>(&m_storage[0]);
  }
  const T* small_begin() const {
    return reinterpret_cast<const T*>(&m_storage[0]);
  }

  T* small_end() {
    return small_begin() + SmallSize;
  }
  const T* small_end() const {
    return small_begin() + SmallSize;
  }

  char m_storage[sizeof(T)*SmallSize];
};

template <class T>
class small_vector_storage<T, 0> {
protected:
  T* small_begin() { return NULL; }
  T* small_begin() const { return NULL; }
  T* small_end() const { return NULL; }
};

template <class T,
          std::size_t SmallSize,
          class Allocator = std::allocator<T> >
class small_vector : small_vector_storage<T, SmallSize>,
                     Allocator {
  typedef small_vector_storage<T, SmallSize> storage_base;
public:
  typedef T                   value_type;
  typedef Allocator           allocator_type;
  typedef value_type&         reference;
  typedef const value_type&   const_reference;
  typedef T*                  iterator;
  typedef const T*            const_iterator;
  typedef std::size_t         size_type;
  typedef std::ptrdiff_t      difference_type;
  // Check the standard on these two. Supposed to use std::allocator_traits
  typedef T*                  pointer;
  typedef const T*            const_pointer;
  typedef std::reverse_iterator<iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  // 23.3.6.2, construct/copy/destroy:
  explicit small_vector(const Allocator& allocator = Allocator()) :
    Allocator(allocator),
    m_begin(storage_base::small_begin()),
    m_end(storage_base::small_begin()),
    m_capacity_end(storage_base::small_end()) {
  }

  explicit small_vector(size_type n) :
    m_begin(storage_base::small_begin()),
    m_end(storage_base::small_begin()),
    m_capacity_end(storage_base::small_end()) {

    // If n is greater than the small size, allocate
    // memory first. Otherwise we can use our small storage.
    if (n > SmallSize) {
      m_begin = Allocator::allocate(n);
      m_capacity_end = m_begin + n;
    }
    m_end = m_begin + n;

    // Fill our range with a default-constructed value
    uninitialized_fill(m_begin, m_end, T());
  }

  small_vector(size_type n, const T& value,
               const Allocator& = Allocator() ) :
    m_begin(storage_base::small_begin()),
    m_end(storage_base::small_begin()),
    m_capacity_end(storage_base::small_end()) {

    // If n is greater than the small size, allocate
    // memory first. Otherwise we can use our small storage.
    if (n > SmallSize) {
      m_begin = Allocator::allocate(n);
      m_capacity_end = m_begin + n;
    }
    m_end = m_begin + n;

    // Fill our range with a default-constructed value
    uninitialized_fill(m_begin, m_end, value);
  }

  template <class InputIterator>
  small_vector(InputIterator first, InputIterator last,
               const Allocator& = Allocator()) :
    m_begin(storage_base::small_begin()),
    m_end(storage_base::small_begin()),
    m_capacity_end(storage_base::small_end()) {

    typedef
      typename std::iterator_traits<InputIterator>::iterator_category
      iterator_category;
    range_construct(first, last, iterator_category());
  }

  ~small_vector() {
    // Destroy our objects
    destroy_range(m_begin, m_end);
    // Free our memory if not using the small storage
    if (!is_small()) {
      Allocator::deallocate(m_begin, capacity());
    }
  }

  // 23.3.6.3, capacity:
  size_type size() const { return m_end - m_begin; }
  size_type capacity() const { return m_capacity_end - m_begin; }
  bool empty() const { return m_begin == m_end; }

  // element access:
  reference operator[](size_type n) {
    return m_begin[n];
  }
  const_reference operator[](size_type n) const {
    return m_begin[n];
  }

  // 23.3.6.5, modifiers:
  void push_back(const T& x) {
    // If we do not have enough capacity, reallocate
    const size_type new_size = size() + 1;
    if (new_size > capacity()) {
      // Reallocate a bigger array, but make it still have size()
      // elements, not size() + 1

      // This could throw bad_alloc
      const size_type new_capacity = 2 * capacity();
      T* new_begin = Allocator::allocate(new_capacity);

      // Copy-construct elements. If the copy constructor throws,
      // we'll delete our new array and rethrow
      try {
        T* old_elem = m_begin;
        for( T* new_elem = new_begin;
             old_elem != m_end;
             ++new_elem, ++old_elem ) {
          Allocator::construct(new_elem, *old_elem);
        }
      } catch (...) {
        Allocator::deallocate(new_begin, 0);
        throw;
      }

      // Now use the new array and free the old one
      const size_type old_size = size();
      T* old_begin = m_begin,
       * old_end = m_end;
      const bool was_small = is_small();

      m_begin = new_begin;
      m_end = new_begin + old_size;
      m_capacity_end = new_begin + new_capacity;

      // Destruct all the old instances. Only free memory if
      // it's not from our small backing storage
      destroy_range(old_begin, old_end);
      if (!was_small) {
        Allocator::deallocate(old_begin, 0);
      }

    }

    // Now just construct the new element
    Allocator::construct(m_end, x);
    ++m_end;
  }

private:
  T* m_begin,
   * m_end,
   * m_capacity_end;

  // Returns whether we're using our small storage
  bool is_small() const { return m_begin == storage_base::small_begin(); }

  // Initializes the range [first, last) to value. Doesn't destruct the
  // range because it assumes that no objects have been constructed there.
  void uninitialized_fill(T* first, T* last, const T& value) {
    for( ; first != last; ++first ) {
      Allocator::construct(first, value);
    }
  }

  // Destroys the objects in the range [first, last)
  void destroy_range(T* first, T* last) {
    for( ; first != last; ++first ) {
      Allocator::destroy( first );
    }
  }

  // Range construct for multi-pass iterators
  template <class Iterator>
  void range_construct_multipass(Iterator first, Iterator last) {
    // Allocate space
    const size_type n = std::distance(first, last);
    if (n > SmallSize) {
      m_begin = Allocator::allocate(n);
      m_capacity_end = m_begin + n;
    }
    m_end = m_begin + n;

    // Copy construct the range
    for ( T* elem = m_begin; first != last; ++first, ++elem) {
      Allocator::construct(elem, *first);
    }
  }

  template <class ForwardIterator>
  void range_construct(ForwardIterator first, ForwardIterator last,
                       std::forward_iterator_tag) {
    range_construct_multipass(first, last);
  }

  template <class BidirectionalIterator>
  void range_construct(BidirectionalIterator first, BidirectionalIterator last,
                       std::bidirectional_iterator_tag) {
    range_construct_multipass(first, last);
  }

  template <class RandomAccessIterator>
  void range_construct(RandomAccessIterator first, RandomAccessIterator last,
                       std::random_access_iterator_tag) {
    range_construct_multipass(first, last);
  }

  // Range construct for input iterators
  template <class InputIterator>
  void range_construct(InputIterator first, InputIterator last,
                       std::input_iterator_tag) {
    for( ; first != last; ++first ) {
      push_back( *first );
    }
  }
};

