#pragma once
#include <string>
#include <vector>

class PlayState;   // forward declaration

namespace Systems {

struct CommandResult {
    bool success;
    std::string message;
};

class CommandParser {
public:
    explicit CommandParser(PlayState& state);

    CommandResult execute(const std::string& input);

private:
    PlayState& playState;

    static std::vector<std::string> tokenize(const std::string& input);

    CommandResult handleFly(const std::vector<std::string>& args);
    CommandResult handleGod(const std::vector<std::string>& args);
    CommandResult handleTp(const std::vector<std::string>& args);
    CommandResult handleSummon(const std::vector<std::string>& args);
    CommandResult handleForm(const std::vector<std::string>& args);
    CommandResult handleMusic(const std::vector<std::string>& args);
    CommandResult handleHelp();
    CommandResult handleReset(const std::vector<std::string>& args);
    CommandResult handleLives(const std::vector<std::string>& args);
    CommandResult handleDestroyer(const std::vector<std::string>& args);
};

} // namespace Systems
