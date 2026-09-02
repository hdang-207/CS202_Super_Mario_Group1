#pragma once

#include <cstddef>
#include <iterator>

namespace duel {

/**
 * @struct ArenaDefinition
 * @brief One selectable duel arena: its map file and how it is presented.
 */
struct ArenaDefinition {
    const char* file;        ///< Map path below the assets root.
    const char* name;        ///< Label shown on the arena selection screen.
    const char* description; ///< One line telling players how the arena fights.
};

/// Every arena is mirrored left to right so neither player gets the better side.
inline constexpr ArenaDefinition kArenas[] = {
    {"assets/maps/duel_arena.txt", "CLASSIC",
     "TWO LEDGES AND A CENTRE PLATFORM - NOWHERE TO HIDE"},
    {"assets/maps/duel_arena_towers.txt", "SKY TOWERS",
     "STACKED PLATFORMS - THE HIGH GROUND WINS THE STOMP"},
    {"assets/maps/duel_arena_chasm.txt", "CHASM",
     "A PIT SPLITS THE FLOOR - ONE SLIP ENDS THE ROUND"},
};

inline constexpr std::size_t kArenaCount = std::size(kArenas);

/// Choice that rolls a different arena at the start of every match.
inline constexpr std::size_t kRandomArena = kArenaCount;

/// Entries offered by the selection screen, the random roll included.
inline constexpr std::size_t kArenaChoiceCount = kArenaCount + 1;

/// True when @p choice means "surprise us" rather than a fixed arena.
inline constexpr bool isRandomArena(std::size_t choice) {
    return choice >= kArenaCount;
}

/// Label of any choice, including the random one which has no map of its own.
inline constexpr const char* arenaChoiceName(std::size_t choice) {
    return isRandomArena(choice) ? "RANDOM" : kArenas[choice].name;
}

/// Description of any choice, including the random one.
inline constexpr const char* arenaChoiceDescription(std::size_t choice) {
    return isRandomArena(choice)
        ? "A DIFFERENT ARENA EVERY MATCH"
        : kArenas[choice].description;
}

/// Next entry on the selection screen, wrapping past the random roll.
inline constexpr std::size_t nextArenaChoice(std::size_t choice) {
    return (choice + 1) % kArenaChoiceCount;
}

/// Previous entry on the selection screen, wrapping past the first arena.
inline constexpr std::size_t previousArenaChoice(std::size_t choice) {
    return (choice + kArenaChoiceCount - 1) % kArenaChoiceCount;
}

} // namespace duel
