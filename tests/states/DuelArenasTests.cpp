#include "States/DuelArenas.hpp"

#include "Systems/MapParser.hpp"
#include "Systems/ResourcePath.hpp"

#include <cstddef>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cout << "[FAIL] " << message << '\n';
    }
}

void testOnlyTheLastChoiceIsRandom() {
    for (std::size_t choice = 0; choice < duel::kArenaCount; ++choice) {
        expect(
            !duel::isRandomArena(choice),
            "arena " + std::to_string(choice) + " should be a fixed map"
        );
    }
    expect(
        duel::isRandomArena(duel::kRandomArena),
        "the sentinel choice should be the random roll"
    );
    expect(
        duel::kArenaChoiceCount == duel::kArenaCount + 1,
        "the screen should offer every arena plus the random roll"
    );
}

void testBrowsingWrapsBothWays() {
    std::size_t choice = 0;
    for (std::size_t step = 0; step < duel::kArenaChoiceCount; ++step) {
        choice = duel::nextArenaChoice(choice);
    }
    expect(choice == 0, "browsing forward through every entry should wrap home");

    expect(
        duel::previousArenaChoice(0) == duel::kRandomArena,
        "browsing left from the first arena should land on the random roll"
    );
    expect(
        duel::nextArenaChoice(duel::kRandomArena) == 0,
        "browsing right from the random roll should land on the first arena"
    );
}

void testEveryChoiceIsLabelled() {
    for (std::size_t choice = 0; choice < duel::kArenaChoiceCount; ++choice) {
        const std::string name = duel::arenaChoiceName(choice);
        const std::string description = duel::arenaChoiceDescription(choice);
        expect(!name.empty(), "choice " + std::to_string(choice) + " needs a name");
        expect(
            !description.empty(),
            "choice " + std::to_string(choice) + " needs a description"
        );
    }

    for (std::size_t left = 0; left < duel::kArenaCount; ++left) {
        for (std::size_t right = left + 1; right < duel::kArenaCount; ++right) {
            expect(
                std::string(duel::kArenas[left].name)
                    != std::string(duel::kArenas[right].name),
                "arena names must be unique so players can tell them apart"
            );
            expect(
                std::string(duel::kArenas[left].file)
                    != std::string(duel::kArenas[right].file),
                "two arenas must not share one map file"
            );
        }
    }
}

// DuelState refuses to run an arena that breaks any of these rules, and a
// rejected arena is a blank screen at runtime rather than a build failure.
void testEveryArenaFileLoadsAndIsPlayable() {
    constexpr std::size_t expectedColumns = 24;
    constexpr std::size_t expectedRows = 15;

    for (const duel::ArenaDefinition& arena : duel::kArenas) {
        const std::string label = arena.name;
        MapParser parser;
        if (!parser.loadFromFile(Systems::resourcePath(arena.file))) {
            ++failures;
            std::cout << "[FAIL] " << label << ": map file could not be opened\n";
            continue;
        }

        expect(
            parser.getWidth() == expectedColumns
                && parser.getHeight() == expectedRows,
            label + ": arena must be 24x15 tiles"
        );

        std::size_t spawnCount = 0;
        std::size_t firstSpawnColumn = 0;
        std::size_t lastSpawnColumn = 0;
        bool rowWidthsValid = true;

        const auto& grid = parser.getGrid();
        for (const auto& row : grid) {
            rowWidthsValid = rowWidthsValid && row.size() == expectedColumns;
            for (std::size_t column = 0; column < row.size(); ++column) {
                if (row[column] != 'P') {
                    continue;
                }
                if (spawnCount == 0) {
                    firstSpawnColumn = column;
                }
                lastSpawnColumn = column;
                ++spawnCount;
            }
        }

        expect(rowWidthsValid, label + ": every row must be 24 tiles wide");
        expect(spawnCount == 2, label + ": arena needs exactly two player spawns");
        expect(
            spawnCount != 2 || firstSpawnColumn < lastSpawnColumn,
            label + ": spawns must be ordered left to right"
        );
    }
}

} // namespace

int main() {
    testOnlyTheLastChoiceIsRandom();
    testBrowsingWrapsBothWays();
    testEveryChoiceIsLabelled();
    testEveryArenaFileLoadsAndIsPlayable();

    if (failures == 0) {
        std::cout << "[PASS] All duel arena catalogue tests passed.\n";
        return 0;
    }

    std::cout << failures << " duel arena catalogue test(s) failed.\n";
    return 1;
}
