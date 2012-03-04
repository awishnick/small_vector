#pragma once

#include <cstddef>      // size_t
#include <memory>       // std::allocator


template <class T,
          std::size_t SmallSize,
          class Alloc = std::allocator<T> >
class small_vector {
};

