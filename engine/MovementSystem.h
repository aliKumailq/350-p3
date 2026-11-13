#pragma once

#include "Entity.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "InputComponent.h"
#include "SystemManager.h"
#include <algorithm>
namespace ECSEngine {

template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("MovementSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {

        auto& entityM = engine->GetEntityManager();
        for (const Entity& entity : entityM) {
            const auto& ID = entity.GetID();
            if (!entityM.template HasComponent<MovementComponent>(ID)) continue;

            

            auto& movementC = entityM.template GetComponent<MovementComponent>(ID);
            auto& locationC = entityM.template GetComponent<LocationComponent>(ID);
            auto& gravityC = entityM.template GetComponent<GravityComponent>(ID);
            
            locationC.loc.x += movementC.velocity.x;

            if (movementC.velocity.x >= 0) {
                movementC.velocity.x = std::max(0.0f, movementC.velocity.x - gravityC.acceleration.x);
            } else movementC.velocity.x = std::min(0.0f, movementC.velocity.x + gravityC.acceleration.x);


        }

        return tSystemReturn::kContinue;
    
    }

    


};

}  // namespace ECSEngine
