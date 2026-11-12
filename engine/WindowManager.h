#pragma once

#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {
    using Vector2D = Point2D;

class WindowManager {
public:
    WindowManager(unsigned int width, unsigned int height, const std::string& title);
    ~WindowManager() = default;
    std::shared_ptr<sf::RenderWindow> GetWindow() const;
    Rect GetWindowBounds() const;

    void SetCamera(const Point2D& worldPt, const Point2D& screenPt);
    void SetWorldCenter(const Point2D& worldCenter);
    void SetWorldRect(const Rect& worldCameraRect);
    void SetWorldScale(float worldUnitsPerPixel);
    Point2D GetWorldCenter() const;
    float GetWorldScale() const;

    float WindowToWorld(float x) const;
    float WorldToWindow(float x) const;
    Rect WindowToWorld(const Rect& rect) const;
    Rect WorldToWindow(const Rect& rect) const;
    Point2D WorldToWindow(const Point2D& pt) const;
    Point2D WindowToWorld(const Point2D& pt) const;

    void DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha = 255);
    void DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha = 255);

private:
    std::shared_ptr<sf::RenderWindow> mWindow;
    Vector2D windowSize() const;       // size of the window in screen pixels (i.e. window coords)
    Vector2D mViewCenterWorld;  // world coords point corresponding to the center of our window.
    float mWorldUnitsPerPixel;  
};

}  // namespace ECSEngine
