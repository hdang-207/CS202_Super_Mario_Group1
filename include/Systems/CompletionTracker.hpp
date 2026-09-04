#pragma once

#include "Core/GameMode.hpp"
#include <string>

namespace Systems {

/// Tracks persistent completion of worlds per game mode.
/// Used to determine if Apocalypse Mode should be unlocked.
class CompletionTracker {
public:
    static CompletionTracker& getInstance();

    /// Call when a world is completed in Nightfall or Inferno.
    void markWorldComplete(GameMode mode, int world);

    /// Returns true if all 3 worlds are complete in both Nightfall and Inferno.
    bool isApocalypseUnlocked() const;

    /// Debug: force-unlock Apocalypse Mode.
    void forceUnlockApocalypse();

    /// Debug: lock Apocalypse Mode (reset everything).
    void resetUnlockState();

    /// Load state from disk.
    void load();
    /// Save state to disk.
    void save() const;

private:
    CompletionTracker();

    static constexpr const char* kFilePath = "completion.dat";

    bool m_nightfallWorld[3]{false, false, false};
    bool m_infernoWorld[3]{false, false, false};
    bool m_forceUnlocked{false};
};

} // namespace Systems
