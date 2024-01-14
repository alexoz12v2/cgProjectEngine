#pragma once
/**
 * @file Core/Type.h
 * contains all necessary fundamental type definitions
 */
#include <concepts>
#include <cstdint>
#include <memory>
#include <type_traits>

#include <glm/glm.hpp>

/// @note we only support x86_64. No macro is needed
#include <immintrin.h>

#include "MacroDefs.h"

namespace cge
{


/// @typedef built-in types
using U8_t  = uint8_t;
using U16_t = uint16_t;
using U32_t = uint32_t;
using U64_t = uint64_t;
using I8_t  = int8_t;
using I16_t = int16_t;
using I32_t = int32_t;
using I64_t = int64_t;

using B8_t  = bool;
using F32_t = float;
using F64_t = double;

using UChar8_t  = unsigned char;
using Byte_t    = UChar8_t;
using Char8_t   = char;
using CharU8_t  = char8_t;
using CharU16_t = char16_t;
using CharU32_t = char32_t;

/// @typedef vector registers
/// @note we only support x86_64. No macro is needed
/// @warning do not put these into structs!
using V128f_t = __m128;
using V128d_t = __m128d;
using V128i_t = __m128i;

// assume AVX512 not supported, therefore don't use bfloat16 vecs
using V256f_t = __m256;
using V256d_t = __m256d;
using V256i_t = __m256i;

union V128_t
{
    V128f_t f;
    V128d_t d;
    V128i_t i;
};

union V256_t
{
    V256f_t f;
    V256d_t d;
    V256i_t i;
};

/// @enum Error type
// to add more when needed ...
enum class EErr_t : U32_t
{
    eSuccess,
    eMemory,
    eGeneric,
    eCreationFailure,
    eInvalid
};

/// @struct TypePack definition
template<typename... Ts> struct TypePack
{
    static U64_t constexpr count = sizeof...(Ts);
};

/// @struct TypePack Operations
template<typename T, typename... Ts> struct indexOf
{
    static I32_t constexpr count = 0;
    static_assert(!std::is_same_v<T, T>, "Type not present in TypePack");
};

/// @struct TypePack Operations
template<typename T, typename... Ts> struct indexOf<T, TypePack<T, Ts...>>
{
    static I32_t constexpr count = 0;
};

/// @struct TypePack Operations
template<typename T, typename U, typename... Ts>
struct indexOf<T, TypePack<U, Ts...>>
{
    static I32_t constexpr count = 1 + indexOf<T, TypePack<Ts...>>::count;
};

/// @struct TypePack Operations
template<U32_t idx, typename T, typename... Ts> struct at
{
    static_assert(!std::is_same_v<T, T>, "at requires parameter a TypePack");
};

/// @struct TypePack Operations
template<U32_t idx, typename T, typename... Ts>
struct at<idx, TypePack<T, Ts...>>
{
    using type = at<idx - 1, TypePack<Ts...>>::type;
};

/// @struct TypePack Operations
template<typename T, typename... Ts> struct at<0, TypePack<T, Ts...>>
{
    using type = T;
};


// Computing return type of a polymorphic function: We cannot use just
// std::invoke_result_r, because we want to check at compile time that calling
// that function for all types in the Pack yields the same type
template<typename... Ts> struct SameType;
template<typename T, typename... Ts> struct SameType<T, Ts...>
{
    using type = T;
    static_assert(
      std::is_same_v<T, Ts...>,
      "Not all types in pack are the same");
};

template<typename F, typename... Ts> struct ReturnType
{
    using type =
      typename SameType<typename std::invoke_result_t<F, Ts *>...>::type;
};

template<typename F, typename... Ts> struct ReturnTypeConst
{
    using type =
      typename SameType<typename std::invoke_result_t<F, const Ts *>...>::type;
};

namespace detail
{
    template<typename F, typename R, typename T>
    R dispatch(F &&func, void const *ptr, I32_t index)
    {
        return func((T const *)ptr);
    }

    template<typename F, typename R, typename T0, typename T1>
    R dispatch(F &&func, void const *ptr, I32_t index)
    {
        switch (index)
        {
        case 0:
            return func((T0 const *)ptr);
        case 1:
            return func((T1 const *)ptr);
        }

        CGE_unreachable();
    }

    template<
      typename F,
      typename R,
      typename T0,
      typename T1,
      typename T2,
      typename... Ts>
    R dispatch(F &&func, void const *ptr, I32_t index)
    {
        switch (index)
        {
        case 0:
            return func((T0 const *)ptr);
        case 1:
            return func((T1 const *)ptr);
        default:
            return dispatch<F, R, Ts...>(func, ptr, index);
        }
    }
} // namespace detail

/** @class TaggedPointer from pbrt-v4
 * @Brief pointer to a polymorphic type, whose type id is embedded in the
 * pointer itself
 */
template<typename... Ts> class TaggedPointer
{
  public:
    using Types = TypePack<Ts...>;

    TaggedPointer() = default;

    template<typename T> TaggedPointer(T *ptr)
    {
        U32_t constexpr type = typeIndex<T>();
        U64_t iptr           = reinterpret_cast<U64_t>(ptr);
        bits                 = iptr | ((U64_t)type << tagShift);
    }

    template<typename F> auto dispatch(F &&func) -> decltype(auto)
    {
        using R = typename ReturnType<F, Ts...>::type;

        // tag is 1-based, as 0 = nullptr, while typepack index is 0-based
        return detail::dispatch<F, R, Ts...>(func, ptr(), tag() - 1);
    }

    template<typename F> auto dispatch(F &&func) const -> decltype(auto)
    {
        using R = typename ReturnTypeConst<F, Ts...>::type;

        // tag is 1-based, as 0 = nullptr, while typepack index is 0-based
        return detail::dispatch<F, R, Ts...>(func, ptr(), tag() - 1);
    }

    template<typename T> static auto constexpr typeIndex() -> U32_t
    {
        using Tp = typename std::remove_cv_t<T>;
        if constexpr (std::is_same_v<Tp, std::nullptr_t>)
            return 0;
        else
            return 1 + indexOf<Tp, Types>::count;
    }

    auto tag() const -> U32_t { return (bits & tagMask) >> tagShift; }

    auto ptr() -> void * { return reinterpret_cast<void *>(bits & ptrMask); }
    auto ptr() const -> void const *
    {
        return reinterpret_cast<void const *>(bits & ptrMask);
    }

  private:
    // tag is stored from bit 63 to bit 58 of the 64-bit wide address
    static I32_t constexpr tagShift = 57;
    static I32_t constexpr tagBits  = 64 - tagShift;

    // let low7Bits = ((1ull << tagBits) - 1) has the 7 least significant bits
    // set
    static U64_t constexpr tagMask = ((1ull << tagBits) - 1) << tagShift;
    static U64_t constexpr ptrMask = ~tagMask;

    U64_t bits;
};

template<typename T> struct RemoveConstPointer;
template<typename T> struct RemoveConstPointer<T const *>
{
    using type = T *;
};
template<typename T> struct RemoveConstPointer<T *>
{
    using type = T *;
};

/** @class TaggedStruct
 * @Brief polymorphic type, stored inline with an index. Dangerous, but could be
 * useful
 */
template<
  U64_t freeSpaceSize,
  U64_t freeSpaceAlignment,
  B8_t  executeDestructor,
  typename... Ts>
class TaggedStruct
{
  public:
    using Types    = TypePack<Ts...>;
    TaggedStruct() = default;

    template<typename T>
        requires(sizeof(T) <= freeSpaceSize * sizeof(U8_t))
    TaggedStruct(T &&copy) : m_index(typeIndex<T>())
    {
        std::construct_at(
          reIerpret_cast<std::remove_reference_t<T> *>(data),
          std::forward<T>(copy));
    }

    template<U32_t idx>
        requires(sizeof(at<idx, Types>::type) <= freeSpaceSize * sizeof(U8_t))
    TaggedStruct() : m_index(idx), data(at<idx, Types>::type())
    {
    }

    ~TaggedStruct()
        requires(executeDestructor)
    {
        auto f = [&]<typename T>(T *ptr)
        { std::destroy_at(reIerpret_cast<T>(data)); };
        dispatch(f);
    }

    ~TaggedStruct()
        requires(!executeDestructor)
    = default;


    template<typename F> auto dispatch(F &&func) -> decltype(auto)
    {
        using R = typename ReturnType<F, Ts...>::type;

        return detail::dispatch<F, R, Ts...>(func, (void *)&data, m_index);
    }

    template<typename F> auto dispatch(F &&func) const -> decltype(auto)
    {
        using R = typename ReturnTypeConst<F, Ts...>::type;

        return detail::dispatch<F, R, Ts...>(
          func, (void const *)&data, m_index);
    }

    template<typename T> static auto constexpr typeIndex() -> U32_t
    {
        using Tp = typename std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr (std::is_same_v<Tp, std::nullptr_t>)
            return 0;
        else
            return 1 + indexOf<Tp, Types>::count;
    }

  public:
    alignas(freeSpaceAlignment) U8_t data[freeSpaceSize];

  private:
    U8_t m_index;
};

/** Utilities */

template<typename M>
concept Monad = requires(M m) {
    typename M::inner;

    {
        m.unit(std::declval<M::inner>())
    } -> std::same_as<M>;

    requires requires(typename M::inner t) {
        {
            m.bind(
              []<typename M1>(typename M::inner b) -> typename M1::inner
              { return std::declval<M1>(); })
        } -> std::same_as<M>;
    };
};

struct AABB_t
{
    glm::vec3 min;
    glm::vec3 max;
};
inline AABB_t aUnion(AABB_t a, AABB_t b)
{
    AABB_t const c{ glm::min(a.min, b.min), glm::min(a.max, b.max) };
    return c;
}

// 00 -> x, 01 -> y, 10 -> z
inline int32_t largestAxis(AABB_t box, float *plane)
{
    glm::vec3 diag = box.max - box.min;
    glm::vec3 mid  = (box.max + box.min) * 0.5f;
    int32_t   res  = 0;
    *plane         = mid.x;
    if (diag.x < diag.y)
    {
        res    = 1;
        *plane = mid.y;
        if (diag.y < diag.z)
        {
            *plane = mid.z;
            res    = 2;
        }
    }
    else if (diag.x < diag.z)
    {
        *plane = mid.z;
        res    = 2;
    }

    return res;
}

inline float aArea(AABB_t a)
{
    glm::vec3 const d = a.max - a.min;
    return 2.f * (d.x * d.y + d.y * d.z + d.z * d.x);
}

inline glm::vec3 centroid(AABB_t b) { return (b.max + b.min) * 0.5f; }
inline glm::vec3 diagonal(AABB_t b) { return b.max - b.min; }

struct Ray_t
{
    glm::vec3 o;
    glm::vec3 d;
};

// preliminary intersection test on AABB
inline bool testOverlap(Ray_t const &ray, AABB_t const &box)
{
    // for each dimension, ray plane intersection
    for (int i = 0; i < 3; i++)
    {
        // if ray is parallel to the slab...
        if (fabsf(ray.d[i]) < std::numeric_limits<F32_t>::epsilon())
        {
            // ... and ray coord is not within, no hit
            // does this work with NaN?
            if (ray.o[i] < box.min[i] || ray.o[i] > box.max[i])
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace cge
