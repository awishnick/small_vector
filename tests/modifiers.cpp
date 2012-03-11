#include "small_vector.h"
#include "allocator_wrapper.h"
#include "gtest/gtest.h"

// 23.3.6.5
// Causes reallocation if the new size is greater than the old capacity
TEST(push_back, reallocation) {
  typedef allocator_wrapper< std::allocator<int> > allocator_type;
  allocator_type::NumAllocs() = 0;
  small_vector<int, 4, allocator_type> vec;

  // Push 4 elements. No resizing should happen
  EXPECT_EQ(4u, vec.capacity());
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);
  vec.push_back(4);
  EXPECT_EQ(4u, vec.capacity());
  EXPECT_EQ(4u, vec.size());
  EXPECT_EQ(0u, allocator_type::NumAllocs());

  // Now push back another element. Capacity should increase
  // and be greater than size. The old elements should be preserved.
  vec.push_back(5);
  EXPECT_GT(vec.capacity(), 4u);
  EXPECT_EQ(5u, vec.size());
  EXPECT_EQ(1u, allocator_type::NumAllocs());
  for (int i=0; i<5; ++i) EXPECT_EQ(i+1, vec[i]);
}
