#include "Systems/TileMap.hpp"
#include <algorithm>
#include <cmath>

namespace {
    /// One entry of the level legend: how a map character behaves.
    struct TileDef {
        char symbol;
        TileType type;
    };

    /**
     * Level legend. Artwork comes from setTileTexture(); this table only decides
     * what a character means to the physics.
     *
     *   #  ground        B  brick          ?  question block   S  staircase block
     *   [] pipe top      {} pipe body      H  hidden block     o  coin
     * Markers handled separately: P player spawn, E enemy spawn, . empty sky.
     * Scenery characters (M m V v l c F X) carry no entry here - they are
     * registered through setDecorationTexture() and never touch the physics.
     */
    constexpr TileDef kTileDefs[] = {
        { '#', TileType::Ground        },
        { 'B', TileType::Brick         },
        { '?', TileType::QuestionBlock },
        { 'S', TileType::StairBlock    },
        { 'H', TileType::HiddenBlock   },
        { '[', TileType::Pipe          },
        { ']', TileType::Pipe          },
        { '{', TileType::Pipe          },
        { '}', TileType::Pipe          },
        { 'o', TileType::Coin          },
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
        return type != TileType::Empty && type != TileType::Decoration
            && type != TileType::Coin;
    }

    /// @brief Appends one rectangle as the two triangles SFML 3 needs.
    void appendQuad(sf::VertexArray& target, sf::Vector2f topLeft, sf::Vector2f size,
                    const sf::FloatRect& texRect) {
        const sf::Vector2f corners[4] = {
            {topLeft.x, topLeft.y},
            {topLeft.x + size.x, topLeft.y},
            {topLeft.x + size.x, topLeft.y + size.y},
            {topLeft.x, topLeft.y + size.y}
        };
        float texRight = texRect.position.x + texRect.size.x;
        float texBottom = texRect.position.y + texRect.size.y;
        const sf::Vector2f texCorners[4] = {
            {texRect.position.x, texRect.position.y},
            {texRight, texRect.position.y},
            {texRight, texBottom},
            {texRect.position.x, texBottom}
        };
        const int triangle[6] = {0, 1, 2, 0, 2, 3};

        for (int corner : triangle) {
            sf::Vertex vertex;
            vertex.position = corners[corner];
            vertex.texCoords = texCorners[corner];
            target.append(vertex);
        }
    }
}

void TileMap::setTileTexture(char symbol, const sf::Texture& texture, int frameCount,
                             sf::Time frameDuration) {
    frameCount = std::max(1, frameCount);

    if (TileBatch* existing = batchFor(symbol)) {
        existing->texture = &texture;
        existing->frameCount = frameCount;
        existing->frameDuration = frameDuration;
        return;
    }

    batches.push_back({symbol, &texture, frameCount, frameDuration, sf::Time::Zero, 0,
                       sf::VertexArray(sf::PrimitiveType::Triangles)});
}

void TileMap::setDecorationTexture(char symbol, const sf::Texture& texture) {
    if (TileBatch* existing = decorationFor(symbol)) {
        existing->texture = &texture;
        return;
    }
    decorations.push_back({symbol, &texture, 1, sf::Time::Zero, sf::Time::Zero, 0,
                           sf::VertexArray(sf::PrimitiveType::Triangles)});
}

TileMap::TileBatch* TileMap::batchFor(char symbol) {
    for (TileBatch& batch : batches) {
        if (batch.symbol == symbol) {
            return &batch;
        }
    }
    return nullptr;
}

TileMap::TileBatch* TileMap::decorationFor(char symbol) {
    for (TileBatch& batch : decorations) {
        if (batch.symbol == symbol) {
            return &batch;
        }
    }
    return nullptr;
}

bool TileMap::build(const MapParser& parser, float scale) {
    const std::vector<std::vector<char>>& grid = parser.getGrid();
    if (grid.empty()) {
        return false;
    }

    columns = static_cast<int>(parser.getWidth());
    rows = static_cast<int>(parser.getHeight());
    tileSizePx = kSourceTileSize * scale;

    types.assign(static_cast<std::size_t>(columns) * rows, TileType::Empty);
    enemies.clear();
    spawn = {0.f, 0.f};

    // Rebuilding restarts every animation, so the baked texture coordinates below
    // (which all point at frame 0) stay in step with what update() thinks is shown.
    for (TileBatch& batch : batches) {
        batch.vertices.clear();
        batch.elapsed = sf::Time::Zero;
        batch.frame = 0;
    }
    for (TileBatch& batch : decorations) {
        batch.vertices.clear();
    }

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

            // Scenery is placed by a single character but covers whatever area its
            // picture needs, so it is handled before the one-cell tiles below.
            if (TileBatch* decor = decorationFor(symbol)) {
                sf::Vector2f artSize(decor->texture->getSize());
                appendQuad(decor->vertices, worldPos, artSize * scale,
                           sf::FloatRect({0.f, 0.f}, artSize));
                continue;
            }

            const TileDef* def = findTile(symbol);
            if (def == nullptr) {
                continue; // sky or an unknown symbol: nothing to store, nothing to draw
            }

            types[static_cast<std::size_t>(row) * columns + col] = def->type;

            // The hidden block is solid but must not be visible until it is struck,
            // so it is deliberately left without artwork.
            TileBatch* batch = batchFor(symbol);
            if (batch == nullptr) {
                continue;
            }

            appendQuad(batch->vertices, worldPos, {tileSizePx, tileSizePx},
                       sf::FloatRect({0.f, 0.f},
                                     {batch->frameWidth(),
                                      static_cast<float>(batch->texture->getSize().y)}));
        }
    }

    return true;
}

void TileMap::update(sf::Time dt) {
    for (TileBatch& batch : batches) {
        if (batch.frameCount <= 1 || batch.frameDuration <= sf::Time::Zero
            || batch.vertices.getVertexCount() == 0) {
            continue;
        }

        batch.elapsed += dt;
        int steps = 0;
        while (batch.elapsed >= batch.frameDuration) {
            batch.elapsed -= batch.frameDuration;
            ++steps;
        }
        if (steps == 0) {
            continue;
        }

        // Frames sit side by side, so switching one is a horizontal slide of the
        // texture coordinates - no need to rebuild the geometry.
        int next = (batch.frame + steps) % batch.frameCount;
        float shift = static_cast<float>(next - batch.frame) * batch.frameWidth();
        batch.frame = next;

        for (std::size_t i = 0; i < batch.vertices.getVertexCount(); ++i) {
            batch.vertices[i].texCoords.x += shift;
        }
    }
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
    // Scenery first: it has to end up behind the blocks and the ground.
    for (const TileBatch& batch : decorations) {
        if (batch.texture == nullptr || batch.vertices.getVertexCount() == 0) {
            continue;
        }
        states.texture = batch.texture;
        target.draw(batch.vertices, states);
    }

    for (const TileBatch& batch : batches) {
        if (batch.texture == nullptr || batch.vertices.getVertexCount() == 0) {
            continue;
        }
        states.texture = batch.texture;
        target.draw(batch.vertices, states);
    }
}
