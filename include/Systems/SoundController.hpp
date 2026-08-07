#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <iostream>
#include <list>

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
        std::list<sf::Sound> _activeSounds; ///< Active sound effects
        
        float _musicVolume = 60.0f; ///< Current music volume (0 - 100)
        float _soundVolume = 60.0f; ///< Current SFX volume (0 - 100)
        
        bool _musicMuted{false};    ///< Track music mute state
        bool _soundMuted{false};    ///< Track sound effect mute state

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

        std::string _currentMusicPath; ///< Currently loaded music file path

        /**
         * @brief Plays background music from a file. Keeps playing across state changes.
         * @param filePath Path to the audio file (e.g. "assets/audio/Theme.mp3").
         * @param loop Set to true to loop continuously.
         */
        bool playMusic(const std::string& filePath, bool loop = true) {
            if (_currentMusicPath == filePath) {
                if (_bgMusic.getStatus() == sf::Music::Status::Paused) {
                    _bgMusic.play();
                }
                return true;
            }

            if (!_bgMusic.openFromFile(filePath)) {
                std::cerr << "[SoundController] Error: Could not load music file: " << filePath << "\n";
                return false;
            }
            _currentMusicPath = filePath;
            _bgMusic.setLooping(loop);
            _bgMusic.setVolume(_musicMuted ? 0.f : _musicVolume);
            _bgMusic.play();
            std::cout << "[SoundController] Playing music: " << filePath << "\n";
            return true;
        }

        /**
         * @brief Stops currently playing background music.
         */
        void stopMusic() {
            _bgMusic.stop();
            _currentMusicPath.clear();
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
            if (!_musicMuted) {
                _bgMusic.setVolume(_musicVolume);
            }
        }

        /**
         * @brief Gets current music volume level.
         */
        float getMusicVolume() const {
            return _musicVolume;
        }

        /**
         * @brief Sets global sound effect volume (0.0f - 100.0f).
         */
        void setSoundVolume(float volume) {
            _soundVolume = volume;
            for (auto& sound : _activeSounds) {
                sound.setVolume(_soundVolume);
            }
        }

        /**
         * @brief Gets current sound effect volume level.
         */
        float getSoundVolume() const {
            return _soundVolume;
        }

        bool isMusicMuted() const { return _musicMuted; }
        bool isSoundMuted() const { return _soundMuted; }

        void setMusicMuted(bool muted) {
            _musicMuted = muted;
            if (_musicMuted) {
                _bgMusic.setVolume(0.f);
            } else {
                _bgMusic.setVolume(_musicVolume);
            }
            std::cout << "[SoundController] Music Muted: " << (_musicMuted ? "YES" : "NO") << "\n";
        }

        void setSoundMuted(bool muted) {
            _soundMuted = muted;
            std::cout << "[SoundController] Sound Effects Muted: " << (_soundMuted ? "YES" : "NO") << "\n";
        }

        void toggleMusicMuted() {
            setMusicMuted(!_musicMuted);
        }

        void toggleSoundMuted() {
            setSoundMuted(!_soundMuted);
        }

        /**
         * @brief Plays a sound effect.
         * @param buffer The sound buffer to play.
         */
        void playSound(const sf::SoundBuffer& buffer) {
            if (_soundMuted) return;
            
            _activeSounds.emplace_back(buffer);
            _activeSounds.back().setVolume(_soundVolume);
            _activeSounds.back().play();
        }

        /**
         * @brief Cleans up finished sounds to prevent memory/resource leaks.
         * Call this periodically in the game loop.
         */
        void update() {
            _activeSounds.remove_if([](const sf::Sound& sound) {
                return sound.getStatus() == sf::Sound::Status::Stopped;
            });
        }
    };
}