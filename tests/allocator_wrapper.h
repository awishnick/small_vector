#pragma once

// Wraps an allocator and monitors calls to allocate, etc
template <class Allocator>
class allocator_wrapper {
public:
  typedef typename Allocator::size_type       size_type;
  typedef typename Allocator::difference_type difference_type;
  typedef typename Allocator::pointer         pointer;
  typedef typename Allocator::const_pointer   const_pointer;
  typedef typename Allocator::reference       reference;
  typedef typename Allocator::const_reference const_reference;
  typedef typename Allocator::value_type      value_type;
  template <class U>
  struct rebind { typedef allocator_wrapper<Allocator> other; };

  allocator_wrapper() {}
  allocator_wrapper(const allocator_wrapper<Allocator>& rhs) :
    m_allocator(rhs.m_allocator) {}
  ~allocator_wrapper() {}

  pointer address(reference x) { return m_allocator.address(x); }
  const_pointer address(reference x) const { return m_allocator.address(x); }

  pointer allocate(
      size_type n, std::allocator<void>::const_pointer hint = 0) {
    ++NumAllocs();
    return m_allocator.allocate(n, hint);
  }
  void deallocate(pointer p, size_type n) {
    m_allocator.deallocate(p, n);
  }
  size_type max_size() const { return m_allocator.max_size(); }

  void construct(pointer p, const value_type& v) {
    return m_allocator.construct(p, v);
  }
  void destroy(pointer p) {
    return m_allocator.destroy(p);
  }

  static unsigned& NumAllocs() {
    static unsigned _NumAllocs;
    return _NumAllocs;
  }
private:
  Allocator m_allocator;
};

