#pragma once
#include <string>
#include <vector>

class PlayState;

namespace Core {

struct CommandResult {
    bool success;
    std::string message;
};

class CommandParser {
private:
    PlayState& playState;
    
    std::vector<std::string> tokenize(const std::string& input);
    
    CommandResult handleSet(const std::vector<std::string>& args);
    CommandResult handleGod(const std::vector<std::string>& args);
    CommandResult handleTp(const std::vector<std::string>& args);
    CommandResult handleLevel(const std::vector<std::string>& args);
    CommandResult handleSpawn(const std::vector<std::string>& args);
    CommandResult handleKill(const std::vector<std::string>& args);
    CommandResult handleSpeed(const std::vector<std::string>& args);
    CommandResult handleGive(const std::vector<std::string>& args);
    CommandResult handleTime(const std::vector<std::string>& args);
    CommandResult handleNoclip(const std::vector<std::string>& args);
    CommandResult handleFly(const std::vector<std::string>& args);
    CommandResult handleHelp();

public:
    explicit CommandParser(PlayState& state);
    
    CommandResult execute(const std::string& input);
};

} // namespace Core
