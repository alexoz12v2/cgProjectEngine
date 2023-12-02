#pragma once

#include "Core/Alloc.h"
#include "Core/Type.h"

#include <array>

template<typename T, U32_t size> using Array = std::array<T, size>;
