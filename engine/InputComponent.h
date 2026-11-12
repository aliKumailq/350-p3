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
        : entity(whichEntity), lastDash(0) {}
    EntityID entity;
    int lastDash;
    std::bitset<sf::Keyboard::ScancodeCount> keydown;
    std::bitset<sf::Keyboard::ScancodeCount> lastKeydown;
};

}  // namespace ECSEngine
