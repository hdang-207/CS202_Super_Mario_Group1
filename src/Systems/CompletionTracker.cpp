#include "Systems/CompletionTracker.hpp"
#include <fstream>
#include <iostream>

namespace Systems {

CompletionTracker& CompletionTracker::getInstance() {
    static CompletionTracker instance;
    return instance;
}

CompletionTracker::CompletionTracker() {
    load();
}

void CompletionTracker::markWorldComplete(GameMode mode, int world) {
    if (world < 1 || world > 3) return;
    int idx = world - 1;
    if (mode == GameMode::Nightfall) {
        m_nightfallWorld[idx] = true;
    } else if (mode == GameMode::Inferno || mode == GameMode::Apocalypse) {
        // Apocalypse counts as Inferno completion since it includes Inferno mechanics
        m_infernoWorld[idx] = true;
    }
    save();
    std::cout << "[CompletionTracker] Marked " 
              << (mode == GameMode::Nightfall ? "Nightfall" : "Inferno")
              << " World " << world << " as complete.\n";
    if (isApocalypseUnlocked()) {
        std::cout << "[CompletionTracker] *** APOCALYPSE MODE UNLOCKED! ***\n";
    }
}

bool CompletionTracker::isApocalypseUnlocked() const {
    if (m_forceUnlocked) return true;
    for (int i = 0; i < 3; ++i) {
        if (!m_nightfallWorld[i] || !m_infernoWorld[i]) return false;
    }
    return true;
}

void CompletionTracker::forceUnlockApocalypse() {
    m_forceUnlocked = true;
    save();
    std::cout << "[CompletionTracker] Apocalypse Mode force-unlocked via debug command.\n";
}

void CompletionTracker::resetUnlockState() {
    m_forceUnlocked = false;
    for (int i = 0; i < 3; ++i) {
        m_nightfallWorld[i] = false;
        m_infernoWorld[i] = false;
    }
    save();
    std::cout << "[CompletionTracker] All completion data reset. Apocalypse locked.\n";
}

void CompletionTracker::load() {
    std::ifstream file(kFilePath);
    if (!file.is_open()) return;

    // Format: 6 booleans (nightfall w1-3, inferno w1-3), then optional force flag
    for (int i = 0; i < 3; ++i) {
        int val = 0;
        if (file >> val) m_nightfallWorld[i] = (val != 0);
    }
    for (int i = 0; i < 3; ++i) {
        int val = 0;
        if (file >> val) m_infernoWorld[i] = (val != 0);
    }
    int forceVal = 0;
    if (file >> forceVal) m_forceUnlocked = (forceVal != 0);

    std::cout << "[CompletionTracker] Loaded completion data. Apocalypse "
              << (isApocalypseUnlocked() ? "UNLOCKED" : "locked") << ".\n";
}

void CompletionTracker::save() const {
    std::ofstream file(kFilePath);
    if (!file.is_open()) {
        std::cerr << "[CompletionTracker] Failed to save completion data.\n";
        return;
    }
    for (int i = 0; i < 3; ++i) file << (m_nightfallWorld[i] ? 1 : 0) << "\n";
    for (int i = 0; i < 3; ++i) file << (m_infernoWorld[i] ? 1 : 0) << "\n";
    file << (m_forceUnlocked ? 1 : 0) << "\n";
}

} // namespace Systems
