/* architecture setup */

#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || \
        defined(_M_IX86) || defined(__i386) || defined(_X86_)
#  define HAVE_ARCH_INTEL 1
#  define HAVE_ARCH_X86 1
#  define HAVE_32BIT 1
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || \
        defined(_M_X64) || defined(_M_AMD64)
#  define HAVE_ARCH_INTEL 1
#  define HAVE_ARCH_X86_64 1
#  define HAVE_64BIT 1
#endif

#if defined(__powerpc__) || defined(__powerpc) || defined(_M_PPC) || \
    defined(__POWERPC__) || defined(__ppc)
#  define HAVE_ARCH_POWERPC 1
#  if defined(__powerpc64__) || defined(__ppc64__) || _M_PPC>=620 || defined(__arch64__)
#    define HAVE_ARCH_POWERPC64 1
#    define HAVE_64BIT 1
#  else
#    define HAVE_ARCH_POWERPC32 1
#    define HAVE_32BIT 1
#  endif
#endif

#if defined(__arm__) || defined(__thumb__) || defined(_M_ARM) || defined(_M_ARMT)
#  define HAVE_ARCH_ARM 1
#  if defined(__aarch64__)
#    define HAVE_ARCH_ARM64 1
#    define HAVE_64BIT 1
#  else
#    define HAVE_ARCH_ARM32 1
#    define HAVE_32BIT 1
#  endif
#endif

#if !defined(HAVE_ARCH_ARM) && defined(__aarch64__)
#  define HAVE_ARCH_ARM 1
#  define HAVE_ARCH_ARM64 1
#  define HAVE_64BIT 1
#endif

/* endianness */
#if defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || defined(_MIPSEB)  || defined(__MIPSEB__) || \
    defined(__MIPSEB)
#  define HAVE_BIG_ENDIAN 1
#else
#  define HAVE_LITTLE_ENDIAN 1
#endif

/* machine bits */
#if !defined(HAVE_32BIT) && !defined(HAVE_64BIT)
#  ifdef _WIN64
#    define HAVE_64BIT 1
#  else
#    ifdef _WIN32
#      define HAVE_32BIT 1
#    endif
#  endif
#endif

#if !defined(HAVE_32BIT) && !defined(HAVE_64BIT)
#  if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__==8
#    define HAVE_64BIT 1
#  else
#    define HAVE_32BIT 1
#  endif
#endif

#ifdef HAVE_32BIT
#error "This is not 64-bit system"
#endif

int main()
{
    return 0;
}
