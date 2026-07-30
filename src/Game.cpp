#include "Game.hpp"
#include "Core/Config.hpp"
#include "States/IntroMenuState.hpp"
#include "Systems/HighDpi.hpp"
#include "Systems/ResourcePath.hpp"
#include <iostream>

namespace {
    const char* kWindowTitle = "Super Mario Bros - Group 1";
}

// Define target update frequency of 60 frames per second (approx 16.67ms per frame)
const sf::Time Game::TimePerFrame = sf::seconds(1.f / 60.f);

Game::Game()
    : window(sf::VideoMode({Config::kWindowWidth, Config::kWindowHeight}), kWindowTitle,
             sf::Style::Close | sf::Style::Titlebar | sf::Style::Resize),
      screenView(sf::FloatRect({0.f, 0.f}, {Config::kViewWidth, Config::kViewHeight}))
{
    // Keep the picture centred and unstretched at the starting window size.
    updateScreenView();

    // 1. Load Font chữ cho UI
    assets.loadFont("MarioFont", Systems::resourcePath("assets/fonts/MarioFont.otf"));

    // 2. Load Âm thanh
    assets.loadSoundBuffer("ThemeSong", Systems::resourcePath("assets/audio/Theme.mp3"));

    assets.loadTexture("MenuBackground", Systems::resourcePath("assets/textures/Background.png"));

    assets.loadTexture("LevelTilemap", Systems::resourcePath("assets/textures/tilemap.png"));

    // Own artwork for single tile types, drawn instead of the matching atlas tile.
    assets.loadTexture("GroundTile", Systems::resourcePath("assets/textures/ground.png"));

    // Limit application framerate to prevent high CPU utilization
    window.setFramerateLimit(60);
    
    // Initialize the stack with the intro menu state
    gsm.pushState(std::make_unique<IntroMenuState>(gsm, assets));
    
    // Flush the queue immediately to set up the initial state
    gsm.processStateChanges();
}

void Game::run() {
    sf::Clock clock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;

    // Core Game Loop
    while (window.isOpen()) {
        sf::Time dt = clock.restart();
        timeSinceLastUpdate += dt;

        // 1. Process any pending state additions/removals before handling input or updates
        gsm.processStateChanges();

        // 2. Poll window and input events
        processEvents();

        // If all states have been popped, exit the game
        if (gsm.isEmpty()) {
            window.close();
            break;
        }

        // 3. Update game logic using a fixed timestep.
        // If the frame rate drops, this ensures we catch up with multiple updates
        // without making the physical movement jump/teleport.
        while (timeSinceLastUpdate > TimePerFrame) {
            timeSinceLastUpdate -= TimePerFrame;
            update(TimePerFrame);
        }

        // 4. Render the current active state
        render();
    }
}

void Game::processEvents() {
    while (const auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        } else if (event->is<sf::Event::Resized>()) {
            // The player dragged the window edges: re-letterbox instead of stretching.
            updateScreenView();
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>();
                   keyPressed != nullptr && keyPressed->code == sf::Keyboard::Key::F11) {
            toggleFullscreen();
        } else {
            // Forward other inputs to the GameStateManager
            gsm.handleInput(*event);
        }
    }
}

void Game::updateScreenView() {
    // Re-asked every time because the answer changes when the window is dragged
    // onto a screen with a different pixel density.
    dpiScale = Systems::enableHighDpi(window.getNativeHandle());

    screenView.setSize({Config::kViewWidth, Config::kViewHeight});
    screenView.setCenter({Config::kViewWidth / 2.f, Config::kViewHeight / 2.f});
    screenView.setViewport(Config::letterboxViewport(window.getSize(), dpiScale));
}

void Game::toggleFullscreen() {
    fullscreen = !fullscreen;

    if (fullscreen) {
        window.create(sf::VideoMode::getDesktopMode(), kWindowTitle,
                      sf::Style::None, sf::State::Fullscreen);
    } else {
        window.create(sf::VideoMode({Config::kWindowWidth, Config::kWindowHeight}), kWindowTitle,
                      sf::Style::Close | sf::Style::Titlebar | sf::Style::Resize);
    }

    // Window settings do not survive a recreate, so restore them here.
    window.setFramerateLimit(60);
    updateScreenView();
}

void Game::update(sf::Time dt) {
    gsm.update(dt);
}

void Game::render() {
    window.clear();               // Black, which is also the colour of the letterbox bars
    window.setView(screenView);   // Every state draws into the same 1152x720 game area
    gsm.render(window);           // Render the current state
    window.display();             // Swap buffers to display drawn contents
}

