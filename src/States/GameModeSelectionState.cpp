#include "States/GameModeSelectionState.hpp"
#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <iostream>

GameModeSelectionState::GameModeSelectionState(GameStateManager& gsm, Systems::AssetManager& assets)
    : State(gsm, assets),
      bgSprite(assets.getTexture("MenuBackground")),
      headerText(assets.getFont("MarioFont")),
      normalText(assets.getFont("MarioFont")),
      nightfallText(assets.getFont("MarioFont")),
      infernoText(assets.getFont("MarioFont")),
      normalDesc(assets.getFont("MarioFont")),
      nightfallDesc(assets.getFont("MarioFont")),
      infernoDesc(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")),
      m_hellfireSprite(assets.getTexture("Hellfire")) {}

void GameModeSelectionState::init() {
    std::cout << "[Core Engine] GameModeSelectionState Initialized.\n";

    // Background
    const sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    const float bgScale = std::max(Config::kViewWidth / bgSize.x,
                                   Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    darkOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 175));

    // Header
    headerText.setString("SELECT GAME MODE");
    headerText.setCharacterSize(38);
    headerText.setFillColor(sf::Color::Yellow);
    headerText.setOutlineColor(sf::Color::Black);
    headerText.setOutlineThickness(3.f);
    const sf::FloatRect headerBounds = headerText.getLocalBounds();
    headerText.setOrigin({headerBounds.position.x + headerBounds.size.x / 2.f,
                          headerBounds.position.y + headerBounds.size.y / 2.f});
    headerText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.15f});

    // Options
    for (sf::Text* t : {&normalText, &nightfallText, &infernoText}) {
        t->setCharacterSize(28);
        t->setOutlineColor(sf::Color::Black);
        t->setOutlineThickness(2.f);
    }

    // Descriptions
    for (sf::Text* t : {&normalDesc, &nightfallDesc, &infernoDesc}) {
        t->setCharacterSize(16);
        t->setOutlineColor(sf::Color::Black);
        t->setOutlineThickness(2.f);
    }
    normalDesc.setString("Classic gameplay with full visibility");
    normalDesc.setFillColor(sf::Color(200, 200, 200));

    nightfallDesc.setString("Darkness surrounds you. Only a small light guides your way!");
    nightfallDesc.setFillColor(sf::Color(200, 200, 200));

    infernoDesc.setString("YOU HAVE ONLY ONE CHANCE TO LIVE! RUN FROM THE WALL OF DEATH!");
    infernoDesc.setFillColor(sf::Color(200, 200, 200));

    // Hint
    hintText.setString("UP/DOWN: SELECT  |  ENTER: CONFIRM  |  B: BACK");
    hintText.setCharacterSize(15);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    const sf::FloatRect hintBounds = hintText.getLocalBounds();
    hintText.setOrigin({hintBounds.position.x + hintBounds.size.x / 2.f,
                        hintBounds.position.y + hintBounds.size.y / 2.f});
    hintText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.88f});

    // Setup Inferno Mode RenderTexture
    if (!m_infernoRT.resize({static_cast<unsigned int>(Config::kViewWidth), 100u})) {
        std::cerr << "[Warning] Failed to resize m_infernoRT\n";
    }
    const auto& hellfireTex = assets.getTexture("Hellfire");
    m_hellfireSprite.setTexture(hellfireTex);
    
    // Scale the hellfire image to fit the text bounds nicely
    float scale = Config::kViewWidth / static_cast<float>(hellfireTex.getSize().x);
    // Alternatively, just make it a bit wider than the text. Text is about 200px wide.
    scale = std::max(scale, 300.f / hellfireTex.getSize().x);
    m_hellfireSprite.setScale({scale, scale});

    // Center the origin
    m_hellfireSprite.setOrigin({hellfireTex.getSize().x / 2.f, hellfireTex.getSize().y / 2.f});
    // Position at the center of the render texture, shifted up to show the lower part of the fire
    m_hellfireSprite.setPosition({Config::kViewWidth / 2.f, 20.f});
}

void GameModeSelectionState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;

    const auto key = keyPressed->scancode;
    if (key == sf::Keyboard::Scancode::Up || key == sf::Keyboard::Scancode::W) {
        selectedIndex = (selectedIndex == 0) ? 2 : selectedIndex - 1;
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
    } else if (key == sf::Keyboard::Scancode::Down || key == sf::Keyboard::Scancode::S) {
        selectedIndex = (selectedIndex == 2) ? 0 : selectedIndex + 1;
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
    } else if (key == sf::Keyboard::Scancode::Enter
               || key == sf::Keyboard::Scancode::Space) {
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
        GameMode mode = static_cast<GameMode>(selectedIndex);
        std::cout << "[Core Engine] Game mode selected: " << selectedIndex << "\n";
        gsm.changeState(std::make_unique<CharacterSelectionState>(gsm, assets, mode));
    } else if (key == sf::Keyboard::Scancode::B
               || key == sf::Keyboard::Scancode::Escape) {
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
        gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
    }
}

void GameModeSelectionState::update(sf::Time) {
    sf::Color dimColor(80, 80, 80, 120);
    sf::Color dimOutline(0, 0, 0, 120);
    
    // Update option labels
    normalText.setString(selectedIndex == 0 ? "> NORMAL MODE" : "  NORMAL MODE");
    normalText.setFillColor(selectedIndex == 0 ? sf::Color::Yellow : dimColor);
    normalText.setOutlineColor(selectedIndex == 0 ? sf::Color::Black : dimOutline);
    const sf::FloatRect nBounds = normalText.getLocalBounds();
    normalText.setOrigin({nBounds.position.x + nBounds.size.x / 2.f,
                          nBounds.position.y + nBounds.size.y / 2.f});
    normalText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.38f});

    // Normal description
    normalDesc.setFillColor(selectedIndex == 0 ? sf::Color(220, 220, 220) : dimColor);
    normalDesc.setOutlineColor(selectedIndex == 0 ? sf::Color::Black : dimOutline);
    const sf::FloatRect ndBounds = normalDesc.getLocalBounds();
    normalDesc.setOrigin({ndBounds.position.x + ndBounds.size.x / 2.f,
                          ndBounds.position.y + ndBounds.size.y / 2.f});
    normalDesc.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.44f});

    nightfallText.setString(selectedIndex == 1 ? "> NIGHTFALL MODE" : "  NIGHTFALL MODE");
    nightfallText.setFillColor(selectedIndex == 1 ? sf::Color(180, 32, 255) : dimColor);
    const sf::FloatRect nfBounds = nightfallText.getLocalBounds();
    nightfallText.setOrigin({nfBounds.position.x + nfBounds.size.x / 2.f,
                             nfBounds.position.y + nfBounds.size.y / 2.f});
    nightfallText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.55f});

    // Nightfall description
    nightfallDesc.setFillColor(selectedIndex == 1 ? sf::Color(220, 220, 220) : dimColor);
    nightfallDesc.setOutlineColor(selectedIndex == 1 ? sf::Color::Black : dimOutline);
    const sf::FloatRect nfdBounds = nightfallDesc.getLocalBounds();
    nightfallDesc.setOrigin({nfdBounds.position.x + nfdBounds.size.x / 2.f,
                             nfdBounds.position.y + nfdBounds.size.y / 2.f});
    nightfallDesc.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.61f});

    infernoText.setString(selectedIndex == 2 ? "> INFERNO MODE" : "  INFERNO MODE");
    // We don't set infernoText color here because it's overridden in render() for the mask
    const sf::FloatRect infBounds = infernoText.getLocalBounds();
    infernoText.setOrigin({infBounds.position.x + infBounds.size.x / 2.f,
                           infBounds.position.y + infBounds.size.y / 2.f});
    infernoText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.72f});

    infernoDesc.setFillColor(selectedIndex == 2 ? sf::Color(220, 220, 220) : dimColor);
    infernoDesc.setOutlineColor(selectedIndex == 2 ? sf::Color::Black : dimOutline);
    const sf::FloatRect infdBounds = infernoDesc.getLocalBounds();
    infernoDesc.setOrigin({infdBounds.position.x + infdBounds.size.x / 2.f,
                           infdBounds.position.y + infdBounds.size.y / 2.f});
    infernoDesc.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.78f});
}

void GameModeSelectionState::render(sf::RenderWindow& window) {
    window.draw(bgSprite);
    window.draw(darkOverlay);
    window.draw(headerText);
    window.draw(normalText);
    window.draw(normalDesc);
    // Draw nightfallText character by character to alternate colors
    sf::String nfStr = nightfallText.getString();
    sf::Text nfCharText = nightfallText;
    nfCharText.setOrigin({0.f, 0.f}); // Reset origin so findCharacterPos works as top-left

    // Disable deprecation warning for findCharacterPos locally
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    sf::Color dimColor(80, 80, 80, 120);
    for (std::size_t i = 0; i < nfStr.getSize(); ++i) {
        nfCharText.setString(nfStr.substring(i, 1));
        nfCharText.setPosition(nightfallText.findCharacterPos(i));
        
        if (selectedIndex == 1) {
            nfCharText.setFillColor(i % 2 == 0 ? sf::Color(180, 32, 255) : sf::Color::Black);
            nfCharText.setOutlineThickness(0.f);
        } else {
            nfCharText.setFillColor(dimColor);
            nfCharText.setOutlineThickness(0.f);
        }
        
        window.draw(nfCharText);
    }
#pragma GCC diagnostic pop
    window.draw(nightfallDesc);
    
    // Render textured inferno text
    m_infernoRT.clear(sf::Color::Transparent);
    
    sf::Color oldColor = infernoText.getFillColor();
    // Use White if selected so texture is bright, dimColor if not to make it darker and faded
    infernoText.setFillColor(selectedIndex == 2 ? sf::Color::White : dimColor);
    
    sf::Vector2f oldPos = infernoText.getPosition();
    infernoText.setPosition({Config::kViewWidth / 2.f, 50.f});
    
    m_infernoRT.draw(infernoText);
    m_infernoRT.draw(m_hellfireSprite, sf::BlendMultiply);
    m_infernoRT.display();
    
    infernoText.setFillColor(oldColor);
    infernoText.setPosition(oldPos);
    
    sf::Sprite finalInferno(m_infernoRT.getTexture());
    finalInferno.setOrigin({Config::kViewWidth / 2.f, 50.f});
    finalInferno.setPosition(oldPos);
    window.draw(finalInferno);

    window.draw(infernoDesc);
    window.draw(hintText);
}
