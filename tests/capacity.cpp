#include "small_vector.h"
#include "allocator_wrapper.h"
#include "gtest/gtest.h"
#include <limits>

// max_size() returns the largest possible value of
// distance(begin(), end()). Right now just use the numeric limit
TEST(max_size, is_defined) {
  typedef small_vector<int, 3> vec_type;
  vec_type vec;
  EXPECT_EQ(vec.max_size(),
            std::numeric_limits<vec_type::size_type>::max());
}

// capacity() is the total number of elements that the
// vector can hold without requiring reallocation
TEST(capacity, no_reallocation) {
  // Test with a small size of 0
  {
    typedef allocator_wrapper< std::allocator<int> > allocator_type;
    allocator_type::NumAllocs() = 0;
    small_vector<int, 0, allocator_type> vec;
    std::size_t capacity = vec.capacity();
    for (unsigned i=0; i<capacity; ++i) vec.push_back(1);
    EXPECT_EQ(0u, allocator_type::NumAllocs());

    vec.push_back(2);
    EXPECT_EQ(1u, allocator_type::NumAllocs());
    ASSERT_GE(vec.capacity(), vec.size());
    unsigned NumToInsert = vec.size() - vec.capacity();
    for (unsigned i=0; i<NumToInsert; ++i) vec.push_back(1);
    EXPECT_EQ(1u, allocator_type::NumAllocs());

    vec.push_back(2);
    EXPECT_EQ(2u, allocator_type::NumAllocs());
  }
}
