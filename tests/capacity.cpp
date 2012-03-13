#include "small_vector.h"
#include "gtest/gtest.h"
#include <limits>

// max_size() returns the largest possible value of
// distance(begin(), end()). Right now just use the numeric limit
TEST(capacity, max_size) {
  typedef small_vector<int, 3> vec_type;
  vec_type vec;
  EXPECT_EQ(vec.max_size(),
            std::numeric_limits<vec_type::size_type>::max());
}
