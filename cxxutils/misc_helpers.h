#ifndef MISC_HELPERS_H
#define MISC_HELPERS_H

// POSIX
#include <sys/types.h>

#ifndef arraylength
template <typename T, size_t N> char (&ArrayLengthHelper(T (&arr)[N]))[N];
#define arraylength(arr) (sizeof(ArrayLengthHelper(arr)))
#endif

#endif // MISC_HELPERS_H
