/* 
TODO: Not too fuzzed about how this file is written. 
Needs a heavy refactor to: 
    A) Better support platforms that do not have floating point. E.g. rather
       than redefining what a float is as a fixed point have them be separate 
       types.
    B) Add support for SIMD where available. E.g. OG XBox.
    C) Prefer build in operators over function calls where possible.

We can make these performance gains later for now...

TODO: Fill out missing 2D transformations. 
TODO: Add missing matrix transpose and determinant. 
TODO: At mat34 type. Useful on NDS. 
TODO: We probably want quaternions. Too much of a headache to figure out for me right now... 
*/

#include <cassert>

#ifndef CORE_H
#define CORE_H

#define b8                              b8_t
#define i8                              i8_t
#define u8                              u8_t
#define i16                             i16_t
#define u16                             u16_t
#define i32                             i32_t
#define u32                             u32_t
#define i64                             i64_t
#define u64                             u64_t
#define sz                              sz_t
#define f32                             f32_t
#define f64                             f64_t
#define fvec2                           fvec2_t
#define fvec3                           fvec3_t
#define fvec4                           fvec4_t
#define fmat22                          fmat22_t
#define fmat33                          fmat33_t
#define fmat44                          fmat44_t

#define f32_pi 3.14159265359f
#define f64_pi 3.14159265358979323846
#define f32_rad_to_deg(i_radians) (i_radian * (180.0f / f32_pi))
#define f32_deg_to_rad(i_degrees) (i_degrees * (f32_pi / 180.0f))
#define f64_rad_to_deg(i_radians) (i_radian * (180.0 / f64_pi))
#define f64_deg_to_rad(i_degrees) (i_degrees * (f64_pi / 180.0))

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4201)
#elif defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
# endif

#if defined(i8_t) || defined(u8_t) || defined(i16_t) || defined(u16_t) || defined(i32_t) || defined(u32_t) || defined(i64_t) || defined(u64_t) || defined(sz_t)
    #if !defined(i8_t) || !defined(u8_t) || !defined(i16_t) || !defined(u16_t) || !defined(i32_t) || !defined(u32_t) || !defined(i64_t) || !defined(u64_t) || !defined(sz_t)
        #error "You must define all of i8_t, u8_t, i16_t, u16_t, i32_t, u32_t, i64_t, u64_t and sz_t."
    #endif
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L /* C99 */
    #include <stdint.h>
    typedef int8_t              i8_t;
    typedef uint8_t             u8_t;
    typedef int16_t             i16_t;
    typedef uint16_t            u16_t;
    typedef int32_t             i32_t;
    typedef uint32_t            u32_t;
    typedef int64_t             i64_t;
    typedef uint64_t            u64_t;
    typedef size_t              sz_t;
#elif defined(__cplusplus) && __cplusplus >= 201103L /* C++11 */
    #include <cstdint>
    typedef std::int8_t         i8_t;
    typedef std::uint8_t        u8_t;
    typedef std::int16_t        i16_t;
    typedef std::uint16_t       u16_t;
    typedef std::int32_t        i32_t;
    typedef std::uint32_t       u32_t;
    typedef std::int64_t        i64_t;
    typedef std::uint64_t       u64_t;
    typedef size_t              sz_t;
#else
    typedef char                i8_t;
    typedef unsigned char       u8_t;
    typedef short               i16_t;
    typedef unsigned short      u16_t;
    typedef long                i32_t;
    typedef unsigned long       u32_t;
    typedef long long           i64_t;
    typedef unsigned long long  u64_t;
    typedef unsigned long long  sz_t;
#endif

/* Provide custom or default implementation for booleans. */
#if defined(b8_t) || defined(TRUE) || defined(FALSE)
    #if !defined(b8_t) || !defined(TRUE) || !defined(FALSE)
        #error "You must define all of b8_t, TRUE and FALSE."
    #endif
#elif defined(__cplusplus) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) /* C++ or C99 */
    typedef bool            b8_t;
    #define TRUE       true
    #define FALSE      false
#else
    typedef u8_t       b8_t;
    #define TRUE       1
    #define FALSE      0
#endif

/* Provide custom or default implementation for floats. */
#if defined(f32_t) || defined(f64_t)
    #if !defined(f32_t) || !defined(f64_t)
        #error "You must define all of f32_t and f64_t."
    #endif
#else
    typedef float f32_t;
    typedef double f64_t;
#endif

/* Sizeof. */
#if !defined(alignof)
    #define sizeof(T) ((sz_t)sizeof(T))
#endif

/* Alignment. Provide custom implementation of alignof or default implementation which attempts to grab the most accurate implementation. */
#if !defined(alignof)
    #if defined(__cplusplus) && __cplusplus >= 201103L /* C++11 */
        #define alignof(T)     ((sz_t)alignof(T))
    #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L /* C11 */
        #define alignof(T)     ((sz_t)_Alignof(T))
    #elif defined(_MSC_VER)
        #define alignof(T)     ((sz_t)__alignof(T))
    #elif defined(__GNUC__)
        #define alignof(T)     ((sz_t)__alignof__(T))
    #elif defined(__cplusplus)
        /* Portable alignment implementation by Martin Buchholz ( https://wambold.com/Martin/writings/alignof.html ) */
        /* 1) Attempt to find the alignment by rounding to the nearest power of 2. */
        namespace alignof_nearest_power_of_two {
            template<typename T>
            struct alignof_t { 
                enum { 
                    size = sizeof(T), 
                    value = size ^ (size & (size - 1))
                }; 
            };
        }
        /* 2) Attempt to find the alignment by adding bytes onto the type until the size of the struct doubles. */
        namespace alignof_size_doubled {
            template<typename T> struct alignof_t;

            template<int size_diff>
            struct helper_t {
                template<typename T> 
                struct val_t { enum { value = size_diff }; };
            };

            template<>
            struct helper_t<0>
            {
                template<typename T> 
                struct val_t { enum { value = alignof_t<T>::value }; };
            };

            template<typename T>
            struct alignof_t
            {
                struct type_plus_one_byte_t { T x; char c; };
                enum { 
                    diff = sizeof(type_plus_one_byte_t) - sizeof(T), 
                    value = helper_t<diff>::template val_t<type_plus_one_byte_t>::value 
                };
            };
        }
        /* Pick the larger of the two reported alignments as neither works in all cases. */
        template<typename T>
        struct alignof_t {
            enum { 
                one = alignof_nearest_power_of_two::alignof_t<T>::value,
                two = alignof_size_doubled::alignof_t<T>::value,
                value = one < two ? one : two
            };
        };

        #define alignof(T)                static_cast<sz_t>(alignof_t<T>::value)
    #else
        #define alignof(T)                (((sz_t)&( (struct{ u8_t b; T t; }* )0 )->t)
    #endif
#endif

/* Force inline. */
#if !defined(force_inline)
    #if defined(__GNUC__) || defined(__clang__)
        #define force_inline static __attribute__((always_inline))
    #elif defined(_MSC_VER)
        #define force_inline static __forceinline
    #else
        #define force_inline static
        #warning "Compiler does not support force inline code."
    #endif
#endif

/* Provide custom or default implementation for memcpy and memset. */
#if defined(memcpy) && !defined(memset) || !defined(memcpy) && defined(memset)
    #error "You must define both memcpy and memset."
#endif
#if defined(memcpy_aligned) && !defined(memset_aligned) || !defined(memcpy_aligned) && defined(memset_aligned)
    #error "You must define both memcpy_aligned and memset_aligned."
#endif
#if !defined(memcpy) && !defined(memset)
    #if defined(__cplusplus)
        #include <cstring>
        #define memcpy(i_dst, i_src, i_size)       std::memcpy((i_dst), (i_src), (i_size))
        #define memset(i_ptr, i_value, i_size)     std::memset((i_ptr), (i_value), (i_size))
    #else
        #include <string.h>
        #define memcpy(i_dst, i_src, i_size)       memcpy((i_dst), (i_src), (i_size))
        #define memset(i_ptr, i_value, i_size)     memset((i_ptr), (i_value), (i_size))
    #endif
#endif

#define memzero(i_ptr, i_size)                    memset(i_ptr, 0, i_size)

/* Memory management. Provide custom allocator implementation or default implementation build on the C standard library. */
#if defined(realloc) && !defined(free) || !defined(realloc) && defined(free)
    #error "You must define both realloc and free."
#endif
#if defined(realloc_aligned) && !defined(free_aligned) || !defined(realloc_aligned) && defined(free_aligned)
    #error "You must define both realloc_aligned and free_aligned."
#endif
#if !defined(realloc) && !defined(free)
    #if defined(__cplusplus)
        #include <cstdlib>
    #else
        #include <stdlib.h>
    #endif
    #define realloc(io_data, i_size)                       realloc((io_data), (i_size))
    #define free(io_data)                                  free(io_data)
#endif

#if !defined(realloc_aligned) && !defined(free_aligned)
    #define realloc_aligned(io_data, i_size, i_alignment)  realloc_aligned_impl((io_data), (i_size), (i_alignment))
    #define free_aligned(io_data)                          free_aligned_impl(io_data)

    void* realloc_aligned_impl(void* io_data, sz_t i_size, sz_t i_alignment)
    {
        sz_t offset;
        u8_t* pointer;
        u8_t* return_pointer;
        sz_t offset_from_prev_align;
        sz_t offset_to_next_align;
        assert(i_alignment < 256); /* Maximum supported alignment. */

        pointer = ((u8_t*)io_data);
        if (pointer != NULL)
        {
            offset = *(((u8_t*)io_data) - 1);
            pointer = (pointer - offset);
        }

        /* Realloc can return any address but our desired alignement can be at most i_alignment bytes away. */
        pointer = (u8_t*)realloc(pointer, i_size + i_alignment); 
        assert(pointer != NULL);
        
        /* Round to the next aligned address. 
        Store the offset from our allocation to this address in the byte before it so we can retreive the orginal address later. */
        offset_from_prev_align = (sz_t)pointer % i_alignment;
        offset_to_next_align = i_alignment - offset_from_prev_align;
        return_pointer = pointer + offset_to_next_align;

        *(u8_t*)(return_pointer - 1) = (u8_t)offset_to_next_align;

        return (void*)return_pointer;
    }

    void free_aligned_impl(void* io_data)
    {
        sz_t offset;
        offset = *(((u8_t*)io_data) - 1);
        free(((u8_t*)io_data) - offset);
    }
#endif

/* Allocation helper functions. */
#define malloc_type(T)                                (T*)realloc(NULL, sizeof(T))
#define malloc_arr(T, i_len)                          (T*)realloc(NULL, (i_len) * sizeof(T))
#define realloc_arr(T, i_ptr, i_len)                  (T*)realloc((i_ptr), (i_len) * sizeof(T))

/* Math. */
#if defined(f32_mod) || defined(f32_sin) || defined(f32_cos) || defined(f32_tan) || defined(f32_sqrt) || defined(f32_pow) || defined(f32_atan2) || defined(f64_mod) || defined(f64_sin) || defined(f64_cos) || defined(f64_tan) || defined(f64_sqrt) || defined(f64_pow) || defined(f64_atan2)
    #if !defined(f32_sin) || !defined(f32_sin) || !defined(f32_cos) || !defined(f32_tan) || !defined(f32_sqrt) || !defined(f32_pow) || !defined(f32_atan2) || !defined(f64_mod) || !defined(f64_sin) || !defined(f64_cos) || !defined(f64_tan) || !defined(f64_sqrt) || !defined(f64_pow) || !defined(f64_atan2)
        #error "You must define all of f32_mod, f32_sin, f32_cos, f32_tan, f32_sqrt, f32_pow, f32_atan2, f64_mod, f64_sin, f64_cos, f64_tan, f64_sqrt, f64_pow and f64_atan2."
    #endif
#else
    #if defined(__cplusplus)
        #include <cmath>
    #else
        #include <math.h>
    #endif
    #define f32_mod(i_x, i_y)                  ((f32_t)fmodf(i_x, i_y))
    #define f32_sin(i_value)                   ((f32_t)sinf(i_value))
    #define f32_cos(i_value)                   ((f32_t)cosf(i_value))
    #define f32_tan(i_value)                   ((f32_t)tanf(i_value))
    #define f32_sqrt(i_value)                  ((f32_t)sqrtf(i_value))
    #define f32_pow(i_value, i_exponent)       ((f32_t)powf(i_value, i_exponent))
    #define f32_atan2(i_y, i_x)                ((f32_t)atan2f(i_y, i_x))
    
    #define f64_mod(i_x, i_y)                  ((f64_t)fmod(i_x, i_y))
    #define f64_sin(i_value)                   ((f64_t)sin(i_value))
    #define f64_cos(i_value)                   ((f64_t)cos(i_value))
    #define f64_tan(i_value)                   ((f64_t)tan(i_value))
    #define f64_sqrt(i_value)                  ((f64_t)sqrt(i_value))
    #define f64_pow(i_value, i_exponent)       ((f64_t)pow(i_value, i_exponent))
    #define f64_atan2(i_y, i_x)                ((f64_t)atan2(i_y, i_x))
#endif

typedef union   
{
    f32_t data[2];

    struct { f32_t x, y; };
    struct { f32_t u, v; };
    struct { f32_t left, right; };
    struct { f32_t width, height; };
} fvec2_t;

typedef union  
{
    f32_t data[3];

    struct { f32_t x, y, z; };
    struct { fvec2_t xy; f32_t _pad0; };
    struct { f32_t _pad1; fvec2_t yz; };

    struct { f32_t u, v, w; };
    struct { fvec2_t uv; f32_t _pad2; };
    struct { f32_t _pad3; fvec2_t vw; };

    struct { f32_t r, g, b; };
    struct { fvec2_t rg; f32_t _pad4; };
    struct { f32_t _pad5; fvec2_t gb; };
} fvec3_t;

typedef union  
{
    f32_t data[4];

    struct { f32_t x, y, z, w; };
    struct { fvec3_t xyz; f32_t _pad0; };
    struct { f32_t _pad1; fvec3_t yzw; };
    struct { fvec2_t xy; fvec2_t zw; };
    struct { f32_t _pad2; fvec2_t yz; f32_t _pad3; };

    struct { f32_t r, g, b, a; };
    struct { fvec3_t rgb; f32_t _pad4; };
    struct { f32_t _pad5; fvec3_t gba; };
    struct { fvec2_t rg; fvec2_t ba; };
    struct { f32_t _pad6; fvec2_t gb; f32_t _pad7; };

    struct { fvec2_t top_left; fvec2_t bottom_right; };
    struct { fvec2_t _pad8; fvec2_t width_height; };
    struct { fvec2_t _pad9; f32_t width; f32_t height; };
} fvec4_t;

typedef union 
{
    f32_t data[2][2];
    fvec2_t columns[2];
} fmat22_t;

typedef union 
{
    f32_t data[3][3];
    fvec3_t columns[3];
} fmat33_t;

typedef union 
{
    f32_t data[4][4];
    fvec4_t columns[4];
} fmat44_t;

force_inline f32_t f32_zero()              { return 0.0f; }
force_inline f32_t f32_quarter()           { return 0.25f; }
force_inline f32_t f32_half()              { return 0.5f; }
force_inline f32_t f32_one()               { return 1.0f; }
force_inline f32_t f32_two()               { return 2.0f; }
force_inline f32_t f32_minus_quarter()     { return -0.25f; }
force_inline f32_t f32_minus_half()        { return -0.5f; }
force_inline f32_t f32_minus_one()         { return -1.0f; }
force_inline f32_t f32_minus_two()         { return -2.0f; }

force_inline f64_t f64_zero()              { return 0.0; }
force_inline f64_t f64_half()              { return 0.5; }
force_inline f64_t f64_one()               { return 1.0; }
force_inline f64_t f64_two()               { return 2.0; }
force_inline f64_t f64_minus_half()        { return -0.5; }
force_inline f64_t f64_minus_one()         { return -1.0; }
force_inline f64_t f64_minus_two()         { return -2.0; }

force_inline fvec2_t fvec2_zero()          { fvec2_t result; result.x = f32_zero();        result.y = f32_zero();         return result; }
force_inline fvec2_t fvec2_half()          { fvec2_t result; result.x = f32_half();        result.y = f32_half();         return result; }
force_inline fvec2_t fvec2_one()           { fvec2_t result; result.x = f32_one();         result.y = f32_one();          return result; }
force_inline fvec2_t fvec2_two()           { fvec2_t result; result.x = f32_two();         result.y = f32_two();          return result; }
force_inline fvec2_t fvec2_minus_half()    { fvec2_t result; result.x = f32_minus_half();  result.y = f32_minus_half();   return result; }
force_inline fvec2_t fvec2_minus_one()     { fvec2_t result; result.x = f32_minus_one();   result.y = f32_minus_one();    return result; }
force_inline fvec2_t fvec2_minus_two()     { fvec2_t result; result.x = f32_minus_two();   result.y = f32_minus_two();    return result; }

force_inline fvec3_t fvec3_zero()          { fvec3_t result; result.x = f32_zero();        result.y = f32_zero();         result.z = f32_zero();         return result; }
force_inline fvec3_t fvec3_half()          { fvec3_t result; result.x = f32_half();        result.y = f32_half();         result.z = f32_half();         return result; }
force_inline fvec3_t fvec3_one()           { fvec3_t result; result.x = f32_one();         result.y = f32_one();          result.z = f32_one();          return result; }
force_inline fvec3_t fvec3_two()           { fvec3_t result; result.x = f32_two();         result.y = f32_two();          result.z = f32_two();          return result; }
force_inline fvec3_t fvec3_minus_half()    { fvec3_t result; result.x = f32_minus_half();  result.y = f32_minus_half();   result.z = f32_minus_half();   return result; }
force_inline fvec3_t fvec3_minus_one()     { fvec3_t result; result.x = f32_minus_one();   result.y = f32_minus_one();    result.z = f32_minus_one();    return result; }
force_inline fvec3_t fvec3_minus_two()     { fvec3_t result; result.x = f32_minus_two();   result.y = f32_minus_two();    result.z = f32_minus_two();    return result; }

force_inline fvec4_t fvec4_zero()          { fvec4_t result; result.x = f32_zero();        result.y = f32_zero();         result.z = f32_zero();         result.w = f32_zero();         return result; }
force_inline fvec4_t fvec4_half()          { fvec4_t result; result.x = f32_half();        result.y = f32_half();         result.z = f32_half();         result.w = f32_half();         return result; }
force_inline fvec4_t fvec4_one()           { fvec4_t result; result.x = f32_one();         result.y = f32_one();          result.z = f32_one();          result.w = f32_one();          return result; }
force_inline fvec4_t fvec4_two()           { fvec4_t result; result.x = f32_two();         result.y = f32_two();          result.z = f32_two();          result.w = f32_two();          return result; }
force_inline fvec4_t fvec4_minus_half()    { fvec4_t result; result.x = f32_minus_half();  result.y = f32_minus_half();   result.z = f32_minus_half();   result.w = f32_minus_half();   return result; }
force_inline fvec4_t fvec4_minus_one()     { fvec4_t result; result.x = f32_minus_one();   result.y = f32_minus_one();    result.z = f32_minus_one();    result.w = f32_minus_one();    return result; }
force_inline fvec4_t fvec4_minus_two()     { fvec4_t result; result.x = f32_minus_two();   result.y = f32_minus_two();    result.z = f32_minus_two();    result.w = f32_minus_two();    return result; }

force_inline fmat44_t fmat44_zero()        { fmat44_t result = {0}; return result; }
force_inline fmat44_t fmat44_identity()    { fmat44_t result = {0}; result.data[0][0] = f32_one(); result.data[1][1] = f32_one(); result.data[2][2] = f32_one();result.data[3][3] = f32_one(); return result; }

#define f32_pi 3.14159265359f
#define f64_pi 3.14159265358979323846
#define f32_rad_to_deg(i_radians) (i_radian * (180.0f / f32_pi))
#define f32_deg_to_rad(i_degrees) (i_degrees * (f32_pi / 180.0f))
#define f64_rad_to_deg(i_radians) (i_radian * (180.0 / f64_pi))
#define f64_deg_to_rad(i_degrees) (i_degrees * (f64_pi / 180.0))

#define math_min(i_value1, i_value2) ((i_value1) < (i_value2) ?  (i_value1) : (i_value2))
#define math_max(i_value1, i_value2) ((i_value1) > (i_value2) ?  (i_value1) : (i_value2))
#define math_clamp(i_value, i_min, i_max) ((i_value) < i_min ? i_min : ((i_value) > i_max ? i_max : (i_value)))
#define math_sign(i_value) ((i_value) > 0 ? 1 : ((i_value) < 0 ? -1 : 0))

force_inline sz_t math_round_up(sz_t i_value, sz_t i_rounding)
{
    return (i_value + (i_rounding - 1)) & ~(i_rounding - 1);
}

force_inline f32_t f32_lerp(f32_t i_start, f32_t i_end, f32_t i_percentage)
{
    return i_start * (f32_one() - i_percentage) + i_end * i_percentage;
}

force_inline f32_t f32_lerp3(f32_t i_start, f32_t i_middle, f32_t i_end, f32_t i_percentage)
{
    return i_percentage < f32_half() ? 
        f32_lerp(i_start, i_middle, i_percentage * f32_two()) : 
        f32_lerp(i_middle, i_end, (i_percentage - f32_half()) * f32_two());
}

force_inline f32_t f32_ease_in(f32_t i_start, f32_t i_end, f32_t i_percentage)
{
    return f32_lerp(i_start, i_end, i_percentage * i_percentage);
}

force_inline f32_t f32_ease_out(f32_t i_start, f32_t i_end, f32_t i_percentage)
{
    return -f32_lerp(i_start, i_end, i_percentage * i_percentage);
}

force_inline f32_t f32_ease_in_out(f32_t i_start, f32_t i_end, f32_t i_percentage)
{
    return i_percentage < f32_half() ? f32_ease_in(i_start, i_end, i_percentage) : f32_ease_out(i_start, i_end, i_percentage);
}

force_inline f32_t f32_lerp_delta_time(f32_t i_delta_time, f32_t i_factor, f32_t i_time)
{
    return f32_one() - f32_pow(i_factor, i_delta_time * (f32_one() / i_time));
}

force_inline f32_t f32_add(f32_t i_left, f32_t i_right)
{
    return i_left + i_right;
}

force_inline f32_t f32_sub(f32_t i_left, f32_t i_right)
{
    return i_left - i_right;
}

force_inline f32_t f32_mul(f32_t i_left, f32_t i_right)
{
    return i_left * i_right;
}

force_inline f32_t f32_div(f32_t i_left, f32_t i_right)
{
    return i_left / i_right;
}

force_inline f32_t f32_abs(f32_t i_vec)
{
    return i_vec < 0 ? -i_vec : i_vec;
}

force_inline f32_t f32_min_smooth(f32_t i_value1, f32_t i_value2, f32_t i_factor)
{
    f32_t h = math_max(i_factor - f32_abs(i_value1 - i_value2), f32_zero());
    return math_min(i_value1, i_value2) - h * h * f32_quarter() / i_factor;
}

force_inline f32_t f32_neg(f32_t i_vec)
{
    return -i_vec;
}

force_inline f32_t f32_inv_sqrt(f32_t i_vec)
{
    return f32_div(f32_one(), f32_sqrt(i_vec));
}

force_inline f64_t f64_add(f64_t i_left, f64_t i_right)
{
    return i_left + i_right;
}

force_inline f64_t f64_sub(f64_t i_left, f64_t i_right)
{
    return i_left - i_right;
}

force_inline f64_t f64_mul(f64_t i_left, f64_t i_right)
{
    return i_left * i_right;
}

force_inline f64_t f64_div(f64_t i_left, f64_t i_right)
{
    return i_left / i_right;
}

force_inline f64_t f64_abs(f64_t i_vec)
{
    return i_vec < 0 ? -i_vec : i_vec;
}

force_inline f64_t f64_neg(f64_t i_vec)
{
    return -i_vec;
}

force_inline f64_t f64_inv_sqrt(f64_t i_vec)
{
    return f64_div(f64_one(), f64_sqrt(i_vec));
}

force_inline f32 fvec2_at(fvec2_t i_value, sz_t i_index)
{
    switch(i_index)
    {
        case 0: return i_value.x;
        case 1: return i_value.y;
        default: 
            assert(0); 
            return 0;
    }
}

force_inline fvec2_t fvec2_add(fvec2_t i_left, fvec2_t i_right)
{
    fvec2_t result;
    result.x = f32_add(i_left.x, i_right.x);
    result.y = f32_add(i_left.y, i_right.y);
    return result;
}

force_inline fvec2_t fvec2_sub(fvec2_t i_left, fvec2_t i_right)
{
    fvec2_t result;
    result.x = f32_sub(i_left.x, i_right.x);
    result.y = f32_sub(i_left.y, i_right.y);
    return result;
}

force_inline fvec2_t fvec2_mul(fvec2_t i_left, fvec2_t i_right)
{
    fvec2_t result;
    result.x = f32_mul(i_left.x, i_right.x);
    result.y = f32_mul(i_left.y, i_right.y);
    return result;
}

force_inline fvec2_t fvec2_div(fvec2_t i_left, fvec2_t i_right)
{
    fvec2_t result;
    result.x = f32_div(i_left.x, i_right.x);
    result.y = f32_div(i_left.y, i_right.y);
    return result;
}

force_inline fvec2_t fvec2_mul_s(fvec2_t i_left, f32_t i_right)
{
    fvec2_t result;
    result.x = f32_mul(i_left.x, i_right);
    result.y = f32_mul(i_left.y, i_right);
    return result;
}

force_inline fvec2_t fvec2_div_s(fvec2_t i_left, f32_t i_right)
{
    fvec2_t result;
    result.x = f32_div(i_left.x, i_right);
    result.y = f32_div(i_left.y, i_right);
    return result;
}

force_inline fvec2_t fvec2_abs(fvec2_t i_vec)
{
    fvec2_t result;
    result.x = f32_abs(i_vec.x);
    result.y = f32_abs(i_vec.y);
    return result;
}

force_inline fvec2_t fvec2_neg(fvec2_t i_vec)
{
    fvec2_t result;
    result.x = f32_neg(i_vec.x);
    result.y = f32_neg(i_vec.y);
    return result;
}

force_inline fvec2_t fvec2_sqrt(fvec2_t i_vec)
{
    fvec2_t result;
    result.x = f32_sqrt(i_vec.x);
    result.y = f32_sqrt(i_vec.y);
    return result;
}

force_inline fvec2_t fvec2_inv_sqrt(fvec2_t i_vec)
{
    fvec2_t result;
    result.x = f32_inv_sqrt(i_vec.x);
    result.y = f32_inv_sqrt(i_vec.y);
    return result;
}

force_inline f32_t fvec2_dot(fvec2_t i_left, fvec2_t i_right)
{
    return f32_add(f32_mul(i_left.x, i_right.x),
                        f32_mul(i_left.y, i_right.y));
}

force_inline f32_t fvec2_len2(fvec2_t i_vec)
{
    return fvec2_dot(i_vec, i_vec);
}

force_inline f32_t fvec2_len(fvec2_t i_vec)
{
    return f32_sqrt(fvec2_dot(i_vec, i_vec));
}

force_inline fvec2_t fvec2_norm(fvec2_t i_vec)
{
    f32_t inv_lentgh;
    inv_lentgh = f32_inv_sqrt(fvec2_dot(i_vec, i_vec));
    return fvec2_mul_s(i_vec, inv_lentgh);
}

force_inline fvec2_t fvec2_mul_fmat22(fvec2_t i_left, fmat22_t i_right)
{
    fvec2_t result;
    result.x = i_left.data[0] * i_right.columns[0].x;
    result.y = i_left.data[0] * i_right.columns[0].y;

    result.x += i_left.data[1] * i_right.columns[1].x;
    result.y += i_left.data[1] * i_right.columns[1].y;

    return result;
}

force_inline f32 fvec3_at(fvec3_t i_value, sz_t i_index)
{
    switch(i_index)
    {
        case 0: return i_value.x;
        case 1: return i_value.y;
        case 2: return i_value.z;
        default: 
            assert(0); 
            return 0;
    }
}

force_inline fvec3_t fvec3_add(fvec3_t i_left, fvec3_t i_right)
{
    fvec3_t result;
    result.x = f32_add(i_left.x, i_right.x);
    result.y = f32_add(i_left.y, i_right.y);
    result.z = f32_add(i_left.z, i_right.z);
    return result;
}

force_inline fvec3_t fvec3_sub(fvec3_t i_left, fvec3_t i_right)
{
    fvec3_t result;
    result.x = f32_sub(i_left.x, i_right.x);
    result.y = f32_sub(i_left.y, i_right.y);
    result.z = f32_sub(i_left.z, i_right.z);
    return result;
}

force_inline fvec3_t fvec3_mul(fvec3_t i_left, fvec3_t i_right)
{
    fvec3_t result;
    result.x = f32_mul(i_left.x, i_right.x);
    result.y = f32_mul(i_left.y, i_right.y);
    result.z = f32_mul(i_left.z, i_right.z);
    return result;
}

force_inline fvec3_t fvec3_div(fvec3_t i_left, fvec3_t i_right)
{
    fvec3_t result;
    result.x = f32_div(i_left.x, i_right.x);
    result.y = f32_div(i_left.y, i_right.y);
    result.z = f32_div(i_left.z, i_right.z);
    return result;
}

force_inline fvec3_t fvec3_mul_s(fvec3_t i_left, f32_t i_right)
{
    fvec3_t result;
    result.x = f32_mul(i_left.x, i_right);
    result.y = f32_mul(i_left.y, i_right);
    result.z = f32_mul(i_left.z, i_right);
    return result;
}

force_inline fvec3_t fvec3_div_s(fvec3_t i_left, f32_t i_right)
{
    fvec3_t result;
    result.x = f32_div(i_left.x, i_right);
    result.y = f32_div(i_left.y, i_right);
    result.z = f32_div(i_left.z, i_right);
    return result;
}

force_inline fvec3_t fvec3_abs(fvec3_t i_vec)
{
    fvec3_t result;
    result.x = f32_abs(i_vec.x);
    result.y = f32_abs(i_vec.y);
    result.z = f32_abs(i_vec.z);
    return result;
}

force_inline fvec3_t fvec3_neg(fvec3_t i_vec)
{
    fvec3_t result;
    result.x = f32_neg(i_vec.x);
    result.y = f32_neg(i_vec.y);
    result.z = f32_neg(i_vec.z);
    return result;
}

force_inline fvec3_t fvec3_sqrt(fvec3_t i_vec)
{
    fvec3_t result;
    result.x = f32_sqrt(i_vec.x);
    result.y = f32_sqrt(i_vec.y);
    result.z = f32_sqrt(i_vec.z);
    return result;
}

force_inline fvec3_t fvec3_inv_sqrt(fvec3_t i_vec)
{
    fvec3_t result;
    result.x = f32_inv_sqrt(i_vec.x);
    result.y = f32_inv_sqrt(i_vec.y);
    result.z = f32_inv_sqrt(i_vec.z);
    return result;
}

force_inline fvec3_t fvec3_cross(fvec3_t i_left, fvec3_t i_right)
{
    fvec3_t result;
    result.x = f32_sub(f32_mul(i_left.y, i_right.z), f32_mul(i_left.z, i_right.y));
    result.y = f32_sub(f32_mul(i_left.z, i_right.x), f32_mul(i_left.x, i_right.z));
    result.z = f32_sub(f32_mul(i_left.x, i_right.y), f32_mul(i_left.y, i_right.x));
    return result;
}

force_inline f32_t fvec3_dot(fvec3_t i_left, fvec3_t i_right)
{
    return f32_add(f32_mul(i_left.x, i_right.x),  
           f32_add(f32_mul(i_left.y, i_right.y),
                        f32_mul(i_left.z, i_right.z)));
}

force_inline fvec3_t fvec3_norm(fvec3_t i_vec)
{
    f32_t inv_lentgh = f32_inv_sqrt(fvec3_dot(i_vec, i_vec));
    return fvec3_mul_s(i_vec, inv_lentgh);
}

force_inline fvec3_t fvec3_mul_fmat33(fvec3_t i_left, fmat33_t i_right)
{
    fvec3_t result;
    result.x = i_left.data[0] * i_right.columns[0].x;
    result.y = i_left.data[0] * i_right.columns[0].y;
    result.z = i_left.data[0] * i_right.columns[0].z;

    result.x += i_left.data[1] * i_right.columns[1].x;
    result.y += i_left.data[1] * i_right.columns[1].y;
    result.z += i_left.data[1] * i_right.columns[1].z;

    result.x += i_left.data[2] * i_right.columns[2].x;
    result.y += i_left.data[2] * i_right.columns[2].y;
    result.z += i_left.data[2] * i_right.columns[2].z;

    return result;
}

force_inline f32 fvec4_at(fvec4_t i_value, sz_t i_index)
{
    switch(i_index)
    {
        case 0: return i_value.x;
        case 1: return i_value.y;
        case 2: return i_value.z;
        case 3: return i_value.w;
        default: 
            assert(0); 
            return 0;
    }
}

force_inline fvec4_t fvec4_add(fvec4_t i_left, fvec4_t i_right)
{
    fvec4_t result;
    result.x = f32_add(i_left.x, i_right.x);
    result.y = f32_add(i_left.y, i_right.y);
    result.z = f32_add(i_left.z, i_right.z);
    result.w = f32_add(i_left.w, i_right.w);
    return result;
}

force_inline fvec4_t fvec4_sub(fvec4_t i_left, fvec4_t i_right)
{
    fvec4_t result;
    result.x = f32_sub(i_left.x, i_right.x);
    result.y = f32_sub(i_left.y, i_right.y);
    result.z = f32_sub(i_left.z, i_right.z);
    result.w = f32_sub(i_left.w, i_right.w);
    return result;
}

force_inline fvec4_t fvec4_mul(fvec4_t i_left, fvec4_t i_right)
{
    fvec4_t result;
    result.x = f32_mul(i_left.x, i_right.x);
    result.y = f32_mul(i_left.y, i_right.y);
    result.z = f32_mul(i_left.z, i_right.z);
    result.w = f32_mul(i_left.w, i_right.w);
    return result;
}

force_inline fvec4_t fvec4_div(fvec4_t i_left, fvec4_t i_right)
{
    fvec4_t result;
    result.x = f32_div(i_left.x, i_right.x);
    result.y = f32_div(i_left.y, i_right.y);
    result.z = f32_div(i_left.z, i_right.z);
    result.w = f32_div(i_left.w, i_right.w);
    return result;
}

force_inline fvec4_t fvec4_mul_s(fvec4_t i_left, f32_t i_right)
{
    fvec4_t result;
    result.x = f32_mul(i_left.x, i_right);
    result.y = f32_mul(i_left.y, i_right);
    result.z = f32_mul(i_left.z, i_right);
    result.w = f32_mul(i_left.w, i_right);
    return result;
}

force_inline fvec4_t fvec4_div_s(fvec4_t i_left, f32_t i_right)
{
    fvec4_t result;
    result.x = f32_div(i_left.x, i_right);
    result.y = f32_div(i_left.y, i_right);
    result.z = f32_div(i_left.z, i_right);
    result.w = f32_div(i_left.w, i_right);
    return result;
}

force_inline fvec4_t fvec4_abs(fvec4_t i_vec)
{
    fvec4_t result;
    result.x = f32_abs(i_vec.x);
    result.y = f32_abs(i_vec.y);
    result.z = f32_abs(i_vec.z);
    result.w = f32_abs(i_vec.w);
    return result;
}

force_inline fvec4_t fvec4_neg(fvec4_t i_vec)
{
    fvec4_t result;
    result.x = f32_neg(i_vec.x);
    result.y = f32_neg(i_vec.y);
    result.z = f32_neg(i_vec.z);
    result.w = f32_neg(i_vec.w);
    return result;
}

force_inline fvec4_t fvec4_sqrt(fvec4_t i_vec)
{
    fvec4_t result;
    result.x = f32_sqrt(i_vec.x);
    result.y = f32_sqrt(i_vec.y);
    result.z = f32_sqrt(i_vec.z);
    result.w = f32_sqrt(i_vec.w);
    return result;
}

force_inline fvec4_t fvec4_inv_sqrt(fvec4_t i_vec)
{
    fvec4_t result;
    result.x = f32_inv_sqrt(i_vec.x);
    result.y = f32_inv_sqrt(i_vec.y);
    result.z = f32_inv_sqrt(i_vec.z);
    result.w = f32_inv_sqrt(i_vec.w);
    return result;
}

force_inline f32_t fvec4_dot(fvec4_t i_left, fvec4_t i_right)
{
    return f32_add(f32_mul(i_left.x, i_right.x), 
           f32_add(f32_mul(i_left.y, i_right.y), 
           f32_add(f32_mul(i_left.z, i_right.z), 
                        f32_mul(i_left.w, i_right.w))));
}

force_inline fvec4_t fvec4_norm(fvec4_t i_vec)
{
    f32_t inv_lentgh = f32_inv_sqrt(fvec4_dot(i_vec, i_vec));
    return fvec4_mul_s(i_vec, inv_lentgh);
}

force_inline fvec4_t fvec4_mul_fmat44(fvec4_t i_left, fmat44_t i_right)
{
    fvec4_t result;
    result.x = i_left.data[0] * i_right.columns[0].x;
    result.y = i_left.data[0] * i_right.columns[0].y;
    result.z = i_left.data[0] * i_right.columns[0].z;
    result.w = i_left.data[0] * i_right.columns[0].w;

    result.x += i_left.data[1] * i_right.columns[1].x;
    result.y += i_left.data[1] * i_right.columns[1].y;
    result.z += i_left.data[1] * i_right.columns[1].z;
    result.w += i_left.data[1] * i_right.columns[1].w;

    result.x += i_left.data[2] * i_right.columns[2].x;
    result.y += i_left.data[2] * i_right.columns[2].y;
    result.z += i_left.data[2] * i_right.columns[2].z;
    result.w += i_left.data[2] * i_right.columns[2].w;

    result.x += i_left.data[3] * i_right.columns[3].x;
    result.y += i_left.data[3] * i_right.columns[3].y;
    result.z += i_left.data[3] * i_right.columns[3].z;
    result.w += i_left.data[3] * i_right.columns[3].w;
    return result;
}

force_inline fmat22_t fmat22_add(fmat22_t i_left, fmat22_t i_right)
{
    fmat22_t result;
    result.columns[0] = fvec2_add(i_left.columns[0], i_right.columns[0]);
    result.columns[1] = fvec2_add(i_left.columns[1], i_right.columns[1]);
    return result;
}

force_inline fmat22_t fmat22_sub(fmat22_t i_left, fmat22_t i_right)
{
    fmat22_t result;
    result.columns[0] = fvec2_sub(i_left.columns[0], i_right.columns[0]);
    result.columns[1] = fvec2_sub(i_left.columns[1], i_right.columns[1]);
    return result;
}

force_inline fmat22_t fmat22_mul(fmat22_t i_left, fmat22_t i_right)
{
    fmat22_t result;
    result.columns[0] = fvec2_mul_fmat22(i_right.columns[0], i_left);
    result.columns[1] = fvec2_mul_fmat22(i_right.columns[1], i_left);
    return result;
}

force_inline fmat22_t fmat22_mul_s(fmat22_t i_left, f32_t i_right)
{
    fmat22_t result;
    result.columns[0] = fvec2_mul_s(i_left.columns[0], i_right);
    result.columns[1] = fvec2_mul_s(i_left.columns[1], i_right);
    return result;
}

force_inline fmat22_t fmat22_div_s(fmat22_t i_left, f32_t i_right)
{
    fmat22_t result;
    result.columns[0] = fvec2_div_s(i_left.columns[0], i_right);
    result.columns[1] = fvec2_div_s(i_left.columns[1], i_right);
    return result;
}

force_inline fmat33_t fmat33_add(fmat33_t i_left, fmat33_t i_right)
{
    fmat33_t result;
    result.columns[0] = fvec3_add(i_left.columns[0], i_right.columns[0]);
    result.columns[1] = fvec3_add(i_left.columns[1], i_right.columns[1]);
    result.columns[2] = fvec3_add(i_left.columns[2], i_right.columns[2]);
    return result;
}

force_inline fmat33_t fmat33_sub(fmat33_t i_left, fmat33_t i_right)
{
    fmat33_t result;
    result.columns[0] = fvec3_sub(i_left.columns[0], i_right.columns[0]);
    result.columns[1] = fvec3_sub(i_left.columns[1], i_right.columns[1]);
    result.columns[2] = fvec3_sub(i_left.columns[2], i_right.columns[2]);
    return result;
}

force_inline fmat33_t fmat33_mul(fmat33_t i_left, fmat33_t i_right)
{
    fmat33_t result;
    result.columns[0] = fvec3_mul_fmat33(i_right.columns[0], i_left);
    result.columns[1] = fvec3_mul_fmat33(i_right.columns[1], i_left);
    result.columns[2] = fvec3_mul_fmat33(i_right.columns[2], i_left);
    return result;
}

force_inline fmat33_t fmat33_mul_s(fmat33_t i_left, f32_t i_right)
{
    fmat33_t result;
    result.columns[0] = fvec3_mul_s(i_left.columns[0], i_right);
    result.columns[1] = fvec3_mul_s(i_left.columns[1], i_right);
    result.columns[2] = fvec3_mul_s(i_left.columns[2], i_right);
    return result;
}

force_inline fmat33_t fmat33_div_s(fmat33_t i_left, f32_t i_right)
{
    fmat33_t result;
    result.columns[0] = fvec3_div_s(i_left.columns[0], i_right);
    result.columns[1] = fvec3_div_s(i_left.columns[1], i_right);
    result.columns[2] = fvec3_div_s(i_left.columns[2], i_right);
    return result;
}

force_inline fmat44_t fmat44_add(fmat44_t i_left, fmat44_t i_right)
{
    fmat44_t result;
    result.columns[0] = fvec4_add(i_left.columns[0], i_right.columns[0]);
    result.columns[1] = fvec4_add(i_left.columns[1], i_right.columns[1]);
    result.columns[2] = fvec4_add(i_left.columns[2], i_right.columns[2]);
    result.columns[3] = fvec4_add(i_left.columns[3], i_right.columns[3]);
    return result;
}

force_inline fmat44_t fmat44_sub(fmat44_t i_left, fmat44_t i_right)
{
    fmat44_t result;
    result.columns[0] = fvec4_sub(i_left.columns[0], i_right.columns[0]);
    result.columns[1] = fvec4_sub(i_left.columns[1], i_right.columns[1]);
    result.columns[2] = fvec4_sub(i_left.columns[2], i_right.columns[2]);
    result.columns[3] = fvec4_sub(i_left.columns[3], i_right.columns[3]);
    return result;
}

force_inline fmat44_t fmat44_mul(fmat44_t i_left, fmat44_t i_right)
{
    fmat44_t result;
    result.columns[0] = fvec4_mul_fmat44(i_right.columns[0], i_left);
    result.columns[1] = fvec4_mul_fmat44(i_right.columns[1], i_left);
    result.columns[2] = fvec4_mul_fmat44(i_right.columns[2], i_left);
    result.columns[3] = fvec4_mul_fmat44(i_right.columns[3], i_left);
    return result;
}

force_inline fmat44_t fmat44_mul_s(fmat44_t i_left, f32_t i_right)
{
    fmat44_t result;
    result.columns[0] = fvec4_mul_s(i_left.columns[0], i_right);
    result.columns[1] = fvec4_mul_s(i_left.columns[1], i_right);
    result.columns[2] = fvec4_mul_s(i_left.columns[2], i_right);
    result.columns[3] = fvec4_mul_s(i_left.columns[3], i_right);
    return result;
}

force_inline fmat44_t fmat44_div_s(fmat44_t i_left, f32_t i_right)
{
    fmat44_t result;
    result.columns[0] = fvec4_div_s(i_left.columns[0], i_right);
    result.columns[1] = fvec4_div_s(i_left.columns[1], i_right);
    result.columns[2] = fvec4_div_s(i_left.columns[2], i_right);
    result.columns[3] = fvec4_div_s(i_left.columns[3], i_right);
    return result;
}

force_inline fmat44_t fmat44_transpose(fmat44_t i_mat)
{
    fmat44_t result = i_mat;
    result.data[0][0] = i_mat.data[0][0];
    result.data[0][1] = i_mat.data[1][0];
    result.data[0][2] = i_mat.data[2][0];
    result.data[0][3] = i_mat.data[3][0];
    result.data[1][0] = i_mat.data[0][1];
    result.data[1][1] = i_mat.data[1][1];
    result.data[1][2] = i_mat.data[2][1];
    result.data[1][3] = i_mat.data[3][1];
    result.data[2][0] = i_mat.data[0][2];
    result.data[2][1] = i_mat.data[1][2];
    result.data[2][2] = i_mat.data[2][2];
    result.data[2][3] = i_mat.data[3][2];
    result.data[3][0] = i_mat.data[0][3];
    result.data[3][1] = i_mat.data[1][3];
    result.data[3][2] = i_mat.data[2][3];
    result.data[3][3] = i_mat.data[3][3];
    return result;
}

force_inline fmat44_t fmat44_translate(fvec3_t i_position)
{
    fmat44_t result = fmat44_identity();
    result.data[3][0] += i_position.x;
    result.data[3][1] += i_position.y;
    result.data[3][2] += i_position.z;
    return result;
}

force_inline fmat44_t fmat44_rotate_angle(fvec3_t i_axis, f32_t i_angle)
{
    /* TODO: i_angle must be normalised. */
    f32_t sin_theta = f32_sin(i_angle);
    f32_t cos_theta = f32_cos(i_angle);
    f32_t one_minus_cos_theta = f32_one() - f32_cos(i_angle);

    fmat44_t result = fmat44_identity();
    result.data[0][0] = f32_add(f32_mul(f32_mul(i_axis.x, i_axis.x), one_minus_cos_theta), cos_theta);
    result.data[0][1] = f32_sub(f32_mul(f32_mul(i_axis.x, i_axis.y), one_minus_cos_theta), f32_mul(i_axis.z, sin_theta));
    result.data[0][2] = f32_add(f32_mul(f32_mul(i_axis.x, i_axis.z), one_minus_cos_theta), f32_mul(i_axis.y, sin_theta));

    result.data[1][0] = f32_add(f32_mul(f32_mul(i_axis.y, i_axis.x), one_minus_cos_theta), f32_mul(i_axis.z, sin_theta));
    result.data[1][1] = f32_add(f32_mul(f32_mul(i_axis.y, i_axis.y), one_minus_cos_theta), cos_theta);
    result.data[1][2] = f32_sub(f32_mul(f32_mul(i_axis.y, i_axis.z), one_minus_cos_theta), f32_mul(i_axis.x, sin_theta));

    result.data[2][0] = f32_sub(f32_mul(f32_mul(i_axis.z, i_axis.x), one_minus_cos_theta), f32_mul(i_axis.y, sin_theta));
    result.data[2][1] = f32_add(f32_mul(f32_mul(i_axis.z, i_axis.y), one_minus_cos_theta), f32_mul(i_axis.x, sin_theta));
    result.data[2][2] = f32_add(f32_mul(f32_mul(i_axis.z, i_axis.z), one_minus_cos_theta), cos_theta);
    return result;
}

force_inline fmat44_t fmat44_rotate_roll_pitch_yaw(fvec3_t i_roll_pitch_yaw)
{

    f32_t cos_roll = f32_cos(i_roll_pitch_yaw.x);
    f32_t cos_pitch = f32_cos(i_roll_pitch_yaw.y);
    f32_t cos_yaw = f32_cos(i_roll_pitch_yaw.z);
    f32_t sin_roll = f32_sin(i_roll_pitch_yaw.x);
    f32_t sin_pitch = f32_sin(i_roll_pitch_yaw.y);
    f32_t sin_yaw = f32_sin(i_roll_pitch_yaw.z);

    fmat44_t result = fmat44_identity();
    result.data[0][0] = f32_mul(cos_roll, cos_pitch);
    result.data[0][1] = f32_sub(f32_mul(f32_mul(cos_roll, cos_pitch), sin_yaw), f32_mul(sin_roll, cos_yaw));
    result.data[0][2] = f32_add(f32_mul(f32_mul(cos_roll, sin_pitch), cos_yaw), f32_mul(sin_roll, sin_yaw));

    result.data[1][0] = f32_mul(sin_roll, cos_pitch);
    result.data[1][1] = f32_add(f32_mul(f32_mul(sin_roll, sin_pitch), sin_yaw), f32_mul(cos_roll, cos_yaw));
    result.data[1][2] = f32_sub(f32_mul(f32_mul(sin_roll, sin_pitch), cos_yaw), f32_mul(cos_roll, sin_yaw));

    result.data[2][0] = f32_neg(sin_pitch);
    result.data[2][1] = f32_mul(cos_pitch, sin_yaw);
    result.data[2][2] = f32_mul(cos_pitch, cos_yaw);
    return result;
}

force_inline fmat44_t fmat44_scale(fvec3_t i_scale)
{
    fmat44_t result = fmat44_identity();
    result.data[0][0] += i_scale.x;
    result.data[1][1] += i_scale.y;
    result.data[2][2] += i_scale.z;
    return result;
}

force_inline fmat44_t math_look_at(fvec3_t i_position, fvec3_t i_target, fvec3_t i_up)
{
    fmat44_t result;
    fvec3_t forward = fvec3_norm(fvec3_sub(i_target, i_position));
    fvec3_t side = fvec3_norm(fvec3_cross(i_up, forward));
    fvec3_t up = fvec3_cross(forward, side);

    result.data[0][0] = side.x;
    result.data[0][1] = up.x;
    result.data[0][2] = forward.x;
    result.data[0][3] = f32_zero();

    result.data[1][0] = side.y;
    result.data[1][1] = up.y;
    result.data[1][2] = forward.y;
    result.data[1][3] = f32_zero();

    result.data[2][0] = side.z;
    result.data[2][1] = up.z;
    result.data[2][2] = forward.z;
    result.data[2][3] = f32_zero();

    result.data[3][0] = f32_neg(fvec3_dot(side, i_position));
    result.data[3][1] = f32_neg(fvec3_dot(up, i_position));
    result.data[3][2] = f32_neg(fvec3_dot(forward, i_position));
    result.data[3][3] = f32_one();
    return result;
}

force_inline fmat44_t math_perspective_projection(f32_t i_fov, f32_t i_aspect_ratio, f32_t i_near, f32_t i_far)
{
    fmat44_t result;
    f32_t cotangent = f32_div(f32_one(), f32_tan(f32_mul(i_fov, f32_half())));
    result.data[0][0] = f32_div(cotangent, i_aspect_ratio);
    result.data[0][1] = f32_zero();
    result.data[0][2] = f32_zero();
    result.data[0][3] = f32_zero();

    result.data[1][0] = f32_zero();
    result.data[1][1] = cotangent;
    result.data[1][2] = f32_zero();
    result.data[1][3] = f32_zero();

    result.data[2][0] = f32_zero();
    result.data[2][1] = f32_zero();
    result.data[2][2] = f32_div(i_far, f32_sub(i_far, i_near));
    result.data[2][3] = f32_one();

    result.data[3][0] = f32_zero();
    result.data[3][1] = f32_zero();
    result.data[3][2] = f32_div(f32_mul(i_near, i_far), f32_sub(i_near, i_far));
    result.data[3][3] = f32_zero();
    return result;
}

force_inline fmat44_t math_orthorgraphic_projection(f32_t i_left, f32_t i_right, f32_t i_bottom, f32_t i_top, f32_t i_near, f32_t i_far)
{
    fmat44_t result;
    result.data[0][0] = f32_two() / f32_sub(i_right, i_left);
    result.data[0][1] = f32_zero();
    result.data[0][2] = f32_zero();
    result.data[0][3] = f32_zero();

    result.data[1][0] = f32_zero();
    result.data[1][1] = f32_two() / f32_sub(i_top, i_bottom);
    result.data[1][2] = f32_zero();
    result.data[1][3] = f32_zero();

    result.data[2][0] = f32_zero();
    result.data[2][1] = f32_zero();
    result.data[2][2] = f32_one() / f32_sub(i_far, i_near);
    result.data[2][3] = f32_zero();

    result.data[3][0] = f32_neg(f32_div(f32_add(i_right, i_left), f32_sub(i_right, i_left)));
    result.data[3][1] = f32_neg(f32_div(f32_add(i_top, i_bottom), f32_sub(i_top, i_bottom)));
    result.data[3][2] = f32_neg(f32_div(i_near, f32_sub(i_far, i_near)));
    result.data[3][3] = f32_one();
    return result;
}

#if defined(_MSC_VER)
    #pragma warning(pop)
#elif defined(__clang__)
    #pragma clang diagnostic pop
#endif

#endif /* CORE_H */
