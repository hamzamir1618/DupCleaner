#include "hasher.h"
#include "dupcleaner/exceptions.h"
#include "xxhash.h"
#include <fstream>
#include <vector>

namespace dupcleaner {

uint64_t FileHasher::fingerprint(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw DupCleanerIOException("Failed to open file for hashing: " + path.string());
    }

    XXH64_state_t* state = XXH64_createState();
    if (!state) {
        throw DupCleanerIOException("Failed to create xxHash state for: " + path.string());
    }

    if (XXH64_reset(state, 0) == XXH_ERROR) {
        XXH64_freeState(state);
        throw DupCleanerIOException("Failed to reset xxHash state for: " + path.string());
    }

    constexpr size_t BUFFER_SIZE = 64ULL * 1024ULL; // 64 KB chunks
    std::vector<char> buffer(BUFFER_SIZE);

    while (file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()))) {
        if (XXH64_update(state, buffer.data(), file.gcount()) == XXH_ERROR) {
            XXH64_freeState(state);
            throw DupCleanerIOException("Failed to update xxHash during read for: " + path.string());
        }
    }

    // Process any remaining bytes after loop
    if (file.gcount() > 0) {
        if (XXH64_update(state, buffer.data(), file.gcount()) == XXH_ERROR) {
            XXH64_freeState(state);
            throw DupCleanerIOException("Failed to update xxHash for final block: " + path.string());
        }
    }

    // Check for file read errors other than EOF
    if (file.bad()) {
        XXH64_freeState(state);
        throw DupCleanerIOException("Error occurred while reading file for hashing: " + path.string());
    }

    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);

    return hash;
}

} // namespace dupcleaner
