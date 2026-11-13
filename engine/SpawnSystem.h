#pragma once

#include <cmath>
#include <random>
#include <vector>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
#include "SpriteComponent.h"
#include "SpawnComponent.h"

namespace ECSEngine {

// A spawned star: 32 units across, worth one point, thrown at a fixed speed in a random direction.
const float kStarSize = 32.0f;
const float kStarSpeed = 6.0f;

// One shared generator. Seeded from random_device so runs differ, which is fine here: nothing about
// the game's correctness depends on the star directions.
inline std::mt19937& StarRng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

// Walks every spawner, counts its timer down, and once it fires builds a star entity. New entities
// are created after the scan, never during it, because CreateEntity can reallocate the entity
// storage while we are iterating it.
template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpawnSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        struct PendingSpawn {
            Point2D at;
            SpriteID textureID;
            Point2D velocity;
        };
        std::vector<PendingSpawn> pending;

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<SpawnComponent>(id)) continue;
            if (!entities.template HasComponent<LocationComponent>(id)) continue;

            auto& spawner = entities.template GetComponent<SpawnComponent>(id);
            const auto& loc = entities.template GetComponent<LocationComponent>(id);

            if (spawner.totalSpawns == 0) continue;  // exhausted

            if (++spawner.timeToNextSpawn >= spawner.timeBetweenSpawns) {
                spawner.timeToNextSpawn = 0;
                if (spawner.totalSpawns > 0) --spawner.totalSpawns;  // -1 means unlimited

                std::uniform_real_distribution<float> angle(0.0f, 6.2831853f);
                const float theta = angle(StarRng());
                pending.push_back({{loc.loc.x, loc.loc.y},
                                   spawner.spawnTextureID,
                                   {std::cos(theta) * kStarSpeed, std::sin(theta) * kStarSpeed}});
            }
        }

        for (const PendingSpawn& spawn : pending) {
            const EntityID star = entities.CreateEntity("star");

            LocationComponent starLoc{spawn.at};
            MovementComponent starMove{spawn.velocity};
            // The star's sprite reuses the spawner's texture and is drawn at the star's own box.
            SpriteComponent starSprite{spawn.textureID,
                                       {{0.0f, 0.0f}, kStarSize, kStarSize}, true};
            CollisionComponent starCollision{};
            starCollision.collisionBox = {{0.0f, 0.0f}, kStarSize, kStarSize};

            entities.template addComponent<LocationComponent>(star, starLoc);
            entities.template addComponent<MovementComponent>(star, starMove);
            entities.template addComponent<SpriteComponent>(star, starSprite);
            entities.template addComponent<CollisionComponent>(star, starCollision);
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
