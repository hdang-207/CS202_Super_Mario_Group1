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
    Decoration,    ///< Drawn but never collides (hills, bushes, clouds, flagpole, castle).
    Ground,        ///< Solid terrain.
    Brick,         ///< Breakable brick block (solid).
    QuestionBlock, ///< Item block (solid).
    StairBlock,    ///< Solid staircase block.
    HiddenBlock,   ///< Invisible until struck from below, but solid.
    Pipe           ///< Solid pipe segment.
};

/**
 * @class TileMap
 * @brief Turns a parsed character grid into a drawable, collidable level.
 *
 * Each map character maps to one 16x16 tile in the level atlas texture plus a
 * TileType that decides whether it is solid. The whole level is uploaded once
 * into a vertex array, so rendering costs a single draw call no matter how big
 * the level is.
 *
 * Physics code does not need to know anything about characters or artwork: it
 * asks for the solid tiles overlapping a bounding box and resolves against those.
 */
class TileMap : public sf::Drawable {
public:
    /// Size of one tile in the source atlas, in pixels.
    static constexpr int kSourceTileSize = 16;

    /**
     * @brief Builds the level geometry from a parsed map.
     * @param parser Map whose character grid describes the level.
     * @param atlas Texture holding the 16x16 tile artwork.
     * @param scale Zoom factor applied to every tile (use an integer to stay pixel-perfect).
     * @return True if the map contained at least one row.
     */
    bool build(const MapParser& parser, const sf::Texture& atlas, float scale);

    /**
     * @brief Draws one tile type with its own image instead of the atlas artwork.
     * @param type Tile type to re-skin, e.g. TileType::Ground for the '#' characters.
     * @param texture Standalone image; it is stretched over the whole tile, so a
     *        16x16 png lines up one-to-one with the rest of the level.
     *
     * Call this before build() - the geometry is baked there. Every overridden
     * type costs one extra draw call, which is nothing next to the single call
     * the atlas needs for the rest of the level.
     */
    void setTileTexture(TileType type, const sf::Texture& texture);

    /// @brief True if the tile at this grid cell blocks movement (out of bounds is not solid).
    bool isSolid(int col, int row) const;

    /// @brief Gameplay type of a grid cell, or TileType::Empty when out of bounds.
    TileType typeAt(int col, int row) const;

    /// @brief World-space rectangles of every solid tile overlapping @p box.
    std::vector<sf::FloatRect> solidTilesOverlapping(const sf::FloatRect& box) const;

    /// @brief True if any solid tile overlaps @p box.
    bool intersectsSolid(const sf::FloatRect& box) const;

    float tileSize() const { return tileSizePx; }              ///< Tile size on screen, in pixels.
    float pixelWidth() const { return columns * tileSizePx; }  ///< Full level width, in pixels.
    float pixelHeight() const { return rows * tileSizePx; }    ///< Full level height, in pixels.

    /// @brief Where the player starts, in world pixels (top-left of the spawn tile).
    sf::Vector2f playerSpawn() const { return spawn; }

    /// @brief Top-left world position of every enemy spawn marker in the map.
    const std::vector<sf::Vector2f>& enemySpawns() const { return enemies; }

private:
    /// Tiles of one type that are drawn from their own image rather than the atlas.
    struct TextureBatch {
        TileType type;
        const sf::Texture* texture;
        sf::VertexArray vertices;
    };

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    /// @brief Batch that owns @p type, or nullptr when the atlas draws it.
    TextureBatch* batchFor(TileType type);

    std::vector<TileType> types;        ///< Row-major grid of tile types.
    sf::VertexArray vertices;           ///< Everything drawn from the atlas, in one buffer.
    std::vector<TextureBatch> batches;  ///< One extra buffer per re-skinned tile type.
    const sf::Texture* atlasTexture{nullptr};
    int columns{0};
    int rows{0};
    float tileSizePx{16.f};
    sf::Vector2f spawn{0.f, 0.f};
    std::vector<sf::Vector2f> enemies;
};
