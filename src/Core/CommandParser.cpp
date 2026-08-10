#include "Core/CommandParser.hpp"
#include "States/PlayState.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Core {

CommandParser::CommandParser(PlayState& state) : playState(state) {}

std::vector<std::string> CommandParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream stream(input);
    std::string token;
    while (stream >> token) {
        // Convert to lowercase
        std::transform(token.begin(), token.end(), token.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        tokens.push_back(token);
    }
    return tokens;
}

CommandResult CommandParser::execute(const std::string& input) {
    auto tokens = tokenize(input);
    if (tokens.empty()) return {false, ""};
    
    const std::string& cmd = tokens[0];
    
    if (cmd == "help") return handleHelp();
    if (cmd == "set") return handleSet(tokens);
    if (cmd == "god") return handleGod(tokens);
    if (cmd == "tp") return handleTp(tokens);
    if (cmd == "level") return handleLevel(tokens);
    if (cmd == "spawn") return handleSpawn(tokens);
    if (cmd == "kill") return handleKill(tokens);
    if (cmd == "speed") return handleSpeed(tokens);
    if (cmd == "give") return handleGive(tokens);
    if (cmd == "time") return handleTime(tokens);
    if (cmd == "noclip") return handleNoclip(tokens);
    if (cmd == "fly") return handleFly(tokens);
    
    return {false, "Unknown command: " + cmd + ". Type 'help' for a list of commands."};
}

CommandResult CommandParser::handleSet(const std::vector<std::string>& args) {
    if (args.size() < 3) return {false, "Usage: set <lives|score|coins|form> <value>"};
    
    const std::string& target = args[1];
    const std::string& value = args[2];
    
    try {
        if (target == "lives") {
            int v = std::stoi(value);
            if (v < 1 || v > 99) return {false, "Lives must be between 1 and 99"};
            playState.setLives(v);
            return {true, "Lives set to " + std::to_string(v)};
        } else if (target == "score") {
            int v = std::stoi(value);
            playState.setScore(v);
            return {true, "Score set to " + std::to_string(v)};
        } else if (target == "coins") {
            int v = std::stoi(value);
            if (v < 0 || v > 999) return {false, "Coins must be between 0 and 999"};
            playState.setCoins(v);
            return {true, "Coins set to " + std::to_string(v)};
        } else if (target == "form") {
            if (value == "fire") {
                playState.setGiantMode(false);
                playState.setForm(PlayState::AvatarForm::Fire);
                return {true, "Form set to fire"};
            } else if (value == "normal") {
                playState.setGiantMode(false);
                playState.setForm(PlayState::AvatarForm::Normal);
                return {true, "Form set to normal"};
            } else if (value == "giant") {
                playState.setGiantMode(true);
                return {true, "Form set to giant"};
            }
            return {false, "Invalid form. Use fire, normal, or giant"};
        }
    } catch (const std::exception&) {
        return {false, "Invalid number format for " + target};
    }
    
    return {false, "Unknown set target: " + target};
}

CommandResult CommandParser::handleGod(const std::vector<std::string>& args) {
    bool on = true;
    if (args.size() > 1) {
        if (args[1] == "off" || args[1] == "false" || args[1] == "0") on = false;
    }
    playState.setGodMode(on);
    return {true, "God mode " + std::string(on ? "enabled" : "disabled")};
}

CommandResult CommandParser::handleTp(const std::vector<std::string>& args) {
    if (args.size() < 3) return {false, "Usage: tp <x> <y>"};
    try {
        float x = std::stof(args[1]);
        float y = std::stof(args[2]);
        playState.teleport(x, y);
        return {true, "Teleported to (" + std::to_string(x) + ", " + std::to_string(y) + ")"};
    } catch (const std::exception&) {
        return {false, "Invalid coordinates"};
    }
}

CommandResult CommandParser::handleLevel(const std::vector<std::string>& args) {
    if (args.size() < 2) return {false, "Usage: level <n>"};
    try {
        int v = std::stoi(args[1]);
        if (v < 1 || v > 3) return {false, "Level must be between 1 and 3"};
        playState.warpToLevel(v);
        return {true, "Warping to level " + std::to_string(v)};
    } catch (const std::exception&) {
        return {false, "Invalid level number"};
    }
}

CommandResult CommandParser::handleSpawn(const std::vector<std::string>& args) {
    if (args.size() < 2) return {false, "Usage: spawn <goomba|koopa>"};
    if (args[1] == "goomba") {
        playState.spawnEnemyAtPlayer(PlayState::EnemyKind::Goomba);
        return {true, "Spawned Goomba"};
    } else if (args[1] == "koopa") {
        playState.spawnEnemyAtPlayer(PlayState::EnemyKind::BlueKoopa);
        return {true, "Spawned Koopa"};
    }
    return {false, "Unknown enemy type: " + args[1]};
}

CommandResult CommandParser::handleKill(const std::vector<std::string>& args) {
    if (args.size() > 1 && args[1] == "all") {
        playState.killAllEnemies();
        return {true, "Killed all enemies"};
    }
    return {false, "Usage: kill all"};
}

CommandResult CommandParser::handleSpeed(const std::vector<std::string>& args) {
    if (args.size() < 2) return {false, "Usage: speed <multiplier>"};
    try {
        float v = std::stof(args[1]);
        if (v <= 0) return {false, "Speed multiplier must be > 0"};
        playState.setSpeedMultiplier(v);
        return {true, "Speed set to " + std::to_string(v) + "x"};
    } catch (const std::exception&) {
        return {false, "Invalid speed multiplier"};
    }
}

CommandResult CommandParser::handleGive(const std::vector<std::string>& args) {
    if (args.size() < 2) return {false, "Usage: give <flower|mushroom>"};
    playState.giveItem(args[1]);
    return {true, "Gave " + args[1]};
}

CommandResult CommandParser::handleTime(const std::vector<std::string>& args) {
    if (args.size() < 2) return {false, "Usage: time <seconds>"};
    try {
        float v = std::stof(args[1]);
        playState.setTime(v);
        return {true, "Time set to " + std::to_string(v) + "s"};
    } catch (const std::exception&) {
        return {false, "Invalid time value"};
    }
}

CommandResult CommandParser::handleNoclip(const std::vector<std::string>& args) {
    bool on = true;
    if (args.size() > 1) {
        if (args[1] == "off" || args[1] == "false" || args[1] == "0") on = false;
    }
    playState.setNoclipMode(on);
    return {true, "Noclip mode " + std::string(on ? "enabled" : "disabled")};
}

CommandResult CommandParser::handleFly(const std::vector<std::string>& args) {
    bool on = true;
    if (args.size() > 1) {
        if (args[1] == "off" || args[1] == "false" || args[1] == "0") on = false;
    }
    playState.setFlyMode(on);
    return {true, "Fly mode " + std::string(on ? "enabled" : "disabled")};
}

CommandResult CommandParser::handleHelp() {
    std::string helpText = 
        "Commands:\n"
        "set <lives|score|coins|form> <val>\n"
        "god [on|off]\n"
        "tp <x> <y>\n"
        "level <1-3>\n"
        "spawn <goomba|koopa>\n"
        "kill all\n"
        "speed <mult>\n"
        "give <flower|mushroom>\n"
        "time <sec>\n"
        "noclip [on|off]\n"
        "fly [on|off]";
    return {true, helpText};
}

} // namespace Core
