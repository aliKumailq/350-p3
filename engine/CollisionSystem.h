#pragma once
#include <ranges>
#include "CollisionComponent.h"
#include "Entity.h"
#include "MathUtil.h"
#include <vector>

namespace ECSEngine {

template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& engineM = engine->GetEntityManager();
        auto hasCollision = [&engineM](const EntityID& ID) -> bool {
            return engineM.template HasComponent<CollisionComponent>(ID);

        };

        auto& entityM = engine->GetEntityManager();
        std::vector<Entity> collisionEntities;

        for (auto entity : entityM) {
            if (entityM.template HasComponent<CollisionComponent>(entity.GetID())) collisionEntities.push_back(std::move(entity));
        }

        for (size_t i = 0; i < collisionEntities.size(); ++i) {
            auto& collisionCompI = entityM.template GetComponent<CollisionComponent>(collisionEntities[i].GetID());
            for (size_t j = i + 1; j < collisionEntities.size(); ++j) {
                auto& collisionComp = entityM.template GetComponent<CollisionComponent>(collisionEntities[j].GetID());

            }
            
        }
         
        
        return kContinue; }

        void checkCollision(CollisionComponent& c1, CollisionComponent& c2) {
            Point2D distanceDelta = c1.collisionBox.center() - c2.collisionBox.center();
            


        }

};

}  // namespace ECSEngine
