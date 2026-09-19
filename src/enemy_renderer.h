#pragma once
#include "enemy_system.h"
#include <array>
class UiFont;
class EnemyRenderer {
public:
    EnemyRenderer();
    ~EnemyRenderer();
    EnemyRenderer(const EnemyRenderer&)=delete;
    EnemyRenderer& operator=(const EnemyRenderer&)=delete;
    void Draw(const EnemySystem& system, const UiFont& font) const;
    void DrawUnit(const EnemyUnit& unit, const UiFont& font) const;
    bool HasSprite(EnemyKind kind) const;
private:
    std::array<Texture2D,6> textures_{};
    std::array<Vector2,6> anchors_{};
    std::array<std::array<float,4>,6> durations_{};
    std::array<std::array<int,4>,6> facing_{};
};
