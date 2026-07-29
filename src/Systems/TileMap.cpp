#include "Systems/TileMap.hpp"
#include <algorithm>
#include <cmath>

namespace {
    /// One entry of the level legend: which artwork a map character draws and how it behaves.
    struct TileDef {
        char symbol;
        int atlasX;      ///< Pixel column of the tile inside the atlas texture.
        int atlasY;      ///< Pixel row of the tile inside the atlas texture.
        TileType type;
    };

    /**
     * Level legend. Atlas coordinates were taken straight from the World 1-1 tile
     * sheet, so the drawn level matches the original artwork exactly.
     *
     * Solid, gameplay-relevant symbols:
     *   #  ground        B  brick          ?  question block   S  staircase block
     *   [] pipe top      {} pipe body      H  hidden block
     * Markers (drawn as sky):
     *   P  player spawn  E  enemy spawn    .  empty sky
     * Everything else is decoration and never collides.
     */
    constexpr TileDef kTileDefs[] = {
        // --- solid ---
        { '#',    0, 208, TileType::Ground        },
        { 'B', 1280,  80, TileType::Brick         },
        { '?',  352,  80, TileType::QuestionBlock },
        { 'S', 3008,  80, TileType::StairBlock    },
        { 'H', 1024, 128, TileType::HiddenBlock   },
        { '[',  736, 144, TileType::Pipe          },
        { ']',  752, 144, TileType::Pipe          },
        { '{',  736, 160, TileType::Pipe          },
        { '}',  752, 160, TileType::Pipe          },
        // --- decoration: clouds ---
        { 'q',  304,  32, TileType::Decoration },
        { 'w',  320,  32, TileType::Decoration },
        { 'e',  336,  32, TileType::Decoration },
        { 'a',  304,  48, TileType::Decoration },
        { 's',  320,  48, TileType::Decoration },
        { 'd',  336,  48, TileType::Decoration },
        // --- decoration: hills ---
        { 'n',   32, 160, TileType::Decoration },
        { 'y',   16, 176, TileType::Decoration },
        { 'u',   32, 176, TileType::Decoration },
        { 'i',   48, 176, TileType::Decoration },
        { 'j',   32, 192, TileType::Decoration },
        { 'k',   48, 192, TileType::Decoration },
        // --- decoration: bushes ---
        { 'z',  176, 192, TileType::Decoration },
        { 'x',  192, 192, TileType::Decoration },
        { 'c',  240, 192, TileType::Decoration },
        // --- decoration: flagpole ---
        { 'O', 3168,  32, TileType::Decoration },
        { '/', 3152,  48, TileType::Decoration },
        { 'F', 3168,  48, TileType::Decoration },
        { '!', 3168,  64, TileType::Decoration },
        { '|', 3168,  80, TileType::Decoration },
        // --- decoration: castle ---
        { '1', 3248, 128, TileType::Decoration },
        { '2', 3248, 144, TileType::Decoration },
        { '3', 3264, 144, TileType::Decoration },
        { '4', 3280, 144, TileType::Decoration },
        { '5', 3248, 160, TileType::Decoration },
        { '6', 3264, 176, TileType::Decoration },
        { '7', 3264, 192, TileType::Decoration },
    };

    /// @brief Looks up a map character; returns nullptr for sky, spawn markers and unknown symbols.
    const TileDef* findTile(char symbol) {
        for (const TileDef& def : kTileDefs) {
            if (def.symbol == symbol) {
                return &def;
            }
        }
        return nullptr;
    }

    bool isSolidType(TileType type) {
        return type != TileType::Empty && type != TileType::Decoration;
    }
}

bool TileMap::build(const MapParser& parser, const sf::Texture& atlas, float scale) {
    const std::vector<std::vector<char>>& grid = parser.getGrid();
    if (grid.empty()) {
        return false;
    }

    atlasTexture = &atlas;
    columns = static_cast<int>(parser.getWidth());
    rows = static_cast<int>(parser.getHeight());
    tileSizePx = kSourceTileSize * scale;

    types.assign(static_cast<std::size_t>(columns) * rows, TileType::Empty);
    enemies.clear();
    spawn = {0.f, 0.f};

    // SFML 3 has no quad primitive, so each tile is drawn as two triangles.
    vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    vertices.clear();

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < static_cast<int>(grid[row].size()); ++col) {
            char symbol = grid[row][col];
            sf::Vector2f worldPos(col * tileSizePx, row * tileSizePx);

            if (symbol == 'P') {
                spawn = worldPos;
                continue;
            }
            if (symbol == 'E') {
                enemies.push_back(worldPos);
                continue;
            }

            const TileDef* def = findTile(symbol);
            if (def == nullptr) {
                continue; // sky or an unknown symbol: nothing to store, nothing to draw
            }

            types[static_cast<std::size_t>(row) * columns + col] = def->type;

            // The hidden block is solid but must not be visible until it is struck.
            if (def->type == TileType::HiddenBlock) {
                continue;
            }

            float left = worldPos.x;
            float top = worldPos.y;
            float right = left + tileSizePx;
            float bottom = top + tileSizePx;

            float texLeft = static_cast<float>(def->atlasX);
            float texTop = static_cast<float>(def->atlasY);
            float texRight = texLeft + kSourceTileSize;
            float texBottom = texTop + kSourceTileSize;

            const sf::Vector2f corners[4] = {
                {left, top}, {right, top}, {right, bottom}, {left, bottom}
            };
            const sf::Vector2f texCorners[4] = {
                {texLeft, texTop}, {texRight, texTop}, {texRight, texBottom}, {texLeft, texBottom}
            };
            const int triangle[6] = {0, 1, 2, 0, 2, 3};

            for (int corner : triangle) {
                sf::Vertex vertex;
                vertex.position = corners[corner];
                vertex.texCoords = texCorners[corner];
                vertices.append(vertex);
            }
        }
    }

    return true;
}

TileType TileMap::typeAt(int col, int row) const {
    if (col < 0 || row < 0 || col >= columns || row >= rows) {
        return TileType::Empty;
    }
    return types[static_cast<std::size_t>(row) * columns + col];
}

bool TileMap::isSolid(int col, int row) const {
    return isSolidType(typeAt(col, row));
}

std::vector<sf::FloatRect> TileMap::solidTilesOverlapping(const sf::FloatRect& box) const {
    std::vector<sf::FloatRect> hits;
    if (tileSizePx <= 0.f) {
        return hits;
    }

    int firstCol = std::max(0, static_cast<int>(std::floor(box.position.x / tileSizePx)));
    int lastCol = std::min(columns - 1, static_cast<int>(std::floor((box.position.x + box.size.x) / tileSizePx)));
    int firstRow = std::max(0, static_cast<int>(std::floor(box.position.y / tileSizePx)));
    int lastRow = std::min(rows - 1, static_cast<int>(std::floor((box.position.y + box.size.y) / tileSizePx)));

    for (int row = firstRow; row <= lastRow; ++row) {
        for (int col = firstCol; col <= lastCol; ++col) {
            if (isSolid(col, row)) {
                hits.push_back(sf::FloatRect({col * tileSizePx, row * tileSizePx},
                                             {tileSizePx, tileSizePx}));
            }
        }
    }
    return hits;
}

bool TileMap::intersectsSolid(const sf::FloatRect& box) const {
    return !solidTilesOverlapping(box).empty();
}

void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (atlasTexture == nullptr || vertices.getVertexCount() == 0) {
        return;
    }
    states.texture = atlasTexture;
    target.draw(vertices, states);
}
