#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <utility> // Dùng cho std::move

namespace Systems {
    class AssetManager {
    private:
        std::map<std::string, sf::Texture> _textures;
        std::map<std::string, sf::Font> _fonts;
        std::map<std::string, sf::SoundBuffer> _soundBuffers;

    public:
        AssetManager() = default;
        ~AssetManager() = default;

        // 1. Texture trong SFML 3.1.0
        void loadTexture(const std::string& name, const std::string& path) {
            sf::Texture texture;
            if (texture.loadFromFile(path)) {
                _textures[name] = std::move(texture);
            }
        }
        const sf::Texture& getTexture(const std::string& name) const { 
            return _textures.at(name); 
        }

        // 2. Font trong SFML 3.1.0 (Chú ý: Dùng openFromFile)
        void loadFont(const std::string& name, const std::string& path) {
            sf::Font font;
            if (font.openFromFile(path)) { // <--- SFML 3 ĐỔI TÊN THÀNH openFromFile
                _fonts[name] = std::move(font);
            }
        }
        const sf::Font& getFont(const std::string& name) const { 
            return _fonts.at(name); 
        }

        // 3. SoundBuffer trong SFML 3.1.0
        void loadSoundBuffer(const std::string& name, const std::string& path) {
            sf::SoundBuffer buffer;
            if (buffer.loadFromFile(path)) {
                _soundBuffers[name] = std::move(buffer);
            }
        }
        const sf::SoundBuffer& getSoundBuffer(const std::string& name) const { 
            return _soundBuffers.at(name); 
        }
    };
}