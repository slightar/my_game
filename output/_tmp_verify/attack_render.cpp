// End-to-end check of the shield soldier's attack facing.
//
// Unlike a hand-built EnemyUnit, this drives the REAL EnemySystem::Update with a real
// player position, then renders whatever the unit actually is through the REAL
// EnemyRenderer::DrawUnit -- texture load, row/frame selection, mirroring and all.
// For every rendered tick it measures the sprite's alpha-weighted horizontal centroid
// relative to feet.x (negative = the sprite's mass sits to the left of the feet, i.e.
// the pose reads as facing left).  It also totals how long each frame stays on screen,
// because a wrong-facing pose held for half a second is what a player notices.
#include "enemy_data.h"
#include "enemy_renderer.h"
#include "enemy_system.h"
#include "file_path.h"
#include "ui_font.h"
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

// Manifest values for the shield (assets/enemies/mobs/manifest.json).
constexpr float kWindup = 0.95F, kRecovery = 1.25F, kStrike = 0.22F;

const char* StateName(EnemyState s) {
    switch (s) {
        case EnemyState::Approach: return "Approach";
        case EnemyState::Windup: return "Windup";
        case EnemyState::Strike: return "Strike";
        case EnemyState::Recover: return "Recover";
        case EnemyState::Fuse: return "Fuse";
        case EnemyState::Blast: return "Blast";
        case EnemyState::Dead: return "Dead";
    }
    return "?";
}

// Mirror of EnemyRenderer::DrawUnit's row/frame selection, so the harness can report the
// exact sprite cell that was just drawn.
int CellFor(const EnemyUnit& u, int& row, int& frame) {
    const auto& d = EnemyData(u.kind);
    // Manifest durations for the shield, used by the idle/walk fallback rows.
    const float durations[4] = {2.0F, 1.2F, 1.0F, 0.9F};
    row = u.moving ? 1 : 0;
    frame = static_cast<int>(u.animationTime * 10 / std::max(.01F, durations[row])) % 10;
    if (u.state == EnemyState::Windup) { row = 2; frame = std::clamp(int((1 - u.timer / d.windup) * 4), 0, 3); }
    if (u.state == EnemyState::Strike) { row = 2; frame = 4 + std::clamp(int(u.animationTime / .22F * 4), 0, 3); }
    if (u.state == EnemyState::Recover && u.stun <= 0) { row = 2; frame = 8 + std::min(1, int(u.animationTime / std::max(.01F, d.recovery) * 2)); }
    if (u.state == EnemyState::Dead) { row = 3; frame = std::min(9, int(u.animationTime / .8F * 10)); }
    return row;
}

float RenderBias(EnemyRenderer& renderer, const UiFont& font, EnemyUnit unit, float feetX) {
    auto target = LoadRenderTexture(512, 512);
    BeginTextureMode(target);
    ClearBackground(BLANK);
    unit.feet = {feetX, 400.0F};
    renderer.DrawUnit(unit, font);
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    double total = 0, weighted = 0;
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x) {
            const unsigned char a = GetImageColor(image, x, y).a;
            if (a < 24) continue;
            total += a;
            weighted += static_cast<double>(x) * a;
        }
    UnloadImage(image);
    UnloadRenderTexture(target);
    if (total <= 0) return 0.0F;
    return static_cast<float>(weighted / total) - feetX;
}

struct Tally {
    int count = 0;
    double biasSum = 0;
    float biasMin = 1e9F, biasMax = -1e9F;
    int facing = 0;
    float feetX = 0;
};

void Run(const char* title, float shieldX, float playerX) {
    std::printf("\n=== %s  (shield x=%.0f, player x=%.0f) ===\n", title, shieldX, playerX);
    EnemyRenderer renderer;
    UiFont font;
    EnemySystem system;
    system.Clear();
    system.Spawn(EnemyKind::Shield, shieldX);

    Tally perCell[4][10];
    const Vector2 player{playerX, GameConfig::kFloorY};
    const int steps = 900;                     // 15 s at 60 Hz
    for (int step = 0; step < steps; ++step) {
        system.Update(1.0F / 60.0F, player);
        if (system.Units().empty()) continue;
        const EnemyUnit& u = system.Units().front();
        int row = 0, frame = -1;
        CellFor(u, row, frame);
        if (frame < 0 || frame > 9) continue;
        const float bias = RenderBias(renderer, font, u, 256.0F);
        Tally& t = perCell[row][frame];
        ++t.count;
        t.biasSum += bias;
        t.biasMin = std::min(t.biasMin, bias);
        t.biasMax = std::max(t.biasMax, bias);
        t.facing = u.facing;
        t.feetX = u.feet.x;
    }

    std::printf("  cell        screen   avg facing bias      range            verdict\n");
    for (int row = 0; row < 4; ++row) {
        for (int frame = 0; frame < 10; ++frame) {
            const Tally& t = perCell[row][frame];
            if (!t.count) continue;
            const double avg = t.biasSum / t.count;
            const char* verdict;
            if (std::abs(avg) < 6.0) verdict = "neutral";
            else if (t.facing < 0) verdict = avg < 0 ? "ok (follows facing)" : "WRONG WAY";
            else verdict = avg > 0 ? "ok (follows facing)" : "WRONG WAY";
            const char* role = row == 0 ? "idle" : row == 1 ? "walk" : row == 2 ? "attack" : "die";
            std::printf("  r%d.%-5s f%d %6.0f ms   %+8.2f          [%+.1f, %+.1f]   %s\n",
                        row, role, frame, t.count * 1000.0 / 60.0, avg, t.biasMin, t.biasMax, verdict);
        }
    }
}

// Build a unit that lands on a chosen attack-row cell (row 2, frame 0-9) using the
// same state/timer values the simulation would have at that instant.
EnemyUnit AttackCell(int frame, int facing) {
    EnemyUnit u;
    u.kind = EnemyKind::Shield;
    u.facing = facing;
    u.health = EnemyData(EnemyKind::Shield).health;
    u.moving = false;
    if (frame <= 3) {
        u.state = EnemyState::Windup;
        u.timer = kWindup * (1.0F - (frame + 0.5F) / 4.0F);   // frame = int((1-t/w)*4)
    } else if (frame <= 7) {
        u.state = EnemyState::Strike;
        u.animationTime = (frame - 4 + 0.5F) * (kStrike / 4.0F);  // frame = 4+int(t/.22*4)
    } else {
        u.state = EnemyState::Recover;
        u.turning = false;
        u.stun = 0;
        u.animationTime = frame == 8 ? 0.15F : 0.90F;         // frame = 8+min(1,int(t/1.25*2))
    }
    return u;
}

// Contact sheet the user can open: both facings x every attack frame, with the feet
// line drawn so the direction of the swing is obvious at a glance.
void ExportSheet(EnemyRenderer& renderer, const UiFont& font) {
    const int cell = 320, cols = 10, rows = 2;
    auto target = LoadRenderTexture(cols * cell, rows * cell + 28);
    BeginTextureMode(target);
    ClearBackground(Color{24, 28, 36, 255});
    for (int r = 0; r < rows; ++r) {
        const int facing = r == 0 ? -1 : 1;
        for (int f = 0; f < cols; ++f) {
            EnemyUnit u = AttackCell(f, facing);
            u.feet = {f * cell + cell / 2.0F, r * cell + 250.0F};
            renderer.DrawUnit(u, font);
            DrawLine(static_cast<int>(u.feet.x), r * cell,
                     static_cast<int>(u.feet.x), r * cell + cell, Color{255, 96, 96, 150});
        }
    }
    DrawText("top row: guard facing LEFT   |   bottom row: guard facing RIGHT",
             8, rows * cell + 6, 20, Color{232, 234, 238, 255});
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    const std::string path =
        std::string(Utf8Path(GetApplicationDirectory()).string()) + "/shield-attack-facing.png";
    const bool ok = ExportImage(image, path.c_str());
    UnloadImage(image);
    UnloadRenderTexture(target);
    std::printf("\ncontact sheet (%s): %s\n", ok ? "written" : "FAILED", path.c_str());
}

}  // namespace

int main() {
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 720, "attack facing render");
    if (!IsWindowReady()) { std::fprintf(stderr, "window not ready\n"); return 2; }

    // Player to the LEFT of the shield -> the guard must face left for the whole attack.
    Run("player on the LEFT", 1960.0F, 1810.0F);
    // Player to the RIGHT -> mirrored case.
    Run("player on the RIGHT", 1960.0F, 2110.0F);

    {
        EnemyRenderer renderer;
        UiFont font;
        ExportSheet(renderer, font);
    }

    CloseWindow();
    return 0;
}
