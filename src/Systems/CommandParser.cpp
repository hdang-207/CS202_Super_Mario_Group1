#include "Systems/CommandParser.hpp"
#include "States/PlayState.hpp"
#include "States/GameStateManager.hpp"
#include "Entities/EntityFactory.hpp"
#include "Systems/SoundController.hpp"
#include "Systems/ResourcePath.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Systems {

CommandParser::CommandParser(PlayState& state) : playState(state) {}

std::vector<std::string> CommandParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream stream(input);
    std::string token;
    while (stream >> token) {
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
    
    if (cmd == "music") return handleMusic(tokens);
    if (cmd == "help") return handleHelp();
    if (cmd == "reset") return handleReset(tokens);
    
    // Check cheats
    if (cmd == "fly" || cmd == "god" || cmd == "tp" || cmd == "summon" || cmd == "form" || cmd == "lives" || cmd == "destroyer") {
        if (!playState.gsm.isCheatsEnabled()) {
            return {false, "Error: Cheats are disabled in settings."};
        }
    }
    
    if (cmd == "fly") return handleFly(tokens);
    if (cmd == "god") return handleGod(tokens);
    if (cmd == "tp") return handleTp(tokens);
    if (cmd == "summon") return handleSummon(tokens);
    if (cmd == "form") return handleForm(tokens);
    if (cmd == "lives") return handleLives(tokens);
    if (cmd == "destroyer") return handleDestroyer(tokens);
    
    return {false, "Unknown command: " + cmd + ". Type 'help' for a list."};
}

CommandResult CommandParser::handleFly(const std::vector<std::string>& args) {
    bool on = true;
    if (args.size() > 1 && (args[1] == "off" || args[1] == "false" || args[1] == "0")) {
        on = false;
    }
    playState.m_flyMode = on;
    return {true, "Fly mode " + std::string(on ? "enabled" : "disabled")};
}

CommandResult CommandParser::handleGod(const std::vector<std::string>& args) {
    bool on = true;
    if (args.size() > 1 && (args[1] == "off" || args[1] == "false" || args[1] == "0")) {
        on = false;
    }
    playState.m_godMode = on;
    playState.m_flyMode = on; // god mode implies fly mode
    return {true, "God mode " + std::string(on ? "enabled" : "disabled")};
}

CommandResult CommandParser::handleTp(const std::vector<std::string>& args) {
    if (args.size() < 2 || args[1] != "spawn") {
        return {false, "Usage: tp spawn"};
    }
    if (playState.m_player) {
        playState.warpAvatarTo(playState.tileMap.playerSpawn());
    }
    return {true, "Teleported to spawn"};
}

CommandResult CommandParser::handleSummon(const std::vector<std::string>& args) {
    if (args.size() < 2) return {false, "Usage: summon <goomba|koopa|paratroopa> [count]"};
    int count = 1;
    if (args.size() >= 3) {
        try { count = std::stoi(args[2]); }
        catch (...) { return {false, "Invalid count"}; }
    }
    if (count < 1 || count > 100) return {false, "Count must be between 1 and 100"};
    
    const std::string& type = args[1];
    entity::EnemyType enemyType = entity::EnemyType::Goomba;
    if (type == "koopa") enemyType = entity::EnemyType::GreenKoopa;
    else if (type == "paratroopa") enemyType = entity::EnemyType::GreenParatroopa;
    
    if (playState.m_player) {
        sf::Vector2f pos = playState.m_player->getPhysicsBody().getPosition();
        pos.x += 100.f; // spawn ahead
        pos.y -= 20.f;
        
        for (int i = 0; i < count; ++i) {
            sf::Vector2f p = pos;
            p.x += i * 20.f;
            
            if (enemyType == entity::EnemyType::Goomba) {
                playState.m_entityManager.addEntity(entity::EntityFactory::createGoomba(p, playState.tileMap.tileSize(), &playState.assets.getTexture("Goomba")));
            } else if (enemyType == entity::EnemyType::GreenKoopa) {
                playState.m_entityManager.addEntity(entity::EntityFactory::createKoopa(p, playState.tileMap.tileSize(), &playState.assets.getTexture("GreenKoopa"), &playState.assets.getTexture("GreenShell")));
            } else if (enemyType == entity::EnemyType::GreenParatroopa) {
                playState.m_entityManager.addEntity(entity::EntityFactory::createParatroopa(p, playState.tileMap.tileSize(), &playState.assets.getTexture("GreenParatroopa"), &playState.assets.getTexture("GreenShell")));
            }
        }
    }
    return {true, "Summoned " + std::to_string(count) + " " + args[1]};
}

CommandResult CommandParser::handleForm(const std::vector<std::string>& args) {
    if (args.size() < 2) return {false, "Usage: form <0|1|2> (0=Normal, 1=Super, 2=Fire)"};
    try {
        int formId = std::stoi(args[1]);
        if (formId < 0 || formId > 2) return {false, "Form must be 0, 1, or 2"};
        if (playState.m_player) {
            // Clear powers
            while(playState.m_player->isSuper() || playState.m_player->hasFirePower()) {
                playState.m_player->removeLatestPower();
            }
            if (formId == 1) { // Super
                playState.m_player->applyPower(entity::PowerType::Super);
            } else if (formId == 2) { // Fire
                playState.m_player->applyPower(entity::PowerType::Super);
                playState.m_player->applyPower(entity::PowerType::Fire);
            }
            playState.syncAvatarPowerVisuals();
        }
        return {true, "Form changed"};
    } catch (...) {
        return {false, "Invalid form ID"};
    }
}

CommandResult CommandParser::handleMusic(const std::vector<std::string>& args) {
    if (args.size() < 2) return {false, "Usage: music <play|stop|volume> [value]"};
    const std::string& action = args[1];
    
    if (action == "stop") {
        Systems::SoundController::getInstance().stopMusic();
        playState.m_musicLocked = true;
        return {true, "Music stopped"};
    } else if (action == "play") {
        if (args.size() < 3) return {false, "Usage: music play <path>"};
        std::string path = args[2];
        for (size_t i = 3; i < args.size(); ++i) path += " " + args[i];
        Systems::SoundController::getInstance().playMusic(Systems::resourcePath(path));
        playState.m_musicLocked = true;
        return {true, "Playing music: " + path};
    } else if (action == "volume") {
        if (args.size() < 3) return {false, "Usage: music volume <0-100>"};
        try {
            float vol = std::stof(args[2]);
            Systems::SoundController::getInstance().setMusicVolume(vol);
            return {true, "Music volume set to " + std::to_string(vol)};
        } catch (...) {
            return {false, "Invalid volume"};
        }
    }
    return {false, "Unknown music action"};
}

CommandResult CommandParser::handleReset(const std::vector<std::string>& args) {
    playState.loadLevel(playState.currentLevel);
    return {true, "Level reset"};
}

CommandResult CommandParser::handleLives(const std::vector<std::string>& args) {
    if (args.size() < 2) return {false, "Usage: lives <n>"};
    try {
        int n = std::stoi(args[1]);
        if (n < 1 || n > 99) return {false, "Lives must be between 1 and 99"};
        playState.lives = n;
        playState.hud.setLives(playState.lives);
        return {true, "Lives set to " + std::to_string(n)};
    } catch (...) {
        return {false, "Invalid number"};
    }
}

CommandResult CommandParser::handleDestroyer(const std::vector<std::string>& args) {
    bool on = true;
    if (args.size() > 1 && (args[1] == "off" || args[1] == "false" || args[1] == "0")) {
        on = false;
    }
    playState.m_destroyerMode = on;
    return {true, "Destroyer mode " + std::string(on ? "enabled" : "disabled")};
}

CommandResult CommandParser::handleHelp() {
    std::string help = 
        "Commands:\n"
        "fly [off] - Fly without gravity\n"
        "god [off] - Invincibility + flight\n"
        "tp spawn - Teleport to start\n"
        "summon <goomba|koopa|paratroopa> [count]\n"
        "form <0|1|2> - 0=Small, 1=Super, 2=Fire\n"
        "music <play path|stop|volume 0-100>\n"
        "reset - Restart current level\n"
        "lives <1-99>\n"
        "destroyer [off] - Touch kills mobs\n"
        "help - Show this list";
    return {true, help};
}

} // namespace Systems
