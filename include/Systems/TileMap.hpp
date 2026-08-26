#pragma once
#include "Systems/MapParser.hpp"
#include <SFML/Graphics.hpp>
#include <vector>

/**
 * @enum TileType
 * @brief Gameplay meaning of a tile, independent of which artwork it draws.
 */
enum class TileType {
    Empty,         ///< Sky / open air.
    Decoration,    ///< Drawn but never collides.
    Coin,          ///< Pickup: drawn and animated, but never blocks movement.
    Ground,        ///< Solid terrain.
    Brick,         ///< Breakable brick block (solid).
    CoinBrick,     ///< Brick that contains coins (solid).
    QuestionBlock, ///< Item block (solid).
    UsedBlock,     ///< Question block after its item has been released (solid).
    StairBlock,    ///< Solid staircase block.
    HiddenBlock,   ///< Invisible until struck from below, but solid.
    Pipe           ///< Solid pipe segment.
};

/**
 * @class TileMap
 * @brief Turns a parsed character grid into a drawable, collidable level.
 *
 * Artwork is supplied per map character through setTileTexture(), so the level
 * is drawn entirely from the project's own images - there is no tile atlas. All
 * tiles sharing a character are batched into one vertex array, which means one
 * draw call per character rather than per tile.
 *
 * A character may be given a horizontal strip of frames instead of a single
 * image; update() then cycles the whole batch through them, which is how the
 * question blocks blink.
 *
 * Physics code does not need to know anything about characters or artwork: it
 * asks for the solid tiles overlapping a bounding box and resolves against those.
 */
class TileMap : public sf::Drawable {
public:
    /// Size of one tile in the source artwork, in pixels.
    static constexpr int kSourceTileSize = 16;

    /**
     * @brief Sets the artwork one map character is drawn with.
     * @param symbol Character as it appears in the map file, e.g. '#', 'b' or '?'.
     * @param texture Image for that character. It is stretched over one whole
     *        tile, so a 16x16 png lines up one-to-one with the level grid.
     * @param frameCount Number of animation frames laid out left to right inside
     *        @p texture. Leave at 1 for a still image.
     * @param frameDuration How long each frame is shown.
     *
     * Call this before build() - the geometry is baked there. A character with
     * no artwork still collides, it is simply not drawn (that is how the hidden
     * block works).
     */
    void setTileTexture(char symbol, const sf::Texture& texture, int frameCount = 1,
                        sf::Time frameDuration = sf::seconds(0.12f));

    /**
     * @brief Registers scenery artwork - clouds, flagpole, castle - for a map character.
     * @param symbol Character as it appears in the map file, e.g. 'l' for a big cloud.
     * @param texture Picture of the whole object. Unlike a tile it may be several
     *        tiles wide and tall; it is anchored at the top-left corner of the cell
     *        holding @p symbol and keeps its own proportions.
     *
     * Scenery is drawn behind the level. Most of it is non-colliding; the map
     * builder gives the floating island top and underwater coral their intended
     * solid cells after placing the artwork.
     */
    void setDecorationTexture(char symbol, const sf::Texture& texture);

    /**
     * @brief Builds the level geometry from a parsed map.
     * @param parser Map whose character grid describes the level.
     * @param scale Zoom factor applied to every tile (use an integer to stay pixel-perfect).
     * @return True if the map contained at least one row.
     */
    bool build(const MapParser& parser, float scale);

    /// @brief Advances the animated tiles. Call once per frame.
    void update(sf::Time dt);

    /**
     * @brief Replaces one question or hidden item block with a used block.
     * @return True only on the first activation of an item block at this cell.
     */
    bool activateItemBlock(int col, int row);

    /**
     * @brief Breaks a brick at the specified location, turning it into empty space.
     * @return True if a brick or coin brick was successfully broken.
     */
    bool breakBrick(int col, int row);

    /**
     * @brief Hides a brick visually for bouncing animation, keeping it solid.
     * @return The original symbol of the brick, or \0 if not a brick.
     */
    char hideBrick(int col, int row);

    /**
     * @brief Restores a previously hidden brick back to the map.
     */
    void restoreBrick(int col, int row, char symbol);

    /// @brief Changes the TileType of the map cell at (col, row).
    void changeType(int col, int row, TileType newType);

    /// @brief True if the tile at this grid cell blocks movement (out of bounds is not solid).
    bool isSolid(int col, int row) const;

    /// @brief Gameplay type of a grid cell, or TileType::Empty when out of bounds.
    TileType typeAt(int col, int row) const;

    /// @brief Map character currently stored at a cell, or '.' when out of bounds.
    char symbolAt(int col, int row) const;

    /// @brief World-space rectangles of every solid tile overlapping @p box.
    std::vector<sf::FloatRect> solidTilesOverlapping(const sf::FloatRect& box) const;

    /// @brief True if any solid tile overlaps @p box.
    bool intersectsSolid(const sf::FloatRect& box) const;

    /// @brief Removes map coins overlapping @p box and returns how many were collected.
    int collectCoinsOverlapping(const sf::FloatRect& box);

    float tileSize() const { return tileSizePx; }              ///< Tile size on screen, in pixels.
    float pixelWidth() const { return columns * tileSizePx; }  ///< Full level width, in pixels.
    float pixelHeight() const { return rows * tileSizePx; }    ///< Full level height, in pixels.

    /// @brief Where the player starts, in world pixels (top-left of the spawn tile).
    sf::Vector2f playerSpawn() const { return spawn; }

    /// @brief Top-left world position of every enemy spawn marker in the map.
    const std::vector<sf::Vector2f>& enemySpawns() const { return enemies; }

    /// @brief Bottom-aligned map cells containing Blue Koopa spawn markers ('K').
    const std::vector<sf::Vector2f>& blueKoopaSpawns() const { return blueKoopas; }

    /// @brief Bottom-aligned Green Koopa spawn markers ('G').
    const std::vector<sf::Vector2f>& greenKoopaSpawns() const { return greenKoopas; }

    /// @brief Bottom-aligned Green Paratroopa spawn markers ('J').
    const std::vector<sf::Vector2f>& greenParatroopaSpawns() const {
        return greenParatroopas;
    }

    /// @brief Fixed World 2-2 Blooper markers ('j').
    const std::vector<sf::Vector2f>& blooperSpawns() const { return bloopers; }

    /// @brief World 2-2 Cheep-Cheep markers ('h').
    const std::vector<sf::Vector2f>& cheepCheepSpawns() const { return cheepCheeps; }

    /// @brief Markers two rows above pipes containing Piranha Plants ('R').
    const std::vector<sf::Vector2f>& piranhaSpawns() const { return piranhas; }

    /// @brief Trampoline markers anchored one row above the ground ('D').
    const std::vector<sf::Vector2f>& trampolineSpawns() const { return trampolines; }

    /// @brief Top-left world positions of horizontal moving-platform markers ('L').
    const std::vector<sf::Vector2f>& movingPlatformSpawns() const { return movingPlatforms; }

    /**
     * @struct SecretRoomWarp
     * @brief The pair of pipes that links a stage to a hidden room.
     *
     * Both halves are written into the map file: 'd' marks the mouth of the
     * pipe the player ducks into, 'e' the cell they drop out at inside the
     * room, 'x' the cell in front of the room's side pipe and 'i' the cell they
     * climb back out on. A map missing any of the four has no warp at all.
     */
    struct SecretRoomWarp {
        bool available{false};
        sf::FloatRect entrance;   ///< Pipe mouth in the stage, two tiles wide.
        sf::Vector2f arrival;     ///< Cell the player drops into.
        sf::FloatRect exitMouth;  ///< Room's side pipe, two tiles tall.
        sf::Vector2f returnCell;  ///< Cell the player comes back out on.
    };

    /// @brief The hidden room this map links to, if it declares one.
    const SecretRoomWarp& secretRoom() const { return secret; }

    /// @brief True when the map contains a level-exit decoration marker ('W').
    bool hasLevelExit() const { return levelExitAvailable; }

    /// @brief World-space area occupied by the level-exit decoration.
    sf::FloatRect levelExitBounds() const { return levelExitTrigger; }

    /// @brief True when the map contains a final flagpole marker ('F').
    bool hasGoal() const { return goalAvailable; }

    /// @brief World-space area occupied by the final flagpole.
    sf::FloatRect goalBounds() const { return goalTrigger; }

private:
    /// Every tile written with the same map character, batched into one buffer.
    struct TileBatch {
        char symbol;
        const sf::Texture* texture;
        int frameCount;
        sf::Time frameDuration;
        sf::Time elapsed;      ///< Time spent on the frame currently shown.
        int frame;             ///< Frame the whole batch is currently showing.
        sf::VertexArray vertices;

        /// Width of one frame inside the texture, in source pixels.
        float frameWidth() const {
            return static_cast<float>(texture->getSize().x) / static_cast<float>(frameCount);
        }
    };

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    /// @brief Batch holding @p symbol, or nullptr when no artwork was registered for it.
    TileBatch* batchFor(char symbol);

    /// @brief Scenery batch holding @p symbol, or nullptr when it is not scenery.
    TileBatch* decorationFor(char symbol);

    /// @brief Rebuilds the vertices for one mutable tile symbol.
    void rebuildTileBatch(char symbol);

    std::vector<TileType> types;        ///< Row-major grid of tile types.
    std::vector<char> symbols;          ///< Mutable row-major map symbols.
    std::vector<TileBatch> batches;     ///< One vertex buffer per map character.
    std::vector<TileBatch> decorations; ///< Scenery, drawn first so it stays behind.
    int columns{0};
    int rows{0};
    float tileSizePx{16.f};
    sf::Vector2f spawn{0.f, 0.f};
    std::vector<sf::Vector2f> enemies;     ///< Goomba spawn markers ('E').
    std::vector<sf::Vector2f> blueKoopas;  ///< Blue Koopa spawn markers ('K').
    std::vector<sf::Vector2f> greenKoopas; ///< Green Koopa spawn markers ('G').
    std::vector<sf::Vector2f> greenParatroopas; ///< Green Paratroopa markers ('J').
    std::vector<sf::Vector2f> bloopers; ///< Underwater Blooper markers ('j').
    std::vector<sf::Vector2f> cheepCheeps; ///< Underwater Cheep-Cheep markers ('h').
    std::vector<sf::Vector2f> piranhas; ///< Piranha Plant markers ('R').
    std::vector<sf::Vector2f> trampolines; ///< Trampoline markers ('D').
    std::vector<sf::Vector2f> movingPlatforms; ///< Horizontal lift markers ('L').
    SecretRoomWarp secret;              ///< Hidden-room pipes ('d', 'e', 'x', 'i').
    bool levelExitAvailable{false};
    sf::FloatRect levelExitTrigger;
    bool goalAvailable{false};
    sf::FloatRect goalTrigger;
};
