#pragma once

#include <bitset>
#include <limits>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace ECSEngine {

struct InputComponent {
    // Defaulted so the sparse set can clear a removed slot with InputComponent{}.
    InputComponent(EntityID whichEntity = std::numeric_limits<EntityID>::max())
        : entity(whichEntity), lastDash(0), lastWallSide(0) {}
    EntityID entity;
    int lastDash;  // frames since the last dash, counted up to kDashInterval
    // Which wall the last wall jump came off: -1 left, +1 right, 0 none. A new wall jump has to use a
    // different wall (or come after touching the ground), so one wall cannot be climbed by mashing.
    int lastWallSide;
    std::bitset<sf::Keyboard::ScancodeCount> keydown;
    std::bitset<sf::Keyboard::ScancodeCount> lastKeydown;
};

}  // namespace ECSEngine
