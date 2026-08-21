#pragma once
#include <functional>
#include <vector>
#include <map>
#include <SFML/System/Vector2.hpp>

namespace Core {

/**
 * @enum EventType
 * @brief Types of events that can be broadcasted globally.
 */
enum class EventType {
    CoinCollected,
    MushroomCollected,
    FireFlowerCollected,
    StarCollected,
    EnemyStomped,
    EnemyDefeated,
    PlayerJumped,
    PlayerDamaged,
    PlayerDied,
    GameOver,
    OneMoreLife,
    BlockHit,
    BrickBroken,
    FireballShot,
    BombExploded,
    LevelCompleted
};

/**
 * @struct Event
 * @brief Data structure holding event information and payload.
 */
struct Event {
    EventType type;
    int scoreGain{0};
    int coinGain{0};
    sf::Vector2f position{0.f, 0.f};
    int extraData{0};

    Event(EventType t) : type(t) {}
    Event(EventType t, int score, int coins = 0, sf::Vector2f pos = {0.f, 0.f}, int extra = 0)
        : type(t), scoreGain(score), coinGain(coins), position(pos), extraData(extra) {}
};

/**
 * @class EventSystem
 * @brief Singleton Event Bus implementing the Observer Pattern.
 */
class EventSystem {
public:
    using EventListener = std::function<void(const Event&)>;

private:
    std::map<EventType, std::vector<EventListener>> listeners;

    EventSystem() = default;
    ~EventSystem() = default;

    // Prevent copying
    EventSystem(const EventSystem&) = delete;
    EventSystem& operator=(const EventSystem&) = delete;

public:
    /**
     * @brief Gets the singleton instance of the EventSystem.
     */
    static EventSystem& getInstance() {
        static EventSystem instance;
        return instance;
    }

    /**
     * @brief Subscribes a listener to a specific event type.
     * @param type The type of event to listen for.
     * @param callback The function to call when the event occurs.
     */
    void subscribe(EventType type, EventListener callback) {
        listeners[type].push_back(callback);
    }

    /**
     * @brief Broadcasts an event to all subscribed listeners.
     * @param event The event data to broadcast.
     */
    void broadcast(const Event& event) {
        auto it = listeners.find(event.type);
        if (it != listeners.end()) {
            for (const auto& listener : it->second) {
                listener(event);
            }
        }
    }
    
    /**
     * @brief Clears all listeners. Useful when switching states to prevent dangling references.
     */
    void clearAllListeners() {
        listeners.clear();
    }
};

} // namespace Core
