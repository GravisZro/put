#include "blockinfo.h"

// https://stackoverflow.com/questions/19747663/where-are-ioctl-parameters-such-as-0x1268-blksszget-actually-specified

#if defined(__linux__)

#elif defined(__unix__)
#error Detection of block partitions is not implemented in PUT for this UNIX!  Please submit a patch!

#else
#error This platform is not supported.
#endif
