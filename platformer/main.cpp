#include <cstddef>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <optional>
#include <string>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <vector>
#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SpriteSystem.h"
#include <SFML/Graphics/Texture.hpp>

std::string gResourcePath = "../assets/";

// Frames between star spawns, and how many stars stay alive at once.
static constexpr int kSpawnInterval = 30;

// Where the player starts, and the sprite sheet cells for the player and the star.
static constexpr float kPlayerStartX = 256.0f;
static constexpr float kPlayerStartY = 320.0f;
static constexpr float kPlayerSize = 64.0f;
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

void LoadMap(Engine& engine, const std::string& path, const std::string& file) {
    std::fstream fs;
    fs.open(std::filesystem::path{path} / std::filesystem::path{file}, std::fstream::in);
    assert(!fs.fail());
    std::string line;

    std::getline(fs, line);
    const bool hasBounds = line.back() == '2';
    assert(line.back() == '2' || line.back() == '1');

    struct SpriteData{
        ECSEngine::SpriteID id;
        struct BoundsInfo {
            ECSEngine::Point2D offset;
            float width;
            float height;
        };

        std::optional<BoundsInfo> bounds{};
    };


    std::unordered_map<char, SpriteData> c;
    std::getline(fs, line);
    while(line.substr(0,3) != "map") {
        std::string current_parse;
        std::stringstream line_stream{line};

        std::getline(line_stream, current_parse, ' ');

        const char sym = current_parse[0];

        std::getline(line_stream, current_parse, ' ');
        std::string file_path = (std::filesystem::path{path} / std::filesystem::path{current_parse}).string();


        std::vector<float> pts(hasBounds ? 8 : 4,0.0f);
        for (auto& f : pts) assert(line_stream >> f);

        ECSEngine::Rect textRect{{pts[0],pts[1]},pts[2],pts[3]};

        ECSEngine::SpriteManager& spr = engine.GetSpriteManager();
        SpriteData data{
            spr.RegisterTexture(file_path, textRect),
            hasBounds ? SpriteData::BoundsInfo{ECSEngine::Point2D{pts[4], pts[5]}, pts[6] ,pts[7]} : std::optional<SpriteData::BoundsInfo>{} };


        c[sym] = std::move(data);


        std::getline(fs, line);      
    }

    std::array<float, 6> pts = {};
    std::string placeholder;
    std::stringstream l{line};
    size_t i = 0;
    while (l >> placeholder) {
        try {
            pts[i] = std::stof(placeholder);
            ++i;
        } catch(...) {} // this sucks but im too tired to care.
    }

    assert(i == 6);

    ECSEngine::Point2D origin{pts[0], pts[1]};
    float scale = pts[2]; // WARNING: assuming that given tile <width> <height>, <width> = <height>. Because our scaling doest change between x adn y
    ECSEngine::Point2D worldSize{pts[4], pts[5]};

    
    auto& entityManager = engine.GetEntityManager();

    for (size_t row = 0; row < static_cast<size_t>(pts[5]); ++row) {
        std::getline(fs, line);
        assert(line.size() == static_cast<size_t>(pts[4]));

        for (size_t col = 0; col < static_cast<size_t>(pts[4]); ++col) {
            const char symbol = line[col];
            if (!c.contains(symbol)) continue;  // blank or unused character, nothing to place

            // The grid cell's world position. Every entity gets its own location, which is what
            // the sprite and collision systems treat as the entity's origin.
            const ECSEngine::Point2D cellLoc{origin.x + static_cast<float>(col) * scale,
                                             origin.y + static_cast<float>(row) * scale};

            // The sprite's draw box is relative to the entity location, so it starts at the cell's
            // top-left and is one tile wide and tall.
            ECSEngine::SpriteComponent sprite{c[symbol].id, {ECSEngine::Point2D{0.0f, 0.0f}, scale, scale}, true};
            ECSEngine::LocationComponent loc{cellLoc};

            const ECSEngine::EntityID id = entityManager.CreateEntity("tile");

            // The spawner tile has no sprite of its own -- it is a marker. It gets a spawn
            // component instead and is otherwise invisible.
            if (symbol == 'S') {
                ECSEngine::SpawnComponent spawner{};
                spawner.entityID = id;
                spawner.spawnTextureID = c['S'].id;
                spawner.spawnType = "star";
                spawner.timeToNextSpawn = kSpawnInterval;
                spawner.timeBetweenSpawns = kSpawnInterval;
                spawner.totalSpawns = -1;  // keep going
                entityManager.template addComponent<ECSEngine::SpawnComponent>(id, spawner);
                entityManager.template addComponent<ECSEngine::LocationComponent>(id, loc);
                continue;
            }

            entityManager.template addComponent<ECSEngine::SpriteComponent>(id, sprite);
            entityManager.template addComponent<ECSEngine::LocationComponent>(id, loc);

            if (hasBounds) {
                const auto& info = c[symbol].bounds;
                ECSEngine::CollisionComponent collision{};
                collision.staticEntity = true;
                collision.collisionBox = {info->offset, info->width, info->height};
                entityManager.template addComponent<ECSEngine::CollisionComponent>(id, collision);
            }
        }
    }








}

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    // Engine test code
    Engine engine(1024, 768, "Test Engine");

    // Load sounds

    // Load maps
    LoadMap(engine, "assets", "sky.map");
    LoadMap(engine, "assets", "world.map");

    engine.GetWindowManager().GetWindow()->setFramerateLimit(30);
    engine.GetWindowManager().SetWorldScale(1);
    engine.GetWindowManager().SetWorldCenter({1152, 350});

    // Place player. The character sheet's first 128x128 cell is the orange alien; the sprite is
    // scaled down to a 64-unit box so it fits the tile grid.
    ECSEngine::EntityID player = 0;
    {
        auto& entityManager = engine.GetEntityManager();
        ECSEngine::SpriteID playerSprite = engine.GetSpriteManager().RegisterTexture(
            "assets/spritesheet-characters-default.png",
            ECSEngine::Rect{ECSEngine::Point2D{0.0f, 0.0f}, 128.0f, 128.0f});

        player = entityManager.CreateEntity("player");

        ECSEngine::LocationComponent playerLoc{{kPlayerStartX, kPlayerStartY}};
        ECSEngine::MovementComponent playerMove{{0.0f, 0.0f}};
        ECSEngine::SpriteComponent playerSpriteComp{
            playerSprite, {ECSEngine::Point2D{0.0f, 0.0f}, kPlayerSize, kPlayerSize}, true};
        ECSEngine::CollisionComponent playerCollision{};
        playerCollision.collisionBox = {{0.0f, 0.0f}, kPlayerSize, kPlayerSize};
        ECSEngine::GravityComponent playerGravity{{0.0f, 1.0f}};
        ECSEngine::InputComponent playerInput{player};
        ECSEngine::CameraShake playerShake{};
        ECSEngine::RespawnComponent playerSpawn{{kPlayerStartX, kPlayerStartY}};

        entityManager.addComponent<ECSEngine::LocationComponent>(player, playerLoc);
        entityManager.addComponent<ECSEngine::MovementComponent>(player, playerMove);
        entityManager.addComponent<ECSEngine::SpriteComponent>(player, playerSpriteComp);
        entityManager.addComponent<ECSEngine::CollisionComponent>(player, playerCollision);
        entityManager.addComponent<ECSEngine::GravityComponent>(player, playerGravity);
        entityManager.addComponent<ECSEngine::InputComponent>(player, playerInput);
        entityManager.addComponent<ECSEngine::CameraShake>(player, playerShake);
        entityManager.addComponent<ECSEngine::RespawnComponent>(player, playerSpawn);
    }

    // Setup score component. The HUD digit sprites come from the tiles sheet, which is where the
    // number glyphs live.
    {
        auto& entityManager = engine.GetEntityManager();
        auto& spriteManager = engine.GetSpriteManager();

        ECSEngine::ScoreComponent scoreComponent{};
        scoreComponent.score = 0;
        for (int d = 0; d < 10; ++d) {
            // The digits sit in the last row of the tiles sheet, one 64x64 cell each.
            scoreComponent.digitTextureID[d] = spriteManager.RegisterTexture(
                "assets/spritesheet-tiles-default.png",
                ECSEngine::Rect{ECSEngine::Point2D{static_cast<float>(d) * 64.0f, 768.0f}, 64.0f, 64.0f});
        }

        const ECSEngine::EntityID hud = entityManager.CreateEntity("hud");
        ECSEngine::LocationComponent hudLoc{{8.0f, 8.0f}};
        entityManager.addComponent<ECSEngine::LocationComponent>(hud, hudLoc);
        entityManager.addComponent<ECSEngine::ScoreComponent>(hud, scoreComponent);
    }

    // Setup Camera. One camera entity, pinned to the player, with a fixed y from the map.
    {
        auto& entityManager = engine.GetEntityManager();
        const ECSEngine::EntityID camera = entityManager.CreateEntity("camera");

        ECSEngine::CameraComponent cameraComponent{};
        cameraComponent.position = {kPlayerStartX, 350.0f};
        cameraComponent.worldUnitsPerPixel = 1.0f;
        cameraComponent.speed = 1.0f;

        ECSEngine::CameraFollower follower{};
        follower.entityID = player;

        entityManager.addComponent<ECSEngine::CameraComponent>(camera, cameraComponent);
        entityManager.addComponent<ECSEngine::CameraFollower>(camera, follower);
    }

    // Add all systems
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
