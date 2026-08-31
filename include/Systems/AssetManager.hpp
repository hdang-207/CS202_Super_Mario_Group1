#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <map>
#include <iostream>
#include <string>
#include <utility>

namespace Systems {
    /**
     * @class AssetManager
     * @brief Centralized resource management system for textures, fonts, and sound buffers.
     */
    class AssetManager {
    private:
        std::map<std::string, sf::Texture> _textures;
        std::map<std::string, sf::Font> _fonts;
        std::map<std::string, sf::SoundBuffer> _soundBuffers;

    public:
        AssetManager() = default;
        ~AssetManager() = default;

        /**
         * @brief Loads a texture from file and stores it under a given identifier.
         * @param name Unique identifier name for the texture.
         * @param path File system path to the image file.
         */
        void loadTexture(const std::string& name, const std::string& path) {
            sf::Texture texture;
            if (texture.loadFromFile(path)) {
                _textures[name] = std::move(texture);
            } else {
                std::cerr << "[AssetManager] Failed to load texture '" << name
                          << "' from " << path << "\n";
            }
        }

        /**
         * @brief Retrieves a loaded texture by identifier.
         * @param name Identifier of the texture to retrieve.
         * @return Const reference to the sf::Texture object.
         */
        const sf::Texture& getTexture(const std::string& name) const { 
            auto it = _textures.find(name);
            if (it != _textures.end()) {
                return it->second;
            }
            std::cerr << "[AssetManager] Warning: Texture '" << name << "' not found!\n";
            if (!_textures.empty()) {
                return _textures.begin()->second;
            }
            static sf::Texture dummy;
            return dummy;
        }

        /**
         * @brief Loads a font from file and stores it under a given identifier.
         * @param name Unique identifier name for the font.
         * @param path File system path to the font file.
         */
        void loadFont(const std::string& name, const std::string& path) {
            sf::Font font;
            if (font.openFromFile(path)) {
                _fonts[name] = std::move(font);
            } else {
                std::cerr << "[AssetManager] Failed to load font '" << name
                          << "' from " << path << "\n";
            }
        }

        /**
         * @brief Retrieves a loaded font by identifier.
         * @param name Identifier of the font to retrieve.
         * @return Const reference to the sf::Font object.
         */
        const sf::Font& getFont(const std::string& name) const { 
            auto it = _fonts.find(name);
            if (it != _fonts.end()) {
                return it->second;
            }
            std::cerr << "[AssetManager] Warning: Font '" << name << "' not found!\n";
            if (!_fonts.empty()) {
                return _fonts.begin()->second;
            }
            static sf::Font dummy;
            return dummy;
        }

        /**
         * @brief Loads a sound buffer from file and stores it under a given identifier.
         * @param name Unique identifier name for the sound buffer.
         * @param path File system path to the audio file.
         */
        void loadSoundBuffer(const std::string& name, const std::string& path) {
            sf::SoundBuffer buffer;
            if (buffer.loadFromFile(path)) {
                _soundBuffers[name] = std::move(buffer);
            } else {
                std::cerr << "[AssetManager] Failed to load sound '" << name
                          << "' from " << path << "\n";
            }
        }

        /**
         * @brief Retrieves a loaded sound buffer by identifier.
         * @param name Identifier of the sound buffer to retrieve.
         * @return Const reference to the sf::SoundBuffer object.
         */
        const sf::SoundBuffer& getSoundBuffer(const std::string& name) const { 
            auto it = _soundBuffers.find(name);
            if (it != _soundBuffers.end()) {
                return it->second;
            }
            std::cerr << "[AssetManager] Warning: SoundBuffer '" << name << "' not found!\n";
            if (!_soundBuffers.empty()) {
                return _soundBuffers.begin()->second;
            }
            static sf::SoundBuffer dummy;
            return dummy;
        }
    };
}
