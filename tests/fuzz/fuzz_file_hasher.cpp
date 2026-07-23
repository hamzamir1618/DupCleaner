#include "hasher.h"
#include <stdint.h>
#include <stddef.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0) return 0;
    
    // Call the newly exposed byte-buffer interface
    uint64_t hash = dupcleaner::FileHasher::fingerprint(data, size);
    
    return 0; // Non-zero return values are reserved for future use
}
