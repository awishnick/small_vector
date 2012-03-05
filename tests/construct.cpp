#include "small_vector.h"
#include "gtest/gtest.h"

namespace {
  unsigned NumMockObjs = 0;
  class MockObj {
  public:
    MockObj() : m_n(42) { ++NumMockObjs; }
    explicit MockObj(int n) : m_n(n) { ++NumMockObjs; }
    MockObj(const MockObj& rhs) : m_n(rhs.m_n) { ++NumMockObjs; }
    MockObj& operator=(const MockObj& rhs) { m_n = rhs.m_n; return *this; }
    ~MockObj() { --NumMockObjs; }
    int m_n;
    bool operator==(const MockObj& rhs) const { return m_n == rhs.m_n; }
  };
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
