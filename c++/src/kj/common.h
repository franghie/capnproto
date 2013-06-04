// Copyright (c) 2013, Kenton Varda <temporal@gmail.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Header that should be #included by everyone.
//
// This defines very simple utilities that are widely applicable.

#include <stddef.h>

#ifndef KJ_COMMON_H_
#define KJ_COMMON_H_

#if __cplusplus < 201103L && !__CDT_PARSER__
  #error "This code requires C++11. Either your compiler does not support it or it is not enabled."
  #ifdef __GNUC__
    // Compiler claims compatibility with GCC, so presumably supports -std.
    #error "Pass -std=c++11 on the compiler command line to enable C++11."
  #endif
#endif

#ifdef __GNUC__
  #if __clang__
    #if __clang_major__ < 3 || (__clang_major__ == 3 && __clang_minor__ < 2)
      #warning "This library requires at least Clang 3.2."
    #endif
  #else
    #if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)
      #warning "This library requires at least GCC 4.7."
    #endif
  #endif
#elif defined(_MSC_VER)
  #warning "As of June 2013, Visual Studio's C++11 support was hopelessly behind what is needed to compile this code."
#endif

// =======================================================================================

namespace kj {

typedef unsigned int uint;
typedef unsigned char byte;

// =======================================================================================
// Common macros, especially for common yet compiler-specific features.

// Detect whether RTTI and exceptions are enabled, assuming they are unless we have specific
// evidence to the contrary.  Clients can always define KJ_NO_RTTI or KJ_NO_EXCEPTIONS explicitly
// to override these checks.
#ifdef __GNUC__
  #if !defined(KJ_NO_RTTI) && !__GXX_RTTI
    #define KJ_NO_RTTI 1
  #endif
  #if !defined(KJ_NO_EXCEPTIONS) && !__EXCEPTIONS
    #define KJ_NO_EXCEPTIONS 1
  #endif
#elif defined(_MSC_VER)
  #if !defined(KJ_NO_RTTI) && !defined(_CPPRTTI)
    #define KJ_NO_RTTI 1
  #endif
  #if !defined(KJ_NO_EXCEPTIONS) && !defined(_CPPUNWIND)
    #define KJ_NO_EXCEPTIONS 1
  #endif
#endif

#define KJ_DISALLOW_COPY(classname) \
  classname(const classname&) = delete; \
  classname& operator=(const classname&) = delete
// Deletes the implicit copy constructor and assignment operator.

#define KJ_EXPECT_TRUE(condition) __builtin_expect(condition, true)
#define KJ_EXPECT_FALSE(condition) __builtin_expect(condition, false)
// Branch prediction macros.  Evaluates to the condition given, but also tells the compiler that we
// expect the condition to be true/false enough of the time that it's worth hard-coding branch
// prediction.

#if defined(NDEBUG) && !__NO_INLINE__
#define KJ_ALWAYS_INLINE(prototype) inline prototype __attribute__((always_inline))
// Force a function to always be inlined.  Apply only to the prototype, not to the definition.
#else
#define KJ_ALWAYS_INLINE(prototype) inline prototype
// Don't force inline in debug mode.
#endif

#define KJ_NORETURN __attribute__((noreturn));
#define KJ_UNUSED __attribute__((unused));

#if __clang__
#define KJ_UNUSED_FOR_CLANG __attribute__((unused));
// Clang reports "unused" warnings in some places where GCC does not even allow the "unused"
// attribute.
#else
#define KJ_UNUSED_FOR_CLANG
#endif

namespace internal {

void inlineRequireFailure(
    const char* file, int line, const char* expectation, const char* macroArgs,
    const char* message = nullptr) KJ_NORETURN;

}  // namespace internal

#ifdef NDEBUG
#define KJ_IREQUIRE(condition, ...)
#else
#define KJ_IREQUIRE(condition, ...) \
    if (KJ_EXPECT_TRUE(condition)); else ::kj::internal::inlineRequireFailure( \
        __FILE__, __LINE__, #condition, #__VA_ARGS__, ##__VA_ARGS__)
// Version of KJ_REQUIRE() which is safe to use in headers that are #included by users.  Used to
// check preconditions inside inline methods.  KJ_INLINE_DPRECOND is particularly useful in that
// it will be enabled depending on whether the application is compiled in debug mode rather than
// whether libkj is.
#endif

// #define KJ_STACK_ARRAY(type, name, size, minStack, maxStack)
//
// Allocate an array, preferably on the stack, unless it is too big.  On GCC this will use
// variable-sized arrays.  For other compilers we could just use a fixed-size array.  `minStack`
// is the stack array size to use if variable-width arrays are not supported.  `maxStack` is the
// maximum stack array size if variable-width arrays *are* supported.
#if __clang__
#define KJ_STACK_ARRAY(type, name, size, minStack, maxStack) \
  size_t name##_size = (size); \
  bool name##_isOnStack = name##_size <= (minStack); \
  type name##_stack[minStack]; \
  ::kj::Array<type> name##_heap = name##_isOnStack ? \
      nullptr : kj::heapArray<type>(name##_size); \
  ::kj::ArrayPtr<type> name = name##_isOnStack ? \
      kj::arrayPtr(name##_stack, name##_size) : name##_heap
#else
#define KJ_STACK_ARRAY(type, name, size, minStack, maxStack) \
  size_t name##_size = (size); \
  bool name##_isOnStack = name##_size <= (maxStack); \
  type name##_stack[name##_isOnStack ? size : 0]; \
  ::kj::Array<type> name##_heap = name##_isOnStack ? \
      nullptr : kj::heapArray<type>(name##_size); \
  ::kj::ArrayPtr<type> name = name##_isOnStack ? \
      kj::arrayPtr(name##_stack, name##_size) : name##_heap
#endif

// =======================================================================================
// Template metaprogramming helpers.

template <typename T>
struct NoInfer_ {
  // Use NoInfer<T>::Type in place of T for a template function parameter to prevent inference of
  // the type based on the parameter value.  There's something in the standard library for this but
  // I didn't want to #include type_traits or whatever.
  typedef T Type;
};
template <typename T>
using NoInfer = typename NoInfer_<T>::Type;

template <typename T> struct RemoveReference_ { typedef T Type; };
template <typename T> struct RemoveReference_<T&> { typedef T Type; };
template <typename T> using RemoveReference = typename RemoveReference_<T>::Type;

template <typename> struct IsLvalueReference_ { static constexpr bool value = false; };
template <typename T> struct IsLvalueReference_<T&> { static constexpr bool value = true; };
template <typename T>
inline constexpr bool isLvalueReference() { return IsLvalueReference_<T>::value; }

template <typename T> struct Decay_ { typedef T Type; };
template <typename T> struct Decay_<T&> { typedef typename Decay_<T>::Type Type; };
template <typename T> struct Decay_<T&&> { typedef typename Decay_<T>::Type Type; };
template <typename T> struct Decay_<T[]> { typedef typename Decay_<T*>::Type Type; };
template <typename T> struct Decay_<const T> { typedef typename Decay_<T>::Type Type; };
template <typename T> struct Decay_<volatile T> { typedef typename Decay_<T>::Type Type; };
template <typename T> using Decay = typename Decay_<T>::Type;

template <bool b> struct EnableIf_;
template <> struct EnableIf_<true> { typedef void Type; };
template <bool b> using EnableIf = typename EnableIf_<b>::Type;
// Use like:
//
//     template <typename T, typename = EnableIf<isValid<T>()>
//     void func(T&& t);

template <typename T>
T instance() noexcept;
// Like std::declval, but doesn't transform T into an rvalue reference.  If you want that, specify
// instance<T&&>().

// =======================================================================================
// Equivalents to std::move() and std::forward(), since these are very commonly needed and the
// std header <utility> pulls in lots of other stuff.
//
// We use abbreviated names mv and fwd because these helpers (especially mv) are so commonly used
// that the cost of typing more letters outweighs the cost of being slightly harder to understand
// when first encountered.

template<typename T> constexpr T&& mv(T& t) noexcept { return static_cast<T&&>(t); }

template<typename T>
constexpr T&& fwd(RemoveReference<T>& t) noexcept {
  return static_cast<T&&>(t);
}
template<typename T> constexpr T&& fwd(RemoveReference<T>&& t) noexcept {
  static_assert(!isLvalueReference<T>(), "Attempting to forward rvalue as lvalue reference.");
  return static_cast<T&&>(t);
}

// =======================================================================================
// Manually invoking constructors and destructors
//
// ctor(x, ...) and dtor(x) invoke x's constructor or destructor, respectively.

// We want placement new, but we don't want to #include <new>.  operator new cannot be defined in
// a namespace, and defining it globally conflicts with the definition in <new>.  So we have to
// define a dummy type and an operator new that uses it.

namespace internal {
struct PlacementNew {};
}  // namespace internal
} // namespace kj

inline void* operator new(size_t, kj::internal::PlacementNew, void* __p) noexcept {
  return __p;
}

namespace kj {

template <typename T, typename... Params>
inline void ctor(T& location, Params&&... params) {
  new (internal::PlacementNew(), &location) T(kj::fwd<Params>(params)...);
}

template <typename T>
inline void dtor(T& location) {
  location.~T();
}

// =======================================================================================
// Maybe
//
// Use in cases where you want to indicate that a value may be null.  Using Maybe<T&> instead of T*
// forces the caller to handle the null case in order to satisfy the compiler, thus reliably
// preventing null pointer dereferences at runtime.
//
// Maybe<T> can be implicitly constructed from T and from nullptr.  Additionally, it can be
// implicitly constructed from T*, in which case the pointer is checked for nullness at runtime.
// To read the value of a Maybe<T>, do:
//
//    KJ_IF_MAYBE(value, someFuncReturningMaybe()) {
//      doSomething(*value);
//    } else {
//      maybeWasNull();
//    }
//
// KJ_IF_MAYBE's first parameter is a variable name which will be defined within the following
// block.  The variable will behave like a (guaranteed non-null) pointer to the Maybe's value,
// though it may or may not actually be a pointer.
//
// Note that Maybe<T&> actually just wraps a pointer, whereas Maybe<T> wraps a T and a boolean
// indicating nullness.

template <typename T>
class Maybe;

namespace internal {

template <typename T>
class NullableValue {
  // Class whose interface behaves much like T*, but actually contains an instance of T and a
  // boolean flag indicating nullness.

public:
  inline NullableValue(NullableValue&& other) noexcept(noexcept(T(instance<T&&>())))
      : isSet(other.isSet) {
    if (isSet) {
      ctor(value, kj::mv(other.value));
    }
  }
  inline NullableValue(const NullableValue& other)
      : isSet(other.isSet) {
    if (isSet) {
      ctor(value, other.value);
    }
  }
  inline ~NullableValue() {
    if (isSet) {
      dtor(value);
    }
  }

  inline T& operator*() { return value; }
  inline const T& operator*() const { return value; }
  inline T* operator->() { return &value; }
  inline const T* operator->() const { return &value; }
  inline operator T*() { return isSet ? &value : nullptr; }
  inline operator const T*() const { return isSet ? &value : nullptr; }

private:  // internal interface used by friends only
  inline NullableValue() noexcept: isSet(false) {}
  inline NullableValue(T&& t) noexcept(noexcept(T(instance<T&&>())))
      : isSet(true) {
    ctor(value, kj::mv(t));
  }
  inline NullableValue(const T& t)
      : isSet(true) {
    ctor(value, t);
  }
  inline NullableValue(const T* t)
      : isSet(t != nullptr) {
    if (isSet) ctor(value, *t);
  }
  template <typename U>
  inline NullableValue(NullableValue<U>&& other) noexcept(noexcept(T(instance<U&&>())))
      : isSet(other.isSet) {
    if (isSet) {
      ctor(value, kj::mv(other.value));
    }
  }
  template <typename U>
  inline NullableValue(const NullableValue<U>& other)
      : isSet(other.isSet) {
    if (isSet) {
      ctor(value, other.value);
    }
  }
  template <typename U>
  inline NullableValue(const NullableValue<U&>& other)
      : isSet(other.isSet) {
    if (isSet) {
      ctor(value, *other.ptr);
    }
  }
  inline NullableValue(decltype(nullptr)): isSet(false) {}

  inline NullableValue& operator=(NullableValue&& other) {
    if (&other != this) {
      if (isSet) {
        dtor(value);
      }
      isSet = other.isSet;
      if (isSet) {
        ctor(value, kj::mv(other.value));
      }
    }
    return *this;
  }

  inline NullableValue& operator=(const NullableValue& other) {
    if (&other != this) {
      if (isSet) {
        dtor(value);
      }
      isSet = other.isSet;
      if (isSet) {
        ctor(value, other.value);
      }
    }
    return *this;
  }

  inline bool operator==(decltype(nullptr)) const { return !isSet; }
  inline bool operator!=(decltype(nullptr)) const { return isSet; }

private:
  bool isSet;
  union {
    T value;
  };

  friend class kj::Maybe<T>;
  template <typename U>
  friend NullableValue<U>&& readMaybe(Maybe<U>&& maybe);
};

template <typename T>
inline NullableValue<T>&& readMaybe(Maybe<T>&& maybe) { return kj::mv(maybe.ptr); }
template <typename T>
inline T* readMaybe(Maybe<T>& maybe) { return maybe.ptr; }
template <typename T>
inline const T* readMaybe(const Maybe<T>& maybe) { return maybe.ptr; }
template <typename T>
inline T* readMaybe(Maybe<T&>&& maybe) { return maybe.ptr; }
template <typename T>
inline T* readMaybe(const Maybe<T&>& maybe) { return maybe.ptr; }

}  // namespace internal

#define KJ_IF_MAYBE(name, exp) if (auto name = ::kj::internal::readMaybe(exp))

template <typename T>
class Maybe {
public:
  Maybe(): ptr(nullptr) {}
  Maybe(T&& t) noexcept(noexcept(T(instance<T&&>()))): ptr(kj::mv(t)) {}
  Maybe(const T& t): ptr(t) {}
  Maybe(const T* t) noexcept: ptr(t) {}
  Maybe(Maybe&& other) noexcept(noexcept(T(instance<T&&>()))): ptr(kj::mv(other.ptr)) {}
  Maybe(const Maybe& other): ptr(other.ptr) {}

  template <typename U>
  Maybe(Maybe<U>&& other) noexcept(noexcept(T(instance<U&&>()))) {
    KJ_IF_MAYBE(val, kj::mv(other)) {
      ptr = *val;
    }
  }
  template <typename U>
  Maybe(const Maybe<U>& other) {
    KJ_IF_MAYBE(val, other) {
      ptr = *val;
    }
  }

  Maybe(decltype(nullptr)) noexcept: ptr(nullptr) {}

  inline Maybe& operator=(Maybe&& other) { ptr = kj::mv(other.ptr); return *this; }
  inline Maybe& operator=(const Maybe& other) { ptr = other.ptr; return *this; }

  inline bool operator==(decltype(nullptr)) const { return ptr == nullptr; }
  inline bool operator!=(decltype(nullptr)) const { return ptr != nullptr; }

  ~Maybe() noexcept {}

private:
  internal::NullableValue<T> ptr;

  template <typename U>
  friend class Maybe;
  template <typename U>
  friend internal::NullableValue<U>&& internal::readMaybe(Maybe<U>&& maybe);
  template <typename U>
  friend U* internal::readMaybe(Maybe<U>& maybe);
  template <typename U>
  friend const U* internal::readMaybe(const Maybe<U>& maybe);
};

template <typename T>
class Maybe<T&> {
public:
  Maybe(): ptr(nullptr) {}
  Maybe(T& t) noexcept: ptr(&t) {}
  Maybe(T* t) noexcept: ptr(t) {}
  Maybe(const Maybe& other) noexcept: ptr(other.ptr) {}
  template <typename U>
  Maybe(const Maybe<U&>& other): ptr(other.ptr) {}
  Maybe(decltype(nullptr)) noexcept: ptr(nullptr) {}

  inline Maybe& operator=(const Maybe& other) { ptr = other.ptr; return *this; }

  inline bool operator==(decltype(nullptr)) const { return ptr == nullptr; }
  inline bool operator!=(decltype(nullptr)) const { return ptr != nullptr; }

  ~Maybe() noexcept {}

private:
  T* ptr;

  template <typename U>
  friend class Maybe;
  template <typename U>
  friend U* internal::readMaybe(Maybe<U&>&& maybe);
  template <typename U>
  friend U* internal::readMaybe(const Maybe<U&>& maybe);
};


// =======================================================================================
// ArrayPtr
//
// So common that we put it in common.h rather than array.h.

template <typename T>
class ArrayPtr {
  // A pointer to an array.  Includes a size.  Like any pointer, it doesn't own the target data,
  // and passing by value only copies the pointer, not the target.

public:
  inline constexpr ArrayPtr(): ptr(nullptr), size_(0) {}
  inline constexpr ArrayPtr(decltype(nullptr)): ptr(nullptr), size_(0) {}
  inline constexpr ArrayPtr(T* ptr, size_t size): ptr(ptr), size_(size) {}
  inline constexpr ArrayPtr(T* begin, T* end): ptr(begin), size_(end - begin) {}

  inline operator ArrayPtr<const T>() {
    return ArrayPtr<const T>(ptr, size_);
  }

  inline size_t size() const { return size_; }
  inline T& operator[](size_t index) const {
    KJ_IREQUIRE(index < size_, "Out-of-bounds ArrayPtr access.");
    return ptr[index];
  }

  inline T* begin() const { return ptr; }
  inline T* end() const { return ptr + size_; }
  inline T& front() const { return *ptr; }
  inline T& back() const { return *(ptr + size_ - 1); }

  inline ArrayPtr slice(size_t start, size_t end) const {
    KJ_IREQUIRE(start <= end && end <= size_, "Out-of-bounds ArrayPtr::slice().");
    return ArrayPtr(ptr + start, end - start);
  }

  inline bool operator==(decltype(nullptr)) { return size_ == 0; }
  inline bool operator!=(decltype(nullptr)) { return size_ != 0; }

private:
  T* ptr;
  size_t size_;
};

template <typename T>
inline constexpr ArrayPtr<T> arrayPtr(T* ptr, size_t size) {
  // Use this function to construct ArrayPtrs without writing out the type name.
  return ArrayPtr<T>(ptr, size);
}

template <typename T>
inline constexpr ArrayPtr<T> arrayPtr(T* begin, T* end) {
  // Use this function to construct ArrayPtrs without writing out the type name.
  return ArrayPtr<T>(begin, end);
}

// =======================================================================================
// Upcast/downcast

template <typename To, typename From>
To upcast(From&& from) {
  // `upcast<T>(value)` casts `value` to type `T` only if the conversion is implicit.  Useful for
  // e.g. resolving ambiguous overloads without sacrificing type-safety.
  return kj::fwd<From>(from);
}

template <typename To, typename From>
Maybe<To&> dynamicDowncastIfAvailable(From& from) {
  // If RTTI is disabled, always returns nullptr.  Otherwise, works like dynamic_cast.  Useful
  // in situations where dynamic_cast could allow an optimization, but isn't strictly necessary
  // for correctness.  It is highly recommended that you try to arrange all your dynamic_casts
  // this way, as a dynamic_cast that is necessary for correctness implies a flaw in the interface
  // design.

  // Force a compile error if To is not a subtype of From.  Cross-casting is rare; if it is needed
  // we should have a separate cast function like dynamicCrosscastIfAvailable().
  if (false) {
    kj::upcast<From*>(upcast<RemoveReference<To>*>(nullptr));
  }

#if KJ_NO_RTTI
  return nullptr;
#else
  return dynamic_cast<RemoveReference<To>*>(&from);
#endif
}

template <typename To, typename From>
To downcast(From* from) {
  // Down-cast a value to a sub-type, asserting that the cast is valid.  In opt mode this is a
  // static_cast, but in debug mode (when RTTI is enabled) a dynamic_cast will be used to verify
  // that the value really has the requested type.

  // Force a compile error if To is not a subtype of From.
  if (false) {
    kj::upcast<From*>(To());
  }

#if !KJ_NO_RTTI
  KJ_IREQUIRE(from == nullptr || dynamic_cast<To>(from) != nullptr,
              "Value cannot be downcast() to requested type.");
#endif

  return static_cast<To>(from);
}

template <typename To, typename From>
To downcast(From& from) {
  // Reference version of downcast().
  return *kj::downcast<RemoveReference<To>*>(&from);
}

}  // namespace kj

#endif  // KJ_COMMON_H_
