#include "WindowManager.h"
#include "SfHelper.h"

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const {
    auto windowSize = SfHelper::PntSf(mWindow->getSize()); 
    return {{0.0f,0.0f}, windowSize.x, windowSize.y}; 
}

void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {}

Rect WindowManager::WindowToWorld(const Rect& rect) const { 
    return {WindowToWorld(rect.topLeft), WindowToWorld(rect.width), WindowToWorld(rect.height)}; }

Point2D WindowManager::WindowToWorld(const Point2D& pt) const { 
    return mViewCenterWorld - ((windowSize() * 0.5f) * mWorldUnitsPerPixel) +
               (pt * mWorldUnitsPerPixel); }

Rect WindowManager::WorldToWindow(const Rect& rect) const { 
    return {WorldToWindow(rect.topLeft), WorldToWindow(rect.width), WorldToWindow(rect.height)}; }

Point2D WindowManager::WorldToWindow(const Point2D& pt) const { return (pt - (mViewCenterWorld - ((windowSize() * 0.5f) * mWorldUnitsPerPixel))) /
               mWorldUnitsPerPixel; }

void WindowManager::SetCamera(const Point2D& pWorld, const Point2D& pWindow) {
    mViewCenterWorld =
            pWorld - pWindow * mWorldUnitsPerPixel + windowSize() * 0.5f * mWorldUnitsPerPixel;
}

Point2D WindowManager::GetWorldCenter() const { return mViewCenterWorld; }

float WindowManager::GetWorldScale() const { return mWorldUnitsPerPixel; }

void WindowManager::SetWorldCenter(const Point2D& worldCenter) { mViewCenterWorld = worldCenter;}

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    mWorldUnitsPerPixel = worldUnitsPerPixel;
}

void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    SetWorldCenter(worldCameraRect.center());
    // FIXME: also needs to account for scaling i.e. rectangles that are different size of the window.
}

float WindowManager::WindowToWorld(float x) const { return x * mWorldUnitsPerPixel; }

float WindowManager::WorldToWindow(float x) const { return x / mWorldUnitsPerPixel; }

Vector2D WindowManager::windowSize() const {
    return {GetWindowBounds().width, GetWindowBounds().height};
}

}  // namespace ECSEngine
