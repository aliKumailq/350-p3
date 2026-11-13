#pragma once

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include "CameraComponent.h"
#include "CollisionUtil.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SoundNames.h"

namespace ECSEngine {

// How hard a hit shakes, per unit of speed into the wall. Small enough that a walk into a wall is a
// nudge and a full-speed dash is a shove.
const float kShakePerSpeed = 0.35f;
const int kShakeFrames = 8;

// How much speed a loose object keeps after hitting a wall. Losing some on every bounce is what the
// assignment asks for, and it also stops a star rattling forever in a corner.
const float kStarBounceDamping = 0.72f;

// Below this speed a bouncing object stops instead of jittering against the wall forever.
const float kStarRestSpeed = 0.35f;

// Stand-in so the resolve can name a velocity even for an entity that has no movement component.
inline Point2D sZeroVelocity{};

// Runs after movement, so every box is already at its new position. Two passes:

//   1. resolve  -- every dynamic collider against every static collider; the moving one is pushed
//                  back out along the shallower overlap axis and its side flags are set for the
//                  next frame.
//   2. collect  -- every star overlapping the player is removed and its value added to the score.

// Resolution is O(dynamic * static) on purpose. The level has a fixed set of static tiles and only a
// handful of dynamic entities, so a broad phase would cost more than it saves.
template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        ResolveStatics(engine, entities);
        ResolveDynamicPairs(entities);
        CollectStars(engine, entities);

        return kContinue;
    }

private:
    // Play a registered sound if the game set one up. See the note in InputSystem: guarding on
    // HasSound keeps the systems runnable without audio.
    static void Play(EngineView<Components...>* engine, const char* name) {
        auto& sounds = engine->GetSoundManager();
        if (sounds.HasSound(name)) sounds.GetSound(name).play();
    }

    // Push every dynamic collider out of every static one, then set its contact flags.
    static void ResolveStatics(EngineView<Components...>* engine,
                               EntityManager<Components...>& entities) {
        for (const Entity& moverEntity : entities) {
            const EntityID moverID = moverEntity.GetID();
            if (!entities.IsValid(moverID)) continue;
            if (!entities.template HasComponent<CollisionComponent>(moverID)) continue;
            if (!entities.template HasComponent<LocationComponent>(moverID)) continue;

            auto& mover = entities.template GetComponent<CollisionComponent>(moverID);
            if (mover.staticEntity) continue;

            const bool wasFalling = !mover.onGround; 

            mover.onGround = false;
            mover.onCeiling = false;
            mover.onLeftWall = false;
            mover.onRightWall = false;

            auto& moverLoc = entities.template GetComponent<LocationComponent>(moverID);

            for (const Entity& tileEntity : entities) {
                const EntityID tileID = tileEntity.GetID();
                if (tileID == moverID) continue;
                if (!entities.IsValid(tileID)) continue;
                if (!entities.template HasComponent<CollisionComponent>(tileID)) continue;
                if (!entities.template HasComponent<LocationComponent>(tileID)) continue;

                const auto& tile = entities.template GetComponent<CollisionComponent>(tileID);
                if (!tile.staticEntity) continue;

                const auto& tileLoc = entities.template GetComponent<LocationComponent>(tileID);

                const Rect moverBox = CollisionUtil::WorldBox(mover, moverLoc);
                const Rect tileBox = CollisionUtil::WorldBox(tile, tileLoc);
                if (!CollisionUtil::Overlaps(moverBox, tileBox)) continue;

                // Who bounces and who stops? The player owns an InputComponent; everything else
                // that moves is a loose object (stars), and those should ricochet and lose speed
                // rather than stick to the wall.
                const bool bounces = !entities.template HasComponent<InputComponent>(moverID);
                const bool hasMovement = entities.template HasComponent<MovementComponent>(moverID);
                auto& velocity = hasMovement ? entities.template GetComponent<MovementComponent>(moverID).velocity
                                             : sZeroVelocity;
                if (!hasMovement) continue;

                // Resolve along whichever axis is less buried. That is what makes a floor push you
                // up rather than sideways when you land near a corner.
                const Point2D depth = CollisionUtil::Penetration(moverBox, tileBox);
                if (std::abs(depth.x) < std::abs(depth.y)) {
                    moverLoc.loc.x += depth.x;
                    if (depth.x > 0) mover.onLeftWall = true;
                    else mover.onRightWall = true;

                    // Reflect off the vertical face, keeping a fraction of the speed. Stopping dead
                    // would make a spawned star die on the first wall it met.
                    velocity.x = bounces ? -velocity.x * kStarBounceDamping : 0.0f;
                    AddShake(entities, moverID);
                } else {
                    // A downward move into a floor that was not on the ground last frame is a
                    // landing, and that is the one that makes a sound. Checking the previous flag
                    // is what keeps it to one blip per landing instead of every frame of contact.

                    moverLoc.loc.y += depth.y;
                    if (depth.y > 0) mover.onCeiling = true;
                    else mover.onGround = true;

                    velocity.y = bounces ? -velocity.y * kStarBounceDamping : 0.0f;
                    AddShake(entities, moverID);

                    if (wasFalling && mover.onGround) Play(engine, SoundName::kLand);
                }

                // A bouncer that has slowed to a crawl is parked on the wall it hit, so zero it
                // rather than let it twitch against the surface every frame.
                if (bounces && std::abs(velocity.x) < kStarRestSpeed &&
                    std::abs(velocity.y) < kStarRestSpeed) {
                    velocity = {0.0f, 0.0f};
                }
            }
        }
    }

    // Two loose objects that run into each other. The assignment leaves the exact behaviour open,
    // so this is an elastic swap: they push apart along the line between their centres and trade
    // velocity along that line. Bouncing off each other is more interesting to watch than passing
    // through, and it keeps the field from collapsing into a pile.
    static void ResolveDynamicPairs(EntityManager<Components...>& entities) {
        std::vector<EntityID> bouncers;
        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<CollisionComponent>(id)) continue;
            if (!entities.template HasComponent<LocationComponent>(id)) continue;
            if (!entities.template HasComponent<MovementComponent>(id)) continue;
            if (entities.template HasComponent<InputComponent>(id)) continue;  // the player handles itself
            if (entities.template GetComponent<CollisionComponent>(id).staticEntity) continue;
            bouncers.push_back(id);
        }

        for (size_t i = 0; i < bouncers.size(); ++i) {
            for (size_t j = i + 1; j < bouncers.size(); ++j) {
                const EntityID aID = bouncers[i];
                const EntityID bID = bouncers[j];

                const auto& aCol = entities.template GetComponent<CollisionComponent>(aID);
                const auto& bCol = entities.template GetComponent<CollisionComponent>(bID);
                const auto& aLoc = entities.template GetComponent<LocationComponent>(aID);
                const auto& bLoc = entities.template GetComponent<LocationComponent>(bID);

                const Rect aBox = CollisionUtil::WorldBox(aCol, aLoc);
                const Rect bBox = CollisionUtil::WorldBox(bCol, bLoc);
                if (!CollisionUtil::Overlaps(aBox, bBox)) continue;

                const Point2D depth = CollisionUtil::Penetration(aBox, bBox);

                // Separate along the shallower axis, moving both half of the overlap each.
                auto& aLocMut = entities.template GetComponent<LocationComponent>(aID);
                auto& bLocMut = entities.template GetComponent<LocationComponent>(bID);
                if (std::abs(depth.x) < std::abs(depth.y)) {
                    aLocMut.loc.x += depth.x * 0.5f;
                    bLocMut.loc.x -= depth.x * 0.5f;
                } else {
                    aLocMut.loc.y += depth.y * 0.5f;
                    bLocMut.loc.y -= depth.y * 0.5f;
                }

                // Trade the velocity along the axis they were separated on, damped, so the two
                // stars rebound off one another rather than merging.
                auto& aVel = entities.template GetComponent<MovementComponent>(aID).velocity;
                auto& bVel = entities.template GetComponent<MovementComponent>(bID).velocity;
                if (std::abs(depth.x) < std::abs(depth.y)) {
                    std::swap(aVel.x, bVel.x);
                    aVel.x *= kStarBounceDamping;
                    bVel.x *= kStarBounceDamping;
                } else {
                    std::swap(aVel.y, bVel.y);
                    aVel.y *= kStarBounceDamping;
                    bVel.y *= kStarBounceDamping;
                }
            }
        }
    }

    // A star is a dynamic collider that carries a ScoreComponent for its value. The score holder is
    // the one entity with a ScoreComponent and no collider. So "scores" are two uses of the same
    // component: a running total on the HUD entity, a point value on each star.
    static void CollectStars(EngineView<Components...>* engine,
                             EntityManager<Components...>& entities) {
        std::vector<EntityID> collected;

        // Find the score holder once, outside the loop.
        EntityID holder = std::numeric_limits<EntityID>::max();
        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<ScoreComponent>(id)) continue;
            if (entities.template HasComponent<CollisionComponent>(id)) continue;
            holder = id;
            break;
        }
        if (holder == std::numeric_limits<EntityID>::max()) return;

        for (const Entity& starEntity : entities) {
            const EntityID starID = starEntity.GetID();
            if (starID == holder) continue;
            if (!entities.IsValid(starID)) continue;
            if (!entities.template HasComponent<CollisionComponent>(starID)) continue;
            if (!entities.template HasComponent<ScoreComponent>(starID)) continue;
            if (!entities.template HasComponent<LocationComponent>(starID)) continue;

            const auto& starCol = entities.template GetComponent<CollisionComponent>(starID);
            if (starCol.staticEntity) continue;

            const auto& starLoc = entities.template GetComponent<LocationComponent>(starID);
            const Rect starBox = CollisionUtil::WorldBox(starCol, starLoc);

            // Only the player collects: an entity with an InputComponent.
            for (const Entity& otherEntity : entities) {
                const EntityID otherID = otherEntity.GetID();
                if (otherID == starID) continue;
                if (!entities.IsValid(otherID)) continue;
                if (!entities.template HasComponent<InputComponent>(otherID)) continue;
                if (!entities.template HasComponent<CollisionComponent>(otherID)) continue;
                if (!entities.template HasComponent<LocationComponent>(otherID)) continue;

                const auto& otherCol = entities.template GetComponent<CollisionComponent>(otherID);
                const auto& otherLoc = entities.template GetComponent<LocationComponent>(otherID);
                if (!CollisionUtil::Overlaps(starBox, CollisionUtil::WorldBox(otherCol, otherLoc)))
                    continue;

                entities.template GetComponent<ScoreComponent>(holder).score +=
                    entities.template GetComponent<ScoreComponent>(starID).score;
                Play(engine, SoundName::kStar);
                collected.push_back(starID);
                break;
            }
        }

        // Removal happens after the scan, never during it: RemoveEntity frees a slot and could hand
        // the same slot straight back to the next caller.
        for (EntityID id : collected) entities.RemoveEntity(id);
    }

    // Kick the camera. The magnitude scales with how fast we were going into the contact, so a slow
    // walk is a nudge and a dash into a wall is a real jolt.
    static void AddShake(EntityManager<Components...>& entities, EntityID mover) {
        if (!entities.template HasComponent<MovementComponent>(mover)) return;
        if (!entities.template HasComponent<CameraShake>(mover)) return;

        const auto& velocity = entities.template GetComponent<MovementComponent>(mover).velocity;
        const float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (speed < 0.01f) return;

        auto& shake = entities.template GetComponent<CameraShake>(mover);
        const float magnitude = speed * kShakePerSpeed;
        shake.frameLimit = kShakeFrames;
        shake.offset = {velocity.x > 0 ? -magnitude : magnitude,
                        velocity.y > 0 ? -magnitude : magnitude};
    }
};

}  // namespace ECSEngine
