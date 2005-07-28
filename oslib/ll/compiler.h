

#if !defined(_GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)

#define likely(x)	x
#define unlikely(x)	x

#else

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#endif
