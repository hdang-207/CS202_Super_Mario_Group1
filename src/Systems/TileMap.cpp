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
     *   # ground   C cloud block   B brick   A star brick   ^ vine brick
     *   * power-up brick   ! power-up question block   5 visible 1-Up brick
     *   b repeatable coin brick (up to 10 coins)
     *   ? question block
     *   U used block      S staircase          [] pipe top    {} pipe body
     *   H hidden coin block   1 hidden 1-Up block   4 hidden power-up block
     *   o coin
     *   g underground ground   r underground brick   p invisible pipe collider
     *   w underwater rock collider   O outdoor ground
     *   s outdoor staircase   = World 2-3 bridge deck
     *   _ water surface   , water body (both drawn, neither solid)
     *   7 Fire-Bar pivot  8 inactive castle pivot
     *   9 lava surface    % lava body
     *   0 Coin Heaven coin
     * Markers handled separately: P player spawn, E Goomba, n half-tile Goomba,
     * K Blue Koopa, G Green Koopa, u half-tile Green Koopa,
     * 2 Red Koopa, J Green Paratroopa, 3 Red Paratroopa,
     * k Hammer Bro, j Blooper, h Cheep-Cheep,
     * R Piranha Plant, D trampoline, ; castle boss, 6 Podoboo,
     * L horizontal lift, : up-and-down lift,
     * ' and " the castle elevators looping up and down,
     * / and \\ the two platforms of a
     * pulley, d/e/x/i the hidden-room pipes, ^/N/>/+ the Coin
     * Heaven route, and . empty sky.
     * Scenery characters (M m V v l c F X W Q I Y Z T t f q z N ~ @ &) carry no entry here - they are
     * registered through setDecorationTexture() and never touch the physics.
     */
    constexpr TileDef kTileDefs[] = {
        { '#', TileType::Ground        },
        { 'C', TileType::Ground        },
        { 'B', TileType::Brick         },
        { 'A', TileType::QuestionBlock },
        { '^', TileType::Brick         },
        { '*', TileType::QuestionBlock },
        { '!', TileType::QuestionBlock },
        { '5', TileType::QuestionBlock },
        { 'b', TileType::CoinBrick     },
        { '?', TileType::QuestionBlock },
        { 'U', TileType::UsedBlock     },
        { 'S', TileType::StairBlock    },
        { 'H', TileType::HiddenBlock   },
        { '1', TileType::HiddenBlock   },
        { '4', TileType::HiddenBlock   },
        { '[', TileType::Pipe          },
        { ']', TileType::Pipe          },
        { '{', TileType::Pipe          },
        { '}', TileType::Pipe          },
        { 'o', TileType::Coin          },
        { '0', TileType::Coin          },
        { '(', TileType::Ground        },
        { '-', TileType::Ground        },
        { ')', TileType::Ground        },
        { '|', TileType::Decoration    },
        { 'g', TileType::Ground        },
        { 'r', TileType::Brick         },
        { 'p', TileType::Pipe          },
        { 'w', TileType::Ground        },
        { 'O', TileType::Ground        },
        { 's', TileType::StairBlock    },
        { '=', TileType::Ground        },
        { '_', TileType::Decoration    },
        { ',', TileType::Decoration    },
        { '7', TileType::Ground        },
        { '8', TileType::Ground        },
        { '9', TileType::Hazard        },
        { '%', TileType::Hazard        },
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
            && type != TileType::Coin && type != TileType::Hazard;
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

void TileMap::rebuildTileBatch(char symbol) {
    TileBatch* batch = batchFor(symbol);
    if (batch == nullptr) {
        return;
    }

    batch->vertices.clear();
    float textureLeft = batch->frame * batch->frameWidth();
    sf::FloatRect textureRect({textureLeft, 0.f},
                              {batch->frameWidth(),
                               static_cast<float>(batch->texture->getSize().y)});

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            std::size_t index = static_cast<std::size_t>(row) * columns + col;
            if (symbols[index] != symbol) {
                continue;
            }
            appendQuad(batch->vertices, {col * tileSizePx, row * tileSizePx},
                       {tileSizePx, tileSizePx}, textureRect);
        }
    }
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
    symbols.assign(static_cast<std::size_t>(columns) * rows, '.');
    spawns.clear();
    enemies.clear();
    blueKoopas.clear();
    greenKoopas.clear();
    redKoopas.clear();
    greenParatroopas.clear();
    redParatroopas.clear();
    bloopers.clear();
    cheepCheeps.clear();
    hammerBros.clear();
    piranhas.clear();
    trampolines.clear();
    movingPlatforms.clear();
    fireBars.clear();
    castleBosses.clear();
    podoboos.clear();
    risingElevators.clear();
    fallingElevators.clear();
    verticalPlatforms.clear();
    balanceLefts.clear();
    balanceRights.clear();
    secret = SecretRoomWarp{};
    coinHeaven = CoinHeavenWarp{};
    levelExitAvailable = false;
    levelExitTrigger = sf::FloatRect();
    goalAvailable = false;
    goalTrigger = sf::FloatRect();

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

    // The four hidden-room markers only count once every one of them is found,
    // so a map holding half a warp simply has none.
    int secretMarkers = 0;
    bool foundCoinHeavenVine = false;
    bool foundCoinHeavenArrival = false;
    bool foundCoinHeavenExit = false;
    bool foundCoinHeavenReturn = false;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < static_cast<int>(grid[row].size()); ++col) {
            char symbol = grid[row][col];
            sf::Vector2f worldPos(col * tileSizePx, row * tileSizePx);
            std::size_t index = static_cast<std::size_t>(row) * columns + col;
            symbols[index] = symbol;

            if (symbol == 'P') {
                spawns.push_back(worldPos);
                continue;
            }
            if (symbol == 'E') {
                enemies.push_back(worldPos);
                continue;
            }
            if (symbol == 'n') {
                enemies.push_back(worldPos + sf::Vector2f(tileSizePx * 0.5f, 0.f));
                continue;
            }
            if (symbol == 'K') {
                blueKoopas.push_back(worldPos);
                continue;
            }
            if (symbol == 'G') {
                greenKoopas.push_back(worldPos);
                continue;
            }
            if (symbol == 'u') {
                greenKoopas.push_back(
                    worldPos + sf::Vector2f(tileSizePx * 0.5f, 0.f));
                continue;
            }
            if (symbol == '2') {
                redKoopas.push_back(worldPos);
                continue;
            }
            if (symbol == 'J') {
                greenParatroopas.push_back(worldPos);
                continue;
            }
            if (symbol == '3') {
                redParatroopas.push_back(worldPos);
                continue;
            }
            if (symbol == 'j') {
                // The guide anchors underwater enemies halfway down a tile.
                // Keeping the inset here lets the text map stay on its 16 px
                // grid while matching the six visible World 2-2 spawns.
                bloopers.push_back(
                    worldPos + sf::Vector2f(0.f, tileSizePx * 0.5f));
                continue;
            }
            if (symbol == 'h') {
                cheepCheeps.push_back(worldPos);
                continue;
            }
            if (symbol == 'k') {
                hammerBros.push_back(worldPos);
                continue;
            }
            if (symbol == 'R') {
                piranhas.push_back(worldPos);
                continue;
            }
            if (symbol == 'D') {
                trampolines.push_back(worldPos);
                continue;
            }
            if (symbol == 'L') {
                movingPlatforms.push_back(worldPos);
                continue;
            }
            if (symbol == '7') {
                fireBars.push_back(worldPos + sf::Vector2f(tileSizePx * 0.5f,
                                                           tileSizePx * 0.5f));
                // The pivot is still a visible solid block, so keep parsing it.
            }
            if (symbol == ';') {
                castleBosses.push_back(worldPos);
                continue;
            }
            if (symbol == '6') {
                podoboos.push_back(worldPos);
                continue;
            }
            if (symbol == '\'') {
                risingElevators.push_back(worldPos);
                continue;
            }
            if (symbol == '"') {
                fallingElevators.push_back(worldPos);
                continue;
            }
            if (symbol == ':') {
                verticalPlatforms.push_back(worldPos);
                continue;
            }
            if (symbol == '/') {
                balanceLefts.push_back(worldPos);
                continue;
            }
            if (symbol == '\\') {
                balanceRights.push_back(worldPos);
                continue;
            }
            if (symbol == '^') {
                coinHeaven.vineBlock = worldPos;
                foundCoinHeavenVine = true;
                // Keep going: unlike the route markers, this is still a brick.
            }
            if (symbol == 'd') {
                secret.entrance = sf::FloatRect(worldPos, {tileSizePx * 2.f, tileSizePx});
                ++secretMarkers;
                continue;
            }
            if (symbol == 'e') {
                secret.arrival = worldPos;
                ++secretMarkers;
                continue;
            }
            if (symbol == 'x') {
                secret.exitMouth = sf::FloatRect(worldPos, {tileSizePx, tileSizePx * 2.f});
                ++secretMarkers;
                continue;
            }
            if (symbol == 'i') {
                secret.returnCell = worldPos;
                ++secretMarkers;
                continue;
            }
            if (symbol == '>') {
                coinHeaven.exitX = worldPos.x;
                foundCoinHeavenExit = true;
                continue;
            }
            if (symbol == '+') {
                coinHeaven.returnCell = worldPos;
                foundCoinHeavenReturn = true;
                continue;
            }

            if (symbol == 'N') {
                // The vine hangs through a deliberate gap in the cloud floor.
                // Receive Mario on the intact cloud immediately to its right.
                coinHeaven.arrival = worldPos
                    + sf::Vector2f(tileSizePx, tileSizePx * 3.f);
                foundCoinHeavenArrival = true;
            }

            // Scenery is placed by a single character but covers whatever area its
            // picture needs, so it is handled before the one-cell tiles below.
            if (TileBatch* decor = decorationFor(symbol)) {
                sf::Vector2f artSize(decor->texture->getSize());
                sf::Vector2f drawSize = artSize * scale;
                sf::Vector2f drawPos = worldPos;
                if (symbol == 'T' || symbol == 't') {
                    // These two source images are tightly cropped from World 2-1.
                    // Centre the narrow tree in its cell and restore the two-pixel
                    // top offset visible in the reference map.
                    drawPos.x += (kSourceTileSize - artSize.x) * 0.5f * scale;
                    drawPos.y += 2.f * scale;
                } else if (symbol == 'N') {
                    // World 3-1's shorter vine hangs half a tile through its
                    // floor gap so Mario can jump and grab the lower stem. The
                    // shared six-tile vine keeps its original one-pixel inset.
                    const float sourceOffset = artSize.y == 79.f ? 9.f : 1.f;
                    drawPos.y += sourceOffset * scale;
                } else if (symbol == 'F') {
                    // The guide-map pole is centred eight source pixels left of
                    // its marker and starts halfway down the marker row.
                    drawPos.x -= 8.f * scale;
                    drawPos.y += 8.f * scale;
                }
                if (symbol == 'I') {
                    // The supplied island is a complete multi-cell sprite. Only
                    // its grassy top is solid; Mario and enemies may pass beside
                    // the hanging trunks exactly like the floating islands in 1-3.
                    const int solidCells = std::max(
                        1, static_cast<int>(std::ceil(artSize.x / kSourceTileSize)));
                    for (int offset = 0; offset < solidCells && col + offset < columns;
                         ++offset) {
                        const std::size_t topIndex =
                            static_cast<std::size_t>(row) * columns + col + offset;
                        types[topIndex] = TileType::Ground;
                    }
                } else if (symbol == 'a') {
                    // Coral is solid for Mario, while aquatic enemies ignore
                    // TileMap physics and may swim through it like in SMB.
                    const int solidRows = std::max(
                        1, static_cast<int>(std::ceil(artSize.y / kSourceTileSize)));
                    for (int offset = 0; offset < solidRows && row + offset < rows;
                         ++offset) {
                        const std::size_t coralIndex =
                            static_cast<std::size_t>(row + offset) * columns + col;
                        types[coralIndex] = TileType::Ground;
                    }
                } else if (symbol == 'W') {
                    levelExitAvailable = true;
                    levelExitTrigger = sf::FloatRect(worldPos, drawSize);
                } else if (symbol == 'F') {
                    goalAvailable = true;
                    goalTrigger = sf::FloatRect(worldPos, drawSize);
                } else if (symbol == '$') {
                    // Castle stages end at the axe rather than a flagpole.
                    goalAvailable = true;
                    goalTrigger = sf::FloatRect(worldPos, drawSize);
                }
                appendQuad(decor->vertices, drawPos, drawSize,
                           sf::FloatRect({0.f, 0.f}, artSize));
                continue;
            }

            const TileDef* def = findTile(symbol);
            if (def == nullptr) {
                continue; // sky or an unknown symbol: nothing to store, nothing to draw
            }

            types[index] = def->type;

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

    secret.available = secretMarkers == 4;
    coinHeaven.available = foundCoinHeavenVine && foundCoinHeavenArrival
                        && foundCoinHeavenExit && foundCoinHeavenReturn;

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

bool TileMap::activateItemBlock(int col, int row) {
    const TileType type = typeAt(col, row);
    if (type != TileType::QuestionBlock && type != TileType::HiddenBlock) {
        return false;
    }

    std::size_t index = static_cast<std::size_t>(row) * columns + col;
    const char oldSymbol = symbols[index];
    types[index] = TileType::UsedBlock;
    symbols[index] = 'U';
    rebuildTileBatch(oldSymbol);
    rebuildTileBatch('U');
    return true;
}

bool TileMap::breakBrick(int col, int row) {
    if (col < 0 || col >= columns || row < 0 || row >= rows) return false;
    std::size_t index = static_cast<std::size_t>(row) * columns + col;
    if (types[index] == TileType::Brick || types[index] == TileType::CoinBrick) {
        char oldSymbol = symbols[index];
        types[index] = TileType::Empty;
        symbols[index] = '.';
        rebuildTileBatch(oldSymbol);
        return true;
    }
    return false;
}

char TileMap::hideBrick(int col, int row) {
    if (col < 0 || col >= columns || row < 0 || row >= rows) return '\0';
    std::size_t index = static_cast<std::size_t>(row) * columns + col;
    if (types[index] == TileType::Brick || types[index] == TileType::CoinBrick || types[index] == TileType::QuestionBlock || types[index] == TileType::UsedBlock) {
        char oldSymbol = symbols[index];
        symbols[index] = '.'; // visually empty
        // type remains unchanged (still solid)
        rebuildTileBatch(oldSymbol);
        return oldSymbol;
    }
    return '\0';
}

void TileMap::restoreBrick(int col, int row, char symbol) {
    if (col < 0 || col >= columns || row < 0 || row >= rows) return;
    std::size_t index = static_cast<std::size_t>(row) * columns + col;
    symbols[index] = symbol;
    rebuildTileBatch(symbol);
}

bool TileMap::removeTile(int col, int row) {
    if (col < 0 || col >= columns || row < 0 || row >= rows) {
        return false;
    }

    const std::size_t index = static_cast<std::size_t>(row) * columns + col;
    const char oldSymbol = symbols[index];
    if (oldSymbol == '.' && types[index] == TileType::Empty) {
        return false;
    }

    symbols[index] = '.';
    types[index] = TileType::Empty;
    rebuildTileBatch(oldSymbol);
    return true;
}

void TileMap::hideDecoration(char symbol) {
    if (TileBatch* decoration = decorationFor(symbol)) {
        decoration->vertices.clear();
    }
    if (symbol == '$' || symbol == 'F') {
        goalAvailable = false;
    } else if (symbol == 'W') {
        levelExitAvailable = false;
    }
}

void TileMap::changeType(int col, int row, TileType newType) {
    if (col < 0 || col >= columns || row < 0 || row >= rows) return;
    std::size_t index = static_cast<std::size_t>(row) * columns + col;
    types[index] = newType;
}

TileType TileMap::typeAt(int col, int row) const {
    if (col < 0 || row < 0 || col >= columns || row >= rows) {
        return TileType::Empty;
    }
    return types[static_cast<std::size_t>(row) * columns + col];
}

char TileMap::symbolAt(int col, int row) const {
    if (col < 0 || row < 0 || col >= columns || row >= rows) {
        return '.';
    }
    return symbols[static_cast<std::size_t>(row) * columns + col];
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
    int lastCol = std::min(columns - 1, static_cast<int>(std::floor(
        (box.position.x + box.size.x - 0.001f) / tileSizePx)));
    int firstRow = std::max(0, static_cast<int>(std::floor(box.position.y / tileSizePx)));
    int lastRow = std::min(rows - 1, static_cast<int>(std::floor(
        (box.position.y + box.size.y - 0.001f) / tileSizePx)));

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

bool TileMap::intersectsHazard(const sf::FloatRect& box) const {
    if (tileSizePx <= 0.f) {
        return false;
    }

    const int firstCol = std::max(
        0, static_cast<int>(std::floor(box.position.x / tileSizePx)));
    const int lastCol = std::min(
        columns - 1, static_cast<int>(std::floor(
            (box.position.x + box.size.x - 0.001f) / tileSizePx)));
    const int firstRow = std::max(
        0, static_cast<int>(std::floor(box.position.y / tileSizePx)));
    const int lastRow = std::min(
        rows - 1, static_cast<int>(std::floor(
            (box.position.y + box.size.y - 0.001f) / tileSizePx)));

    for (int row = firstRow; row <= lastRow; ++row) {
        for (int col = firstCol; col <= lastCol; ++col) {
            if (typeAt(col, row) == TileType::Hazard) {
                return true;
            }
        }
    }
    return false;
}

int TileMap::collectCoinsOverlapping(const sf::FloatRect& box) {
    if (tileSizePx <= 0.f) {
        return 0;
    }

    int firstCol = std::max(0, static_cast<int>(std::floor(box.position.x / tileSizePx)));
    int lastCol = std::min(columns - 1, static_cast<int>(
        std::floor((box.position.x + box.size.x - 0.001f) / tileSizePx)));
    int firstRow = std::max(0, static_cast<int>(std::floor(box.position.y / tileSizePx)));
    int lastRow = std::min(rows - 1, static_cast<int>(
        std::floor((box.position.y + box.size.y - 0.001f) / tileSizePx)));

    int collected = 0;
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int col = firstCol; col <= lastCol; ++col) {
            std::size_t index = static_cast<std::size_t>(row) * columns + col;
            if (types[index] != TileType::Coin) {
                continue;
            }
            types[index] = TileType::Empty;
            symbols[index] = '.';
            ++collected;
        }
    }

    if (collected > 0) {
        rebuildTileBatch('o');
        rebuildTileBatch('0');
    }
    return collected;
}

void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // Scenery first: it has to end up behind the blocks and the ground.
    for (const TileBatch& batch : decorations) {
        if (batch.symbol == 'N') {
            continue; // The vine crosses in front of Coin Heaven's cloud floor.
        }
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

    // Foreground scenery is still non-solid, but must cover the tile layer.
    for (const TileBatch& batch : decorations) {
        if (batch.symbol != 'N' || batch.texture == nullptr
            || batch.vertices.getVertexCount() == 0) {
            continue;
        }
        states.texture = batch.texture;
        target.draw(batch.vertices, states);
    }
}
