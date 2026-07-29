#include "Systems/ResourcePath.hpp"
#include <filesystem>

namespace {
    /// Directories the assets/ folder is looked for in, nearest first.
    const char* kSearchRoots[] = {
        "",           // launched from the folder that holds assets/ (bin/)
        "../",        // launched from build/
        "../../",     // launched from the repo root with a build/bin/ layout
        "../../../",
    };
}

namespace Systems {
    std::string resourcePath(const std::string& relative) {
        std::error_code ec;
        for (const char* root : kSearchRoots) {
            std::filesystem::path candidate = std::filesystem::path(root) / relative;
            if (std::filesystem::exists(candidate, ec)) {
                return candidate.string();
            }
        }

#ifdef MARIO_SOURCE_DIR
        // Last resort: the source tree this build was configured from. Keeps the
        // game runnable straight out of an IDE, whatever working directory it picks.
        std::filesystem::path fromSource = std::filesystem::path(MARIO_SOURCE_DIR) / relative;
        if (std::filesystem::exists(fromSource, ec)) {
            return fromSource.string();
        }
#endif

        return relative;
    }
}
