#include "States/GameModeSelectionState.hpp"

#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/DuelState.hpp"
#include "States/GameStateManager.hpp"
#include "States/IntroMenuState.hpp"
#include "Systems/SoundController.hpp"

#include <algorithm>
#include <iostream>
#include <string>

GameModeSelectionState::GameModeSelectionState(
    GameStateManager& gsm,
    Systems::AssetManager& assets,
    GameMode initiallySelected
)
    : State(gsm, assets),
      selectedOption(SelectionOption::Normal),
      bgSprite(assets.getTexture("MenuBackground")),
      m_hellfireSprite(assets.getTexture("Hellfire")),
      headerText(assets.getFont("MarioFont")),
      normalText(assets.getFont("MarioFont")),
      nightfallText(assets.getFont("MarioFont")),
      infernoText(assets.getFont("MarioFont")),
      duelText(assets.getFont("MarioFont")),
      normalDesc(assets.getFont("MarioFont")),
      nightfallDesc(assets.getFont("MarioFont")),
      infernoDesc(assets.getFont("MarioFont")),
      duelDesc(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")) {
    switch (initiallySelected) {
        case GameMode::Normal:
            selectedOption = SelectionOption::Normal;
            break;

        case GameMode::Nightfall:
            selectedOption = SelectionOption::Nightfall;
            break;

        case GameMode::Inferno:
            selectedOption = SelectionOption::Inferno;
            break;
    }
}

void GameModeSelectionState::init() {
    std::cout << "[Core Engine] GameModeSelectionState Initialized.\n";

    const sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    const float bgScale = std::max(
        Config::kViewWidth / bgSize.x,
        Config::kViewHeight / bgSize.y
    );

    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({
        (Config::kViewWidth - bgSize.x * bgScale) / 2.f,
        (Config::kViewHeight - bgSize.y * bgScale) / 2.f
    });

    darkOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 175));

    headerText.setString("SELECT GAME MODE");
    headerText.setCharacterSize(38);
    headerText.setFillColor(sf::Color::Yellow);
    headerText.setOutlineColor(sf::Color::Black);
    headerText.setOutlineThickness(3.f);

    const sf::FloatRect headerBounds = headerText.getLocalBounds();
    headerText.setOrigin({
        headerBounds.position.x + headerBounds.size.x / 2.f,
        headerBounds.position.y + headerBounds.size.y / 2.f
    });
    headerText.setPosition({
        Config::kViewWidth / 2.f,
        Config::kViewHeight * 0.10f
    });

    for (sf::Text* text : {
             &normalText,
             &nightfallText,
             &infernoText,
             &duelText
         }) {
        text->setCharacterSize(26);
        text->setOutlineColor(sf::Color::Black);
        text->setOutlineThickness(2.f);
    }

    for (sf::Text* text : {
             &normalDesc,
             &nightfallDesc,
             &infernoDesc,
             &duelDesc
         }) {
        text->setCharacterSize(15);
        text->setFillColor(sf::Color(200, 200, 200));
        text->setOutlineColor(sf::Color::Black);
        text->setOutlineThickness(2.f);
    }

    normalDesc.setString("Classic gameplay with full visibility");
    nightfallDesc.setString(
        "Darkness surrounds you. Only a small light guides your way!"
    );
    infernoDesc.setString(
        "ONE CHANCE TO LIVE! RUN FROM THE WALL OF DEATH!"
    );
    duelDesc.setString("Local Mario vs Luigi battle");

    hintText.setString(
        "UP/DOWN OR 1/2/3/4: SELECT | "
        "ENTER/SPACE: CONFIRM | B/ESC: BACK"
    );
    hintText.setCharacterSize(15);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);

    const sf::FloatRect hintBounds = hintText.getLocalBounds();
    hintText.setOrigin({
        hintBounds.position.x + hintBounds.size.x / 2.f,
        hintBounds.position.y + hintBounds.size.y / 2.f
    });
    hintText.setPosition({
        Config::kViewWidth / 2.f,
        Config::kViewHeight * 0.90f
    });

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

void GameModeSelectionState::selectPreviousOption() {
    switch (selectedOption) {
        case SelectionOption::Normal:
            chooseOption(SelectionOption::Duel);
            break;

        case SelectionOption::Nightfall:
            chooseOption(SelectionOption::Normal);
            break;

        case SelectionOption::Inferno:
            chooseOption(SelectionOption::Nightfall);
            break;

        case SelectionOption::Duel:
            chooseOption(SelectionOption::Inferno);
            break;
    }
}

void GameModeSelectionState::selectNextOption() {
    switch (selectedOption) {
        case SelectionOption::Normal:
            chooseOption(SelectionOption::Nightfall);
            break;

        case SelectionOption::Nightfall:
            chooseOption(SelectionOption::Inferno);
            break;

        case SelectionOption::Inferno:
            chooseOption(SelectionOption::Duel);
            break;

        case SelectionOption::Duel:
            chooseOption(SelectionOption::Normal);
            break;
    }
}

void GameModeSelectionState::chooseOption(SelectionOption option) {
    if (option == selectedOption) {
        return;
    }

    selectedOption = option;

    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound")
    );
}

void GameModeSelectionState::confirmSelection() {
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound")
    );

    switch (selectedOption) {
        case SelectionOption::Normal:
            std::cout << "[Core Engine] Game mode selected: NORMAL\n";
            gsm.changeState(
                std::make_unique<CharacterSelectionState>(
                    gsm,
                    assets,
                    GameMode::Normal
                )
            );
            break;

        case SelectionOption::Nightfall:
            std::cout << "[Core Engine] Game mode selected: NIGHTFALL\n";
            gsm.changeState(
                std::make_unique<CharacterSelectionState>(
                    gsm,
                    assets,
                    GameMode::Nightfall
                )
            );
            break;

        case SelectionOption::Inferno:
            std::cout << "[Core Engine] Game mode selected: INFERNO\n";
            gsm.changeState(
                std::make_unique<CharacterSelectionState>(
                    gsm,
                    assets,
                    GameMode::Inferno
                )
            );
            break;

        case SelectionOption::Duel:
            std::cout << "[Core Engine] Game mode selected: DUEL\n";
            gsm.pushState(std::make_unique<DuelState>(gsm, assets));
            break;
    }
}

void GameModeSelectionState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed =
            event.getIf<sf::Event::KeyPressed>()) {
        const auto key = keyPressed->scancode;

        if (key == sf::Keyboard::Scancode::Up
            || key == sf::Keyboard::Scancode::W) {
            selectPreviousOption();
        } else if (
            key == sf::Keyboard::Scancode::Down
            || key == sf::Keyboard::Scancode::S
        ) {
            selectNextOption();
        } else if (
            key == sf::Keyboard::Scancode::Num1
            || key == sf::Keyboard::Scancode::Numpad1
        ) {
            chooseOption(SelectionOption::Normal);
        } else if (
            key == sf::Keyboard::Scancode::Num2
            || key == sf::Keyboard::Scancode::Numpad2
        ) {
            chooseOption(SelectionOption::Nightfall);
        } else if (
            key == sf::Keyboard::Scancode::Num3
            || key == sf::Keyboard::Scancode::Numpad3
        ) {
            chooseOption(SelectionOption::Inferno);
        } else if (
            key == sf::Keyboard::Scancode::Num4
            || key == sf::Keyboard::Scancode::Numpad4
        ) {
            chooseOption(SelectionOption::Duel);
        } else if (
            key == sf::Keyboard::Scancode::Enter
            || key == sf::Keyboard::Scancode::Space
        ) {
            confirmSelection();
        } else if (
            key == sf::Keyboard::Scancode::B
            || key == sf::Keyboard::Scancode::Escape
        ) {
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("SelectSound")
            );

            gsm.changeState(
                std::make_unique<IntroMenuState>(gsm, assets)
            );
        }

        return;
    }

    const auto* mousePressed =
        event.getIf<sf::Event::MouseButtonPressed>();

    if (!mousePressed
        || mousePressed->button != sf::Mouse::Button::Left) {
        return;
    }

    sf::Vector2f mousePosition(
        static_cast<float>(mousePressed->position.x),
        static_cast<float>(mousePressed->position.y)
    );

    if (gsm.getWindow() != nullptr) {
        mousePosition = gsm.getWindow()->mapPixelToCoords(
            mousePressed->position
        );
    }

    if (normalText.getGlobalBounds().contains(mousePosition)) {
        chooseOption(SelectionOption::Normal);
    } else if (
        nightfallText.getGlobalBounds().contains(mousePosition)
    ) {
        chooseOption(SelectionOption::Nightfall);
    } else if (
        infernoText.getGlobalBounds().contains(mousePosition)
    ) {
        chooseOption(SelectionOption::Inferno);
    } else if (
        duelText.getGlobalBounds().contains(mousePosition)
    ) {
        chooseOption(SelectionOption::Duel);
    }
}

void GameModeSelectionState::update(sf::Time) {
    const sf::Color dimColor(80, 80, 80, 120);
    const sf::Color dimOutline(0, 0, 0, 120);

    auto updateOption = [this](
        sf::Text& optionText,
        sf::Text& descriptionText,
        SelectionOption option,
        const std::string& label,
        const sf::Color& selectedColor,
        float optionY,
        float descriptionY,
        const sf::Color& dimColor,
        const sf::Color& dimOutline
    ) {
        const bool selected = selectedOption == option;

        optionText.setString(
            (selected ? "> " : "  ") + label
        );
        optionText.setFillColor(
            selected
                ? selectedColor
                : dimColor
        );
        optionText.setOutlineColor(
            selected ? sf::Color::Black : dimOutline
        );

        const sf::FloatRect optionBounds =
            optionText.getLocalBounds();

        optionText.setOrigin({
            optionBounds.position.x + optionBounds.size.x / 2.f,
            optionBounds.position.y + optionBounds.size.y / 2.f
        });
        optionText.setPosition({
            Config::kViewWidth / 2.f,
            Config::kViewHeight * optionY
        });

        const sf::FloatRect descriptionBounds =
            descriptionText.getLocalBounds();

        descriptionText.setOrigin({
            descriptionBounds.position.x
                + descriptionBounds.size.x / 2.f,
            descriptionBounds.position.y
                + descriptionBounds.size.y / 2.f
        });
        descriptionText.setPosition({
            Config::kViewWidth / 2.f,
            Config::kViewHeight * descriptionY
        });
        descriptionText.setFillColor(
            selected ? sf::Color(220, 220, 220) : dimColor
        );
        descriptionText.setOutlineColor(
            selected ? sf::Color::Black : dimOutline
        );
    };

    updateOption(
        normalText,
        normalDesc,
        SelectionOption::Normal,
        "NORMAL MODE",
        sf::Color::Yellow,
        0.22f,
        0.27f,
        dimColor,
        dimOutline
    );

    updateOption(
        nightfallText,
        nightfallDesc,
        SelectionOption::Nightfall,
        "NIGHTFALL MODE",
        sf::Color(180, 32, 255),
        0.38f,
        0.43f,
        dimColor,
        dimOutline
    );

    updateOption(
        infernoText,
        infernoDesc,
        SelectionOption::Inferno,
        "INFERNO MODE",
        sf::Color::White,
        0.54f,
        0.59f,
        dimColor,
        dimOutline
    );

    updateOption(
        duelText,
        duelDesc,
        SelectionOption::Duel,
        "DUEL MODE",
        sf::Color(80, 220, 120),
        0.70f,
        0.75f,
        dimColor,
        dimOutline
    );
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
        
        if (selectedOption == SelectionOption::Nightfall) {
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
    infernoText.setFillColor(
        selectedOption == SelectionOption::Inferno
            ? sf::Color::White
            : dimColor
    );
    
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

    window.draw(duelText);
    window.draw(duelDesc);

    window.draw(hintText);
}
