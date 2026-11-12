#include <cstddef>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <string>
#include <sstream>
#include <regex>
#include <unordered_map>
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
    assert(line.back() == '2');

    std::unordered_map<char, ECSEngine::SpriteID> c;
    std::getline(fs, line);
    while(line.substr(0,3) != "map") {
        std::string current_parse;
        std::stringstream line_stream{line};

        std::getline(line_stream, current_parse, ' ');

        const char sym = current_parse[0];

        std::getline(line_stream, current_parse, ' ');        
        std::string file_path = std::filesystem::path{path} / std::filesystem::path{current_parse};

        std::array<float, 6> pts = {};
        for (auto& f : pts) assert(line_stream >> f);

        ECSEngine::Rect textRect{{pts[0],pts[1]},pts[2],pts[3]};

        ECSEngine::SpriteManager& spr = engine.GetSpriteManager();
        c[sym] = spr.RegisterTexture(file_path, textRect);


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

    
    for (size_t i = 0; i < static_cast<size_t>(pts[5]); ++i) {
        std::getline(fs, line);
        assert(line.size() == static_cast<size_t>(pts[4]));

        for (size_t j = 0; j < static_cast<size_t>(pts[4]); ++j) {
            if (!c.contains(line[j])) continue; // skip over shit we dont care about.
            ECSEngine::Point2D screenSpot = origin + (ECSEngine::Point2D{static_cast<float>(j),static_cast<float>(i)} * scale);
            ECSEngine::SpriteComponent spritecomp{c[line[j]], {screenSpot, scale, scale}, true};
            ECSEngine::LocationComponent loc{{0.0f,0.0f}};
            auto& entityManager = engine.GetEntityManager();
            auto id = entityManager.CreateEntity("fart");
            entityManager.template addComponent<ECSEngine::SpriteComponent>(id, spritecomp);
            entityManager.template addComponent<ECSEngine::LocationComponent>(id,loc);
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
    LoadMap(engine, "assets", "world.map");

    engine.GetWindowManager().GetWindow()->setFramerateLimit(30);
    engine.GetWindowManager().SetWorldScale(1);
    engine.GetWindowManager().SetWorldCenter({1152, 576});

    // Place player

    // Setup score component

    // Setup Camera

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
