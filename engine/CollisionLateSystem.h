#pragma once

#include "CollisionUtil.h"
#include "EntityManager.h"
#include "LocationComponent.h"

namespace ECSEngine {

// Runs before anything moves. Its only job is to remember where every collider was at the end of
// last frame, so the collision system can sweep between the old box and the new one instead of only
// testing the new one. Without this a fast-moving object can pass clean through a wall.
template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<CollisionComponent>(id)) continue;
            if (!entities.template HasComponent<LocationComponent>(id)) continue;

            auto& collision = entities.template GetComponent<CollisionComponent>(id);
            const auto& location = entities.template GetComponent<LocationComponent>(id);

            collision.previousCollisionBox = CollisionUtil::WorldBox(collision, location);
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
