#pragma once

#include <algorithm>

#include "Keyboard.h"

namespace ECSEngine {

// Speeds are in world units per frame at the fixed step the systems use, not per second.
//
// Horizontal movement is a ramp, not a snap: the player accelerates toward kMaxRunVelocity and the
// system only clamps the result. These are the tuning constants the assignment lets us choose, so
// they sit together in one block instead of being scattered through the system body.
const float kMaxRunVelocity = 15;
const float kDashVelocity = 50;
const float kJumpVelocity = 25;
const float kGroundRunAcceleration = 5;
const float kAirRunAcceleration = 4;
const float kDashInterval = 25;

// One frame of the player's intent, read out of the input bitset. The system builds this so the
// behaviour reads in words instead of bit indices.
struct InputState {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool jump = false;
    bool dash = false;
};

template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;                        // skip the gaps removals leave
            if (!entities.template HasComponent<InputComponent>(id)) continue;
            if (!entities.template HasComponent<MovementComponent>(id)) continue;

            auto& input = entities.template GetComponent<InputComponent>(id);
            auto& movement = entities.template GetComponent<MovementComponent>(id);

            const InputState state = ReadInput(input);

            const bool onGround = entities.template HasComponent<CollisionComponent>(id) &&
                                  entities.template GetComponent<CollisionComponent>(id).onGround;
            const float accel = onGround ? kGroundRunAcceleration : kAirRunAcceleration;

            // Arrow keys drive horizontal velocity whether or not we are airborne. Letting go
            // bleeds the speed off instead of stopping dead, which is what makes the movement feel
            // like a ramp rather than a toggle.
            float& vx = movement.velocity.x;
            if (state.left && !state.right) {
                vx = std::max(vx - accel, -kMaxRunVelocity);
            } else if (state.right && !state.left) {
                vx = std::min(vx + accel, kMaxRunVelocity);
            } else if (vx > 0) {
                vx = std::max(vx - accel, 0.0f);
            } else if (vx < 0) {
                vx = std::min(vx + accel, 0.0f);
            }

            // Dash is edge triggered -- the key must go down this frame and not last frame -- and
            // rate limited so it can only fire once per kDashInterval frames.
            const bool dashPressed = state.dash && !input.lastKeydown.test(Key::C);
            if (dashPressed && input.lastDash >= kDashInterval) {
                vx = (vx < 0 ? -1.0f : 1.0f) * kDashVelocity;
                input.lastDash = 0;
            } else {
                ++input.lastDash;
            }

            // Jump is edge triggered and only from the ground. Wall jumps are handled in gravity
            // and collision, because they depend on the wall flags from the previous frame.
            const bool jumpPressed = state.jump && !input.lastKeydown.test(Key::Z);
            if (jumpPressed && onGround) movement.velocity.y = -kJumpVelocity;

            input.lastKeydown = input.keydown;
        }

        return kContinue;
    }

private:
    static InputState ReadInput(const InputComponent& input) {
        InputState s;
        s.left  = input.keydown.test(Key::Left);
        s.right = input.keydown.test(Key::Right);
        s.up    = input.keydown.test(Key::Up);
        s.down  = input.keydown.test(Key::Down);
        s.jump  = input.keydown.test(Key::Z);
        s.dash  = input.keydown.test(Key::C);
        return s;
    }
};

}  // namespace ECSEngine
