#include "small_vector.h"
#include "gtest/gtest.h"
#include "allocator_wrapper.h"
#include <list>
#include <vector>

namespace {
  unsigned NumMockObjs = 0;
  unsigned NumCopyConstructs = 0;
  class MockObj {
  public:
    MockObj() : m_n(42) { ++NumMockObjs; }
    explicit MockObj(int n) : m_n(n) { ++NumMockObjs; }
    MockObj(const MockObj& rhs) : m_n(rhs.m_n) {
      ++NumMockObjs;
      ++NumCopyConstructs;
    }
    MockObj& operator=(const MockObj& rhs) { m_n = rhs.m_n; return *this; }
    ~MockObj() { --NumMockObjs; }
    int m_n;
    bool operator==(const MockObj& rhs) const { return m_n == rhs.m_n; }
  };

  // Useful for testing that all MockObjs have been destroyed
  // at scope exit
  class MockObjLeakSentry {
  public:
    MockObjLeakSentry() : ObjCount(NumMockObjs) {}
    ~MockObjLeakSentry() {
      EXPECT_EQ(ObjCount, NumMockObjs);
    }
  private:
    unsigned ObjCount;
  };

  // Wraps an iterator and ony allows operations
  // that a forward iterator supports
  template <class Iterator>
  class forward_iterator_wrapper :
    public std::iterator<std::forward_iterator_tag,
                         typename Iterator::value_type> {
  public:
    typedef forward_iterator_wrapper<Iterator> self_type;
    typedef std::iterator<std::forward_iterator_tag,
                          typename Iterator::value_type> base_type;

    typedef typename base_type::value_type          value_type;
    typedef typename base_type::difference_type     difference_type;
    typedef typename base_type::pointer             pointer;
    typedef typename base_type::reference           reference;
    typedef typename base_type::iterator_category   iterator_category;

    // Initialize and wrap an iterator
    forward_iterator_wrapper(Iterator it) : m_it(it) {}

    // 24.2.2, iterator requirements
    reference operator*() const { return *m_it; }
    self_type& operator++() { ++m_it; return *this; }

    // 24.2.3, input iterator requirements
    // 24.2.4, output iterator requirements
    bool operator==( const self_type& rhs ) { return m_it == rhs.m_it; }
    bool operator!=( const self_type& rhs ) { return m_it != rhs.m_it; }
    pointer operator->() const { return &*m_it; }
    self_type operator++(int) { self_type tmp= *this; ++*this; return tmp; }

  private:
    Iterator m_it;  
  };

  template <class Iterator>
  forward_iterator_wrapper<Iterator> forward_iterator_wrap( Iterator it ) {
    return forward_iterator_wrapper<Iterator>(it);
  }

  // Wraps an iterator and only allows operations
  // that an input iterator supports
  template <class Iterator>
  class input_iterator_wrapper :
    public std::iterator<std::input_iterator_tag,
                         typename Iterator::value_type> {
  public:
    typedef input_iterator_wrapper<Iterator> self_type;
    typedef std::iterator<std::input_iterator_tag,
                          typename Iterator::value_type> base_type;
    
    typedef typename base_type::value_type          value_type;
    typedef typename base_type::difference_type     difference_type;
    typedef typename base_type::pointer             pointer;
    typedef typename base_type::reference           reference;
    typedef typename base_type::iterator_category   iterator_category;

    // Initialize and wrap an iterator
    input_iterator_wrapper(Iterator it) : m_it(it) {}

    // 24.2.2, iterator requirements
    const value_type& operator*() const { return *m_it; }
    self_type& operator++() { ++m_it; return *this; }

    // 24.2.3, input iterator requirements
    bool operator==( const self_type& rhs ) { return m_it == rhs.m_it; }
    bool operator!=( const self_type& rhs ) { return m_it != rhs.m_it; }
    pointer operator->() const { return &*m_it; }
    self_type operator++(int) { self_type tmp= *this; ++*this; return tmp; }

  private:
    Iterator m_it;  
  };

  template <class Iterator>
  input_iterator_wrapper<Iterator> input_iterator_wrap( Iterator it ) {
    return input_iterator_wrapper<Iterator>(it);
  }
                        
}

// Make sure small_vector works when the small size
// is 0. This should be equivalent to a regular vector.
TEST(small_vector, zero_size) {
  small_vector<int, 0> vec;
  (void)vec;
}

// Default construct
// Effects: Constructs an empty vector, using the specified allocator
// Complexity: Constant
TEST(small_vector, default_construct) {
  small_vector<MockObj, 16> vec;
  EXPECT_EQ(0u, vec.size());
  EXPECT_TRUE(vec.empty());
  EXPECT_EQ(16u, vec.capacity());

  // We shouldn't construct any objects
  EXPECT_EQ(0u, NumMockObjs);
}

// Construct with size
// Effects: Constructs a vector with n value-initialized elements.
// Requires: T shall be DefaultConstructible
// Complexity: Linear in n
TEST(small_vector, size_construct) {
  // Test the case where we initialize to a size smaller than our small size
  {
    small_vector<MockObj, 16> vec(5u);
    EXPECT_EQ(5u, vec.size());
    EXPECT_EQ(16u, vec.capacity());
    EXPECT_EQ(NumMockObjs, vec.size());
  }

  // Test the case where we initialize to exactly our small size
  {
    small_vector<MockObj, 16> vec(16u);
    EXPECT_EQ(16u, vec.size());
    EXPECT_EQ(16u, vec.capacity());
    EXPECT_EQ(NumMockObjs, vec.size());
  }

  // Test the case where we initialize to a size larger than our small size
  {
    small_vector<MockObj, 16> vec(17u);
    EXPECT_EQ(17u, vec.size());
    // The growth strategy isn't specified, but we need to increase
    // our capacity.
    EXPECT_GT(vec.capacity(), 16u);
    EXPECT_EQ(NumMockObjs, vec.size());
  }
}

// Construct with size and value
// Effects: Constructs a vector with n copies of value, using the
// specified allocator.
// Requires: T shall be CopyInsertable into *this
// Complexity: Linear in n
TEST(small_vector, value_construct) {
  MockObj mock(3);

  // Test the case where we initialize to a size smaller than our small size
  {
    small_vector<MockObj, 16> vec(5u, mock);
    EXPECT_EQ(5u, vec.size());
    EXPECT_EQ(16u, vec.capacity());
    EXPECT_EQ(NumMockObjs, vec.size()+1);
    for( int i=0, s=vec.size(); i<s; ++i ) EXPECT_EQ(vec[i], mock);
  }

  // Test the case where we initialize to exactly our small size
  {
    small_vector<MockObj, 16> vec(16u, mock);
    EXPECT_EQ(16u, vec.size());
    EXPECT_EQ(16u, vec.capacity());
    EXPECT_EQ(NumMockObjs, vec.size()+1);
    for( int i=0, s=vec.size(); i<s; ++i ) EXPECT_EQ(vec[i], mock);
  }

  // Test the case where we initialize to a size larger than our small size
  {
    small_vector<MockObj, 16> vec(17u, mock);
    EXPECT_EQ(17u, vec.size());
    // The growth strategy isn't specified, but we need to increase
    // our capacity.
    EXPECT_GT(vec.capacity(), 16u);
    EXPECT_EQ(NumMockObjs, vec.size()+1);
    for( int i=0, s=vec.size(); i<s; ++i ) EXPECT_EQ(vec[i], mock);
  }
}

// Construct with a range
// Effects: Constructs a vector equal to the range [first, last) using
// the specified allocator.
// Complexity: Makes only N calls to the copy constructor of T (where N is the
// distance between first and last) and no reallocations if iterators first and
// last are of forward, bidirectional, or random access categories. It makes
// order N calls to the copy constructor of T and order log(N) reallocations
// if they are just input iterators.
namespace {
  // Testing for bidirectional, forward, and random access iterators.
  // The standard says, for these, there should be N copy constructions
  // and only one allocation (no reallocations)
  template <class Iterator>
  void TestRangeConstructOneAlloc(Iterator first, Iterator last) {
    typedef allocator_wrapper< std::allocator<MockObj> > allocator_type;

    // Test when smaller than small size
    {
      NumCopyConstructs = 0;
      allocator_type::NumAllocs() = 0;
      small_vector<MockObj, 16, allocator_type> vec(first, last);
      EXPECT_EQ(8u, NumCopyConstructs);
      EXPECT_EQ(16u, NumMockObjs);
      // We expect the allocator to never get called
      EXPECT_EQ(0u, allocator_type::NumAllocs());
    }

    // Test when equal to small size
    {
      NumCopyConstructs = 0;
      allocator_type::NumAllocs() = 0;
      small_vector<MockObj, 8, allocator_type> vec(first, last);
      EXPECT_EQ(8u, NumCopyConstructs);
      EXPECT_EQ(16u, NumMockObjs);
      // We expect the allocator to never get called
      EXPECT_EQ(0u, allocator_type::NumAllocs());
    }

    // Test when larger than small size
    {
      NumCopyConstructs = 0;
      allocator_type::NumAllocs() = 0;
      small_vector<MockObj, 4, allocator_type> vec(first, last);
      EXPECT_EQ(8u, NumCopyConstructs);
      EXPECT_EQ(16u, NumMockObjs);
      // The standard says the allocator should get called once
      EXPECT_EQ(1u, allocator_type::NumAllocs());
    }
  }
}
TEST(small_vector, range_construct) {
  // Test with a bidirectional iterator
  {
    std::list<MockObj> l;
    for (int i=0; i<8; ++i) l.push_back(MockObj());
    TestRangeConstructOneAlloc( l.begin(), l.end() );    
  }

  // Test with a forward iterator
  {
    std::list<MockObj> l;
    for (int i=0; i<8; ++i) l.push_back(MockObj());
    TestRangeConstructOneAlloc( forward_iterator_wrap(l.begin()),
                                forward_iterator_wrap(l.end()) );
  }

  // Test with a random access iterator
  {
    std::vector<MockObj> v;
    for (int i=0; i<8; ++i) v.push_back(MockObj());
    TestRangeConstructOneAlloc( v.begin(), v.end() );
  }

  // Test with an input iterator. We expect O(log n) allocations
  // and O(n) calls to the copy constructor
  {
    std::vector<MockObj> v;
    for (int i=0; i<8; ++i) v.push_back(MockObj());

    typedef allocator_wrapper< std::allocator<MockObj> > allocator_type;
    typedef input_iterator_wrapper<std::vector<MockObj>::iterator>
            iterator;

    iterator first = v.begin(),
             last = v.end();

    // Test when smaller than small size
    {
      MockObjLeakSentry LeakSentry;
      NumCopyConstructs = 0;
      allocator_type::NumAllocs() = 0;
      small_vector<MockObj, 16, allocator_type> vec(first, last);
      EXPECT_EQ(8u, NumCopyConstructs);
      EXPECT_EQ(16u, NumMockObjs);
      // We expect the allocator to never get called
      EXPECT_EQ(0u, allocator_type::NumAllocs());
    }

    // Test when equal to small size
    {
      MockObjLeakSentry LeakSentry;
      NumCopyConstructs = 0;
      allocator_type::NumAllocs() = 0;
      small_vector<MockObj, 8, allocator_type> vec(first, last);
      EXPECT_EQ(8u, NumCopyConstructs);
      EXPECT_EQ(16u, NumMockObjs);
      // We expect the allocator to never get called
      EXPECT_EQ(0u, allocator_type::NumAllocs());
    }

    // Test when larger than small size
    {
      MockObjLeakSentry LeakSentry;
      NumCopyConstructs = 0;
      allocator_type::NumAllocs() = 0;
      small_vector<MockObj, 4, allocator_type> vec(first, last);
      // Construct 4, then double capacity, copy-constructing a new 4.
      // Then construct 4 more.
      EXPECT_EQ(12u, NumCopyConstructs);
      EXPECT_EQ(16u, NumMockObjs);
      EXPECT_EQ(1u, allocator_type::NumAllocs());
    }
    
    // Test when much larger than small size
    {
      v.resize(33);
      first = v.begin();
      last = v.end();

      MockObjLeakSentry LeakSentry;
      NumCopyConstructs = 0;
      allocator_type::NumAllocs() = 0;

      small_vector<MockObj, 4, allocator_type> vec(first, last);
      // Construct 4
      // Reallocate to 8, constructing 4
      // Construct 4
      // Reallocate to 16, constructing 8
      // Construct 8
      // Reallocate to 32, constructing 16
      // Construct 16
      // Reallocate to 64, constructing 32
      // Construct 1

      EXPECT_EQ(93u, NumCopyConstructs);
      EXPECT_EQ(66u, NumMockObjs);
      EXPECT_EQ(4u, allocator_type::NumAllocs());
    }

  }
}

