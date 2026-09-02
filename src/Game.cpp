#include "Game.hpp"
#include "Core/Config.hpp"
#include "States/IntroMenuState.hpp"
#include "Systems/HighDpi.hpp"
#include "Systems/ResourcePath.hpp"
#include <algorithm>
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
    gsm.setWindow(&window);
    // Keep the picture centred and unstretched at the starting window size.
    updateScreenView();

    // 1. Load UI fonts
    assets.loadFont("MarioFont", Systems::resourcePath("assets/fonts/MarioFont.otf"));

    // 2. Load audio buffers
    assets.loadSoundBuffer("ThemeSong", Systems::resourcePath("assets/audio/Theme.mp3"));
    assets.loadSoundBuffer("CoinSound", Systems::resourcePath("assets/audio/sound/coin_received.mp3"));
    assets.loadSoundBuffer("DieSound", Systems::resourcePath("assets/audio/sound/die.mp3"));
    assets.loadSoundBuffer("GameOverSound", Systems::resourcePath("assets/audio/sound/game_over.mp3"));
    assets.loadSoundBuffer("JumpSound", Systems::resourcePath("assets/audio/sound/jump.mp3"));
    assets.loadSoundBuffer("OneMoreLifeSound", Systems::resourcePath("assets/audio/sound/onemorelife.mp3"));
    assets.loadSoundBuffer("PowerUpSound", Systems::resourcePath("assets/audio/sound/powerup.mp3"));
    assets.loadSoundBuffer("VictorySound", Systems::resourcePath("assets/audio/sound/victory.mp3"));
    
    // New sounds
    assets.loadSoundBuffer("SelectSound", Systems::resourcePath("assets/audio/sound/selecting.wav"));
    assets.loadSoundBuffer("StompSound", Systems::resourcePath("assets/audio/sound/kill.wav"));
    assets.loadSoundBuffer("WalkingSound", Systems::resourcePath("assets/audio/sound/walking.wav"));
    assets.loadSoundBuffer("FireSound", Systems::resourcePath("assets/audio/sound/Fire.wav"));
    assets.loadSoundBuffer("ExplodeSound", Systems::resourcePath("assets/audio/sound/Explode.wav"));
    assets.loadSoundBuffer("DowngradeSound", Systems::resourcePath("assets/audio/sound/downgrade.wav"));
    assets.loadSoundBuffer("BrickBreak", Systems::resourcePath("assets/audio/sound/brick_break.wav"));
    assets.loadSoundBuffer("BrickCollision", Systems::resourcePath("assets/audio/sound/brick_collision.wav"));

    // Duel round-winner announcements
    assets.loadSoundBuffer("MarioWinsSound", Systems::resourcePath("assets/audio/sound/mario_wins.wav"));
    assets.loadSoundBuffer("LuigiWinsSound", Systems::resourcePath("assets/audio/sound/luigi_wins.wav"));
    assets.loadSoundBuffer("FightSound", Systems::resourcePath("assets/audio/sound/fight.wav"));
    
    assets.loadTexture("MenuBackground", Systems::resourcePath("assets/textures/Background.png"));
    assets.loadTexture("CoinIcon", Systems::resourcePath("assets/textures/coinHUD.png"));

    // Menu artwork: the character-select preview and the standing sprite the
    // respawn screen puts next to the lives count.
    assets.loadTexture("MarioPreview", Systems::resourcePath("assets/character/Mario_preview.png"));
    assets.loadTexture("LuigiPreview", Systems::resourcePath("assets/character/Luigi_preview.png"));
    assets.loadTexture("MarioIdle", Systems::resourcePath("assets/character/Mario_idle.png"));
    assets.loadTexture("LuigiIdle", Systems::resourcePath("assets/character/Luigi_idle.png"));

    // Gameplay sheets: nine land poses followed by the six NES swimming poses,
    // all in equal cells so PlayerAnimator can draw them at the project's
    // whole-number zoom. Every frame is cut from the NES source artwork by
    // `python3 tools/build_character_sheets.py`, which also writes the four
    // menu sprites above.
    assets.loadTexture("MarioSmallSheet", Systems::resourcePath("assets/character/mario_small.png"));
    assets.loadTexture("MarioSuperSheet", Systems::resourcePath("assets/character/mario_super.png"));
    assets.loadTexture("MarioFireSheet", Systems::resourcePath("assets/character/mario_fire.png"));
    assets.loadTexture("LuigiSmallSheet", Systems::resourcePath("assets/character/luigi_small.png"));
    assets.loadTexture("LuigiSuperSheet", Systems::resourcePath("assets/character/luigi_super.png"));
    assets.loadTexture("LuigiFireSheet", Systems::resourcePath("assets/character/luigi_fire.png"));

    assets.loadTexture("MusicSymbol", Systems::resourcePath("assets/textures/music_symbol.png"));
    assets.loadTexture("SoundSymbol", Systems::resourcePath("assets/textures/sound_symbol.png"));

    // Items and projectiles
    assets.loadTexture("FireFlower", Systems::resourcePath("assets/textures/FireFlower.png"));
    assets.loadTexture("ManaOrb", Systems::resourcePath("assets/textures/duel_mana_orb.png"));
    assets.loadTexture("Bullet", Systems::resourcePath("assets/textures/Bullet.png"));
    assets.loadTexture("Explosion", Systems::resourcePath("assets/textures/explosion.png"));

    // Level artwork: one image per map character, no tile atlas involved.
    assets.loadTexture("GroundTile", Systems::resourcePath("assets/textures/ground.png"));
    assets.loadTexture("CloudBlock", Systems::resourcePath("assets/textures/cloud_block.png"));
    assets.loadTexture("GroundUndergroundTile",
                       Systems::resourcePath("assets/textures/ground_underground.png"));
    assets.loadTexture("UnderwaterTiles",
                       Systems::resourcePath("assets/textures/underwater_tiles.png"));
    assets.loadTexture("UnderwaterRock",
                       Systems::resourcePath("assets/textures/underwater_rock.png"));
    assets.loadTexture("CoralTall",
                       Systems::resourcePath("assets/textures/coral_tall.png"));
    assets.loadTexture("BrickTile", Systems::resourcePath("assets/textures/brick.png"));
    assets.loadTexture("BrickUndergroundTile",
                       Systems::resourcePath("assets/textures/brick_underground.png"));
    assets.loadTexture("HardBlockTile", Systems::resourcePath("assets/textures/hard_block.png"));
    assets.loadTexture("HardBlockUndergroundTile",
                       Systems::resourcePath("assets/textures/hard_block_underground.png"));
    assets.loadTexture("PipeTopLeft", Systems::resourcePath("assets/textures/pipe_top_left.png"));
    assets.loadTexture("PipeTopRight", Systems::resourcePath("assets/textures/pipe_top_right.png"));
    assets.loadTexture("PipeBodyLeft", Systems::resourcePath("assets/textures/pipe_body_left.png"));
    assets.loadTexture("PipeBodyRight", Systems::resourcePath("assets/textures/pipe_body_right.png"));

    // Frames are laid out left to right; TileMap cycles them so the blocks blink.
    assets.loadTexture("QuestionBlock", Systems::resourcePath("assets/textures/question_block.png"));
    assets.loadTexture("QuestionBlockUnderground",
                       Systems::resourcePath("assets/textures/question_block_underground.png"));
    assets.loadTexture("EmptyBlock", Systems::resourcePath("assets/textures/empty_block.png"));
    assets.loadTexture("Coin", Systems::resourcePath("assets/textures/coin.png"));
    assets.loadTexture("SuperMushroom", Systems::resourcePath("assets/textures/super_mushroom.png"));
    assets.loadTexture("OneUpMushroom", Systems::resourcePath("assets/textures/one_up_mushroom.png"));
    assets.loadTexture("Goomba", Systems::resourcePath("assets/textures/goomba.png"));
    assets.loadTexture("GoombaUnderground",
                       Systems::resourcePath("assets/textures/goomba_underground.png"));
    assets.loadTexture("BlueKoopaUnderground",
                       Systems::resourcePath("assets/textures/blue_koopa_underground.png"));
    assets.loadTexture("GreenKoopa", Systems::resourcePath("assets/textures/green_koopa.png"));
    assets.loadTexture("GreenParatroopa", Systems::resourcePath("assets/textures/green_paratroopa.png"));
    assets.loadTexture("GreenShell", Systems::resourcePath("assets/textures/green_shell.png"));
    assets.loadTexture("RedKoopa", Systems::resourcePath("assets/textures/red_koopa.png"));
    assets.loadTexture("RedParatroopa", Systems::resourcePath("assets/textures/red_paratroopa.png"));
    assets.loadTexture("RedShell", Systems::resourcePath("assets/textures/red_shell.png"));
    assets.loadTexture("BlueShell", Systems::resourcePath("assets/textures/blue_shell.png"));
    assets.loadTexture("Blooper", Systems::resourcePath("assets/textures/blooper.png"));
    assets.loadTexture("CheepCheep", Systems::resourcePath("assets/textures/cheep_cheep.png"));
    assets.loadTexture("FlyingCheepCheep",
                       Systems::resourcePath("assets/textures/flying_cheep_cheep.png"));
    assets.loadTexture("HammerBro", Systems::resourcePath("assets/textures/hammer_bro.png"));
    assets.loadTexture("Hammer", Systems::resourcePath("assets/textures/hammer.png"));

    // Scenery: whole objects rather than tiles, so each one is several tiles big.
    assets.loadTexture("HillBig", Systems::resourcePath("assets/textures/hill_big.png"));
    assets.loadTexture("HillSmall", Systems::resourcePath("assets/textures/hill_small.png"));
    assets.loadTexture("BushBig", Systems::resourcePath("assets/textures/bush_big.png"));
    assets.loadTexture("BushSmall", Systems::resourcePath("assets/textures/bush_small.png"));
    assets.loadTexture("BushMedium", Systems::resourcePath("assets/textures/bush_medium.png"));
    assets.loadTexture("CloudBig", Systems::resourcePath("assets/textures/cloud_big.png"));
    assets.loadTexture("CloudSmall", Systems::resourcePath("assets/textures/cloud_small.png"));
    assets.loadTexture("CloudWide", Systems::resourcePath("assets/textures/cloud_wide.png"));
    assets.loadTexture("Island", Systems::resourcePath("assets/textures/island.png"));
    assets.loadTexture("IslandTopLeft", Systems::resourcePath("assets/textures/island_top_left.png"));
    assets.loadTexture("IslandTopMiddle", Systems::resourcePath("assets/textures/island_top_middle.png"));
    assets.loadTexture("IslandTopRight", Systems::resourcePath("assets/textures/island_top_right.png"));
    assets.loadTexture("IslandTrunk", Systems::resourcePath("assets/textures/island_trunk.png"));
    assets.loadTexture("MovingPlatform", Systems::resourcePath("assets/textures/moving_platform.png"));
    assets.loadTexture("CoinHeavenLift", Systems::resourcePath("assets/textures/coin_heaven_lift.png"));
    assets.loadTexture("CastleWorld1_3", Systems::resourcePath("assets/textures/castle_world1-3.png"));
    assets.loadTexture("CastleWorld2_1", Systems::resourcePath("assets/textures/castle_world2-1.png"));
    assets.loadTexture("HorsetailTall", Systems::resourcePath("assets/textures/green_horsetail_tall.png"));
    assets.loadTexture("HorsetailShort", Systems::resourcePath("assets/textures/green_horsetail_short.png"));
    assets.loadTexture("FenceWorld2_1", Systems::resourcePath("assets/textures/fence_world2-1.png"));
    assets.loadTexture("FenceWorld2_1Group", Systems::resourcePath("assets/textures/fence_world2-1_group.png"));
    assets.loadTexture("PiranhaPlant", Systems::resourcePath("assets/textures/piranha_plant.png"));
    assets.loadTexture("SuperStar", Systems::resourcePath("assets/textures/super_star.png"));
    assets.loadTexture("VineTop", Systems::resourcePath("assets/textures/vine_top.png"));
    assets.loadTexture("TrampolineNormal", Systems::resourcePath("assets/textures/trampoline_normal.png"));
    assets.loadTexture("TrampolineCompressed", Systems::resourcePath("assets/textures/trampoline_compressed.png"));
    assets.loadTexture("TrampolineLaunch", Systems::resourcePath("assets/textures/trampoline_launch.png"));

    // World 2-3 is reconstructed from the supplied 237x15 NES guide. These
    // exact crops keep its orange bridge/castle palette separate from the
    // reusable overworld tiles used by the other stages.
    assets.loadTexture("World23Ground",
                       Systems::resourcePath("assets/textures/world23_ground.png"));
    assets.loadTexture("World23HardBlock",
                       Systems::resourcePath("assets/textures/world23_hard_block.png"));
    assets.loadTexture("World23BridgeRail",
                       Systems::resourcePath("assets/textures/world23_bridge_rail.png"));
    assets.loadTexture("World23BridgeDeck",
                       Systems::resourcePath("assets/textures/world23_bridge_deck.png"));
    assets.loadTexture("World23IslandLeft",
                       Systems::resourcePath("assets/textures/world23_island_left.png"));
    assets.loadTexture("World23IslandMiddle",
                       Systems::resourcePath("assets/textures/world23_island_middle.png"));
    assets.loadTexture("World23IslandRight",
                       Systems::resourcePath("assets/textures/world23_island_right.png"));
    assets.loadTexture("World23IslandTrunk",
                       Systems::resourcePath("assets/textures/world23_island_trunk.png"));
    assets.loadTexture("World23CloudBig",
                       Systems::resourcePath("assets/textures/world23_cloud_big.png"));
    assets.loadTexture("World23CloudSmall",
                       Systems::resourcePath("assets/textures/world23_cloud_small.png"));
    assets.loadTexture("World23StartCastle",
                       Systems::resourcePath("assets/textures/world23_start_castle.png"));
    assets.loadTexture("World23EndCastle",
                       Systems::resourcePath("assets/textures/world23_end_castle.png"));
    assets.loadTexture("World23GoalPole",
                       Systems::resourcePath("assets/textures/world23_goal_pole.png"));
    assets.loadTexture("World23Coin",
                       Systems::resourcePath("assets/textures/world23_coin.png"));
    assets.loadTexture("World23QuestionBlock",
                       Systems::resourcePath("assets/textures/world23_question_block.png"));
    assets.loadTexture("World23EmptyBlock",
                       Systems::resourcePath("assets/textures/world23_empty_block.png"));

    // World 3-1 is the night stage, rebuilt from its own 213x45 NES guide: it
    // shares almost no artwork with the daylight stages, and the last three
    // entries furnish the hidden coin room behind its warp pipe.
    assets.loadTexture("World31Ground",
                       Systems::resourcePath("assets/textures/world31_ground.png"));
    assets.loadTexture("World31Brick",
                       Systems::resourcePath("assets/textures/world31_brick.png"));
    assets.loadTexture("World31HardBlock",
                       Systems::resourcePath("assets/textures/world31_hard_block.png"));
    assets.loadTexture("World31QuestionBlock",
                       Systems::resourcePath("assets/textures/world31_question_block.png"));
    assets.loadTexture("World31EmptyBlock",
                       Systems::resourcePath("assets/textures/world31_empty_block.png"));
    assets.loadTexture("World31Coin",
                       Systems::resourcePath("assets/textures/world31_coin.png"));
    assets.loadTexture("World31PipeTopLeft",
                       Systems::resourcePath("assets/textures/world31_pipe_top_left.png"));
    assets.loadTexture("World31PipeTopRight",
                       Systems::resourcePath("assets/textures/world31_pipe_top_right.png"));
    assets.loadTexture("World31PipeBodyLeft",
                       Systems::resourcePath("assets/textures/world31_pipe_body_left.png"));
    assets.loadTexture("World31PipeBodyRight",
                       Systems::resourcePath("assets/textures/world31_pipe_body_right.png"));
    assets.loadTexture("World31BridgeDeck",
                       Systems::resourcePath("assets/textures/world31_bridge_deck.png"));
    assets.loadTexture("World31BridgeRail",
                       Systems::resourcePath("assets/textures/world31_bridge_rail.png"));
    assets.loadTexture("World31Water",
                       Systems::resourcePath("assets/textures/world31_water.png"));
    assets.loadTexture("World31WaterSurface",
                       Systems::resourcePath("assets/textures/world31_water_surface.png"));
    assets.loadTexture("World31CloudBlock",
                       Systems::resourcePath("assets/textures/world31_cloud_block.png"));
    assets.loadTexture("World31CoinHeavenLift",
                       Systems::resourcePath("assets/textures/world31_coin_heaven_lift.png"));
    assets.loadTexture("World31Vine",
                       Systems::resourcePath("assets/textures/world31_vine.png"));
    assets.loadTexture("World31TreeTall",
                       Systems::resourcePath("assets/textures/world31_tree_tall.png"));
    assets.loadTexture("World31TreeShort",
                       Systems::resourcePath("assets/textures/world31_tree_short.png"));
    assets.loadTexture("World31Fence",
                       Systems::resourcePath("assets/textures/world31_fence.png"));
    assets.loadTexture("World31CloudBig",
                       Systems::resourcePath("assets/textures/world31_cloud_big.png"));
    assets.loadTexture("World31CloudSmall",
                       Systems::resourcePath("assets/textures/world31_cloud_small.png"));
    assets.loadTexture("World31StartCastle",
                       Systems::resourcePath("assets/textures/world31_start_castle.png"));
    assets.loadTexture("World31EndCastle",
                       Systems::resourcePath("assets/textures/world31_end_castle.png"));
    assets.loadTexture("World31GoalPole",
                       Systems::resourcePath("assets/textures/world31_goal_pole.png"));
    assets.loadTexture("World31RoomGround",
                       Systems::resourcePath("assets/textures/world31_room_ground.png"));
    assets.loadTexture("World31RoomBrick",
                       Systems::resourcePath("assets/textures/world31_room_brick.png"));
    assets.loadTexture("World31RoomCoin",
                       Systems::resourcePath("assets/textures/world31_room_coin.png"));
    assets.loadTexture("World31RoomPipe",
                       Systems::resourcePath("assets/textures/world31_room_pipe.png"));

    // World 3-2 keeps the night sky but returns to overworld terrain and music.
    // Every entry below is an exact crop from its supplied 222x15 guide.
    assets.loadTexture("World32Ground",
                       Systems::resourcePath("assets/textures/world32_ground.png"));
    assets.loadTexture("World32HardBlock",
                       Systems::resourcePath("assets/textures/world32_hard_block.png"));
    assets.loadTexture("World32Brick",
                       Systems::resourcePath("assets/textures/world32_brick.png"));
    assets.loadTexture("World32QuestionBlock",
                       Systems::resourcePath("assets/textures/world32_question_block.png"));
    assets.loadTexture("World32EmptyBlock",
                       Systems::resourcePath("assets/textures/world32_empty_block.png"));
    assets.loadTexture("World32Coin",
                       Systems::resourcePath("assets/textures/world32_coin.png"));
    assets.loadTexture("World32PipeTopLeft",
                       Systems::resourcePath("assets/textures/world32_pipe_top_left.png"));
    assets.loadTexture("World32PipeTopRight",
                       Systems::resourcePath("assets/textures/world32_pipe_top_right.png"));
    assets.loadTexture("World32PipeBodyLeft",
                       Systems::resourcePath("assets/textures/world32_pipe_body_left.png"));
    assets.loadTexture("World32PipeBodyRight",
                       Systems::resourcePath("assets/textures/world32_pipe_body_right.png"));
    assets.loadTexture("World32TreeTall",
                       Systems::resourcePath("assets/textures/world32_tree_tall.png"));
    assets.loadTexture("World32TreeShort",
                       Systems::resourcePath("assets/textures/world32_tree_short.png"));
    assets.loadTexture("World32Fence",
                       Systems::resourcePath("assets/textures/world32_fence.png"));
    assets.loadTexture("World32FenceGroup",
                       Systems::resourcePath("assets/textures/world32_fence_group.png"));
    assets.loadTexture("World32FencePairOffset",
                       Systems::resourcePath("assets/textures/world32_fence_pair_offset.png"));
    assets.loadTexture("World32CloudBig",
                       Systems::resourcePath("assets/textures/world32_cloud_big.png"));
    assets.loadTexture("World32CloudSmall",
                       Systems::resourcePath("assets/textures/world32_cloud_small.png"));
    assets.loadTexture("World32StartCastle",
                       Systems::resourcePath("assets/textures/world32_start_castle.png"));
    assets.loadTexture("World32EndCastle",
                       Systems::resourcePath("assets/textures/world32_end_castle.png"));
    assets.loadTexture("World32GoalPole",
                       Systems::resourcePath("assets/textures/world32_goal_pole.png"));

    // World 3-3 spans a bottomless pit on green-capped pillars, swinging lifts
    // and two pulleys. Cropped from its own 163x15 guide; the coins and blocks
    // share World 3-2's night palette.
    assets.loadTexture("World33Ground",
                       Systems::resourcePath("assets/textures/world33_ground.png"));
    assets.loadTexture("World33Pillar",
                       Systems::resourcePath("assets/textures/world33_pillar.png"));
    assets.loadTexture("World33PlatformLeft",
                       Systems::resourcePath("assets/textures/world33_platform_left.png"));
    assets.loadTexture("World33PlatformMiddle",
                       Systems::resourcePath("assets/textures/world33_platform_middle.png"));
    assets.loadTexture("World33PlatformRight",
                       Systems::resourcePath("assets/textures/world33_platform_right.png"));
    assets.loadTexture("World33QuestionBlock",
                       Systems::resourcePath("assets/textures/world33_question_block.png"));
    assets.loadTexture("World33EmptyBlock",
                       Systems::resourcePath("assets/textures/world33_empty_block.png"));
    assets.loadTexture("World33Coin",
                       Systems::resourcePath("assets/textures/world33_coin.png"));
    assets.loadTexture("World33Lift",
                       Systems::resourcePath("assets/textures/world33_lift.png"));
    assets.loadTexture("World33PulleyWide",
                       Systems::resourcePath("assets/textures/world33_pulley_wide.png"));
    assets.loadTexture("World33PulleyShort",
                       Systems::resourcePath("assets/textures/world33_pulley_short.png"));
    assets.loadTexture("World33CloudBig",
                       Systems::resourcePath("assets/textures/world33_cloud_big.png"));
    assets.loadTexture("World33CloudSmall",
                       Systems::resourcePath("assets/textures/world33_cloud_small.png"));
    assets.loadTexture("World33StartCastle",
                       Systems::resourcePath("assets/textures/world33_start_castle.png"));
    assets.loadTexture("World33EndCastle",
                       Systems::resourcePath("assets/textures/world33_end_castle.png"));
    assets.loadTexture("World33GoalPole",
                       Systems::resourcePath("assets/textures/world33_goal_pole.png"));

    // Worlds 1-4, 2-4 and 3-4 are the castle courses. Their stone, lava,
    // Fire-Bar pivots, bridge room and boss artwork all come from the supplied
    // 160x15 NES maps and matching sprite sheets, and every piece below the
    // reveal sprites is shared between the three stages.
    assets.loadTexture("CastleWall",
                       Systems::resourcePath("assets/textures/castle_wall.png"));
    assets.loadTexture("CastleBrick",
                       Systems::resourcePath("assets/textures/castle_brick.png"));
    assets.loadTexture("CastleFireBarBlock",
                       Systems::resourcePath("assets/textures/castle_firebar_block.png"));
    assets.loadTexture("CastleLavaSurface",
                       Systems::resourcePath("assets/textures/castle_lava_surface.png"));
    assets.loadTexture("CastleLava",
                       Systems::resourcePath("assets/textures/castle_lava.png"));
    assets.loadTexture("CastleBridge",
                       Systems::resourcePath("assets/textures/castle_bridge.png"));
    assets.loadTexture("CastleLift",
                       Systems::resourcePath("assets/textures/castle_lift.png"));
    assets.loadTexture("CastleElevator",
                       Systems::resourcePath("assets/textures/castle_elevator.png"));
    assets.loadTexture("CastleAxe",
                       Systems::resourcePath("assets/textures/castle_axe.png"));
    assets.loadTexture("CastleQuestionBlock",
                       Systems::resourcePath("assets/textures/castle_question_block.png"));
    assets.loadTexture("CastleEmptyBlock",
                       Systems::resourcePath("assets/textures/castle_empty_block.png"));
    assets.loadTexture("CastleCoin",
                       Systems::resourcePath("assets/textures/castle_coin.png"));
    assets.loadTexture("CastleBowser",
                       Systems::resourcePath("assets/textures/castle_bowser.png"));
    assets.loadTexture("CastlePodoboo",
                       Systems::resourcePath("assets/textures/castle_podoboo.png"));
    // What each stage's fake Bowser turns back into once it is beaten.
    assets.loadTexture("CastleGoomba",
                       Systems::resourcePath("assets/textures/castle_goomba.png"));
    assets.loadTexture("CastleKoopa",
                       Systems::resourcePath("assets/textures/castle_koopa.png"));
    assets.loadTexture("CastleBuzzy",
                       Systems::resourcePath("assets/textures/castle_buzzy.png"));

    // End of the level.
    assets.loadTexture("Flagpole", Systems::resourcePath("assets/textures/Goal_Pole.png"));
    assets.loadTexture("Castle", Systems::resourcePath("assets/textures/Fortress.png"));
    assets.loadTexture("DoomFire1", Systems::resourcePath("assets/textures/doomfire1.png"));
    assets.loadTexture("DoomFire2", Systems::resourcePath("assets/textures/doomfire2.png"));
    assets.loadTexture("Hellfire", Systems::resourcePath("assets/textures/hellfire.jpg"));
    assets.loadTexture("Apocalypse", Systems::resourcePath("assets/textures/Apocalypse.png"));
    assets.loadTexture("Lock", Systems::resourcePath("assets/textures/lock.png"));
    assets.loadTexture("WarpPipeForked", Systems::resourcePath("assets/textures/warp_pipe_forked.png"));
    assets.loadTexture("NormalGameover", Systems::resourcePath("assets/textures/NormalGameover.png"));
    assets.loadTexture("NightfallGameover", Systems::resourcePath("assets/textures/NightfallGameover.png"));
    assets.loadTexture("InfernoGameover", Systems::resourcePath("assets/textures/InfernoGameover.png"));
    assets.loadTexture("ApocalypseGameover", Systems::resourcePath("assets/textures/ApocalypseGameover.png"));

    // Limit application framerate to prevent high CPU utilization
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
    
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
        dt = std::min(dt, sf::seconds(0.25f));
        timeSinceLastUpdate += dt;

        // 1. Process any pending state additions/removals before handling input or updates
        gsm.processStateChanges();

        // 2. Poll window and input events
        processEvents();

        if (!window.isOpen()) {
            break;
        }

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
    window.setKeyRepeatEnabled(false);
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
