//
//  memory.cpp
//  Paladin
//
//  Created by SATAN_Z on 2019/7/17.
//  Copyright © 2019 Zero. All rights reserved.
//

#include "memory.hpp"
#include <stdlib.h>
PALADIN_BEGIN

// Memory Allocation Functions
void *allocAligned(size_t size) {
#if defined(PALADIN_HAVE__ALIGNED_MALLOC)
    return _aligned_malloc(size, PALADIN_L1_CACHE_LINE_SIZE);
#elif defined(PALADIN_HAVE_POSIX_MEMALIGN)
    void *ptr;
    if (posix_memalign(&ptr, PALADIN_L1_CACHE_LINE_SIZE, size) != 0) ptr = nullptr;
    return ptr;
#else
    return memalign(PALADIN_L1_CACHE_LINE_SIZE, size);
#endif
}

void freeAligned(void *ptr) {
    if (!ptr) return;
#if defined(PALADIN_HAVE__ALIGNED_MALLOC)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

PALADIN_END
