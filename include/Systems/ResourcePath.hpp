#pragma once
#include <string>

namespace Systems {
    /**
     * @brief Turns a repo-relative asset path into one that opens on any machine.
     * @param relative Path below the assets root, e.g. "assets/maps/level1-1.txt".
     * @return The first candidate that exists on disk, or @p relative unchanged
     *         when none does (so the caller still reports a sensible error).
     *
     * CMake copies assets/ next to the executable, but the working directory
     * depends on how the game was started: from bin/, from build/, from the repo
     * root, or from an IDE. This walks those candidates and finally falls back to
     * the source tree baked in at configure time, so nobody has to hardcode their
     * own home directory.
     */
    std::string resourcePath(const std::string& relative);
}
