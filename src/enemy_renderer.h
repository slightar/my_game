#pragma once
#include "enemy_system.h"
#include <array>
class UiFont;
class EnemyRenderer {
public:
    static constexpr unsigned kKindCount = static_cast<unsigned>(EnemyKind::Count);
    EnemyRenderer();
    ~EnemyRenderer();
    EnemyRenderer(const EnemyRenderer&)=delete;
    EnemyRenderer& operator=(const EnemyRenderer&)=delete;
    void Draw(const EnemySystem& system, const UiFont& font) const;
    void DrawUnit(const EnemyUnit& unit, const UiFont& font) const;
    bool HasSprite(EnemyKind kind) const;
private:
    std::array<Texture2D,kKindCount> textures_{};
    std::array<Vector2,kKindCount> anchors_{};
    std::array<std::array<float,4>,kKindCount> durations_{};
    std::array<std::array<int,4>,kKindCount> facing_{};
};
