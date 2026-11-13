#pragma once

#include <bitset>
#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "ECSEngine.h"
#include "Keyboard.h"
#include "LocationComponent.h"
#include "SystemManager.h"
#include <algorithm>

namespace ECSEngine {

const float kMaxRunVelocity = 15;
const float kDashVelocity = 50;
const float kJumpVelocity = 25;
const float kGroundRunAcceleration = 5;
const float kAirRunAcceleration = 4;
const float kDashInterval = 25;

struct PlayerAttributes {
    const CollisionComponent& collision;
    MovementComponent& movement;
    const LocationComponent& location;
};

template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override { 
        auto& entityM = engine->GetEntityManager();

        auto isPressed = [](const std::bitset<sf::Keyboard::ScancodeCount>& keyBits, const unsigned int& key) {
            return keyBits.test(static_cast<int>(key));
        };



        for (const Entity& entity : entityM) {
            auto ID = entity.GetID();
            if (!entityM.template HasComponent<InputComponent>(ID)) continue;

            auto& MovementC = entityM.template GetComponent<MovementComponent>(ID);
            auto& InputC = entityM.template GetComponent<InputComponent>(ID);
            const auto& CollisionC = entityM.template GetComponent<CollisionComponent>(ID);

            const auto& keyBits = InputC.keydown;
            auto& xVel = MovementC.velocity.x;
            auto& yVel = MovementC.velocity.y;

            auto rW = [&CollisionC, &isPressed, &keyBits](const unsigned int key) -> bool {
                if (!isPressed(keyBits,key)) return false;
                if (key == Key::Left) return !CollisionC.onLeftWall;
                if (key == Key::Right) return !CollisionC.onRightWall;
                if (key == Key::Z) {
                    if (CollisionC.onGround) return true;
                    else if (CollisionC.onRightWall && isPressed(keyBits,Key::Right)) return true;
                    else if (CollisionC.onLeftWall && isPressed(keyBits,Key::Left)) return true;
                } return false;

            };

            if (rW(Key::Left)) xVel =  std::max(- kMaxRunVelocity, xVel - kGroundRunAcceleration);
            if (rW(Key::Right)) xVel = std::min(kMaxRunVelocity, xVel + kGroundRunAcceleration);

           // if (rW(Key::Z)) yVel = std::min(kJumpVelocity, yVel - kAirRunAcceleration * 5);

        }

                return tSystemReturn::kContinue;

    }

    private:
    inline void PlayerMovement(PlayerAttributes& player) {

    }
};

}  // namespace ECSEngine
