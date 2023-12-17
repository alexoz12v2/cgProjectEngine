#pragma once

#include "Core/Alloc.h"
#include "Core/Type.h"

#include <array>

namespace cge
{

template<typename T, U32_t size> using Array = std::array<T, size>;

template<typename T> class ArrayView
{
  public:
    // Types
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    // Constructors
    constexpr ArrayView() noexcept : m_data(nullptr), m_size(0) {}

    constexpr ArrayView(T* data, size_type size) noexcept
      : m_data(data), m_size(size)
    {
    }

    // Element access
    constexpr reference operator[](size_type index) { return m_data[index]; }

    constexpr const_reference operator[](size_type index) const
    {
        return m_data[index];
    }

    constexpr reference front() { return m_data[0]; }

    constexpr const_reference front() const { return m_data[0]; }

    constexpr reference back() { return m_data[m_size - 1]; }

    constexpr const_reference back() const { return m_data[m_size - 1]; }

    constexpr T* data() noexcept { return m_data; }

    constexpr const T* data() const noexcept { return m_data; }

    // Capacity
    constexpr bool empty() const noexcept { return m_size == 0; }

    constexpr size_type size() const noexcept { return m_size; }

    // Iterators
    constexpr pointer begin() noexcept { return m_data; }

    constexpr const_pointer begin() const noexcept { return m_data; }

    constexpr const_pointer cbegin() const noexcept { return m_data; }

    constexpr pointer end() noexcept { return m_data + m_size; }

    constexpr const_pointer end() const noexcept { return m_data + m_size; }

    constexpr const_pointer cend() const noexcept { return m_data + m_size; }

  private:
    T*        m_data;
    size_type m_size;
};
} // namespace cge
