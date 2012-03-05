#pragma once

#include <cstddef>      // std::size_t, std::ptrdiff_t
#include <memory>       // std::allocator

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

  explicit small_vector(size_type n, const T& value,
                        const Allocator& = Allocator() ) :
    m_begin(storage_base::small_begin()),
    m_end(storage_base::small_end()),
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
private:
  T* m_begin,
   * m_end,
   * m_capacity_end;

  // Returns whether we're using our small storage
  bool is_small() const { return m_begin == storage_base::small_begin(); }

  // Initializes the range [begin, end) to value. Doesn't destruct the
  // range because it assumes that no objects have been constructed there.
  void uninitialized_fill(T* begin, T* end, const T& value) {
    for( ; begin != end; ++begin ) {
      Allocator::construct(begin, value);
    }
  }

  // Destroys the objects in the range [begin, end)
  void destroy_range(T* begin, T* end) {
    for( ; begin != end; ++begin ) {
      Allocator::destroy( begin );
    }
  }
};

