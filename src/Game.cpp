#include "Game.hpp"
#include "States/IntroMenuState.hpp"
#include <iostream>

// Define target update frequency of 60 frames per second (approx 16.67ms per frame)
const sf::Time Game::TimePerFrame = sf::seconds(1.f / 60.f);

Game::Game() 
    : window(sf::VideoMode({1200u, 800u}), "Super Mario Bros - Group 1", sf::Style::Close | sf::Style::Titlebar)
{
    // 1. Load Font chữ cho UI
    assets.loadFont("MarioFont", "/Users/tranquochuy/Downloads/CS202_MarioGame/assets/fonts/MarioFont.otf");

    // 2. Load Âm thanh
    assets.loadSoundBuffer("ThemeSong", "/Users/tranquochuy/Downloads/CS202_MarioGame/assets/audio/Theme.mp3");

    assets.loadTexture("MenuBackground", "/Users/tranquochuy/Downloads/CS202_MarioGame/assets/textures/Background.png");

    assets.loadTexture("CloudBackground", "/Users/tranquochuy/Downloads/CS202_MarioGame/assets/textures/cloud.png");

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
        } else {
            // Forward other inputs to the GameStateManager
            gsm.handleInput(*event);
        }
    }
}

void Game::update(sf::Time dt) {
    gsm.update(dt);
}

void Game::render() {
    window.clear();               // Clear the window with default color (black)
    gsm.render(window);           // Render the current state
    window.display();             // Swap buffers to display drawn contents
}

