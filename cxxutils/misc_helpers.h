#ifndef MISC_HELPERS_H
#define MISC_HELPERS_H

// PUT
#include <cxxutils/posix_helpers.h>

#ifndef arraylength
template <typename T, posix::size_t N> char (&ArrayLengthHelper(T (&arr)[N]))[N];
#define arraylength(arr) (sizeof(ArrayLengthHelper(arr)))
#endif

#endif // MISC_HELPERS_H
