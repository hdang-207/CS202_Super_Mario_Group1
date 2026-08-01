#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <iostream>

namespace Systems {
    /**
     * @class SoundController
     * @brief Singleton class managing Background Music (BGM) and Sound Effects (SFX).
     * 
     * // DESIGN PATTERN: SINGLETON APPLIED FOR CENTRALIZED AUDIO MANAGEMENT
     */
    class SoundController {
    private:
        sf::Music _bgMusic;         ///< Unique background music stream
        float _musicVolume = 60.0f; ///< Current music volume (0 - 100)

        // Constructor & Destructor are private for Singleton
        SoundController() = default;
        ~SoundController() = default;

        // Prevent copying & assignment
        SoundController(const SoundController&) = delete;
        SoundController& operator=(const SoundController&) = delete;

    public:
        /**
         * @brief Access the unique global instance of SoundController.
         */
        static SoundController& getInstance() {
            static SoundController instance;
            return instance;
        }

        /**
         * @brief Plays background music from a file. Keeps playing across state changes.
         * @param filePath Path to the audio file (e.g. "assets/audio/Theme.mp3").
         * @param loop Set to true to loop continuously.
         */
        bool playMusic(const std::string& filePath, bool loop = true) {
            if (!_bgMusic.openFromFile(filePath)) {
                std::cerr << "[SoundController] Error: Could not load music file: " << filePath << "\n";
                return false;
            }
            _bgMusic.setLooping(loop);
            _bgMusic.setVolume(_musicVolume);
            _bgMusic.play();
            std::cout << "[SoundController] Playing music: " << filePath << "\n";
            return true;
        }

        /**
         * @brief Stops currently playing background music.
         */
        void stopMusic() {
            _bgMusic.stop();
        }

        /**
         * @brief Pauses currently playing background music.
         */
        void pauseMusic() {
            _bgMusic.pause();
        }

        /**
         * @brief Resumes paused music.
         */
        void resumeMusic() {
            _bgMusic.play();
        }

        /**
         * @brief Sets the global background music volume (0.0f - 100.0f).
         */
        void setMusicVolume(float volume) {
            _musicVolume = volume;
            _bgMusic.setVolume(_musicVolume);
        }

        /**
         * @brief Gets current volume level.
         */
        float getMusicVolume() const {
            return _musicVolume;
        }
    };
}