#pragma once

#include <cmath>

#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"

namespace ECSEngine {

// A few box helpers used by more than one system. Everything here works in world space: a
// CollisionComponent stores its box relative to the entity location, so the world box is the local
// box pushed through the location.
namespace CollisionUtil {

inline Rect WorldBox(const CollisionComponent& collision, const LocationComponent& location) {
    return {collision.collisionBox.topLeft + location.loc,
            collision.collisionBox.width, collision.collisionBox.height};
}

inline bool Overlaps(const Rect& a, const Rect& b) {
    return a.topLeft.x < b.topLeft.x + b.width && b.topLeft.x < a.topLeft.x + a.width &&
           a.topLeft.y < b.topLeft.y + b.height && b.topLeft.y < a.topLeft.y + a.height;
}

// How far a overlap b on each axis. Used to push the moving box back out along the shallower axis,
// which is what makes landing on a floor resolve upward instead of sideways.
inline Point2D Penetration(const Rect& a, const Rect& b) {
    const float aCenterX = a.topLeft.x + a.width * 0.5f;
    const float aCenterY = a.topLeft.y + a.height * 0.5f;
    const float bCenterX = b.topLeft.x + b.width * 0.5f;
    const float bCenterY = b.topLeft.y + b.height * 0.5f;

    return {(aCenterX < bCenterX ? -1.0f : 1.0f) * (a.width * 0.5f + b.width * 0.5f -
                                                    std::abs(aCenterX - bCenterX)),
            (aCenterY < bCenterY ? -1.0f : 1.0f) * (a.height * 0.5f + b.height * 0.5f -
                                                    std::abs(aCenterY - bCenterY))};
}

}  // namespace CollisionUtil

}  // namespace ECSEngine
