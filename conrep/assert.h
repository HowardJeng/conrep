#ifndef CONREP_ASSERT_H
#define CONREP_ASSERT_H

#ifdef ASSERT_DIABLED
  #define ASSERT(exp) do { (void)sizeof(exp); __assume(exp); } while (0)
#else
  #define ASSERT(exp)                                                   \
    do {                                                                \
      if (!(exp)) {                                                     \
        if (::console::assert_halt_first) { __debugbreak(); }           \
        ::console::assert_impl(#exp, __FILE__, __FUNCTION__, __LINE__); \
      }                                                                 \
      __assume(exp);                                                    \
    } while (0)
#endif

namespace console {
  extern const bool assert_halt_first;

  __declspec(noreturn) void assert_impl(const char * expression, const char * file, const char * function, unsigned line);
}

#endif