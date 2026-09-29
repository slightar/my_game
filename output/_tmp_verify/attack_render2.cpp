// End-to-end check: with a left-facing guard, does the whole Attack clip stay on the
// LEFT of its feet, i.e. inside AttackArea()? Renders through the real EnemyRenderer.
#include "enemy_system.h"
#include "enemy_renderer.h"
#include "enemy_data.h"
#include "ui_font.h"
#include <raylib.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <vector>
#include <map>

int main() {
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(960, 540, "shield attack facing");
    SetTargetFPS(1000);
    auto* renderer = new EnemyRenderer();
    auto* font = new UiFont();

    struct Sample { float cx; float t; };
    for (int side = 0; side < 2; ++side) {
        EnemySystem sys;
        sys.Clear();
        const unsigned id = sys.Spawn(EnemyKind::Shield, 1500);
        const float playerX = side == 0 ? 1120.0F : 1880.0F;
        const Vector2 player{playerX, GameConfig::kFloorY};
        std::map<int, Sample> samples;
        float facingSeen = 0;
        int windupSeen = 0, strikeSeen = 0;
        for (int step = 0; step < 1200; ++step) {
            sys.Update(1.0F / 60.0F, player);
            const auto& units = sys.Units();
            for (const auto& u : units) {
                if (u.id != id) continue;
                facingSeen = static_cast<float>(u.facing);
                if (u.state == EnemyState::Windup) ++windupSeen;
                if (u.state == EnemyState::Strike) ++strikeSeen;
                if (u.state != EnemyState::Strike && u.state != EnemyState::Windup) continue;
                // Which cell does the renderer actually pick for this unit right now?
                // Reproduce the same row/frame selection as DrawUnit.
                const auto& d = EnemyData(u.kind);
                int row = 0, frame = 0;
                if (u.state == EnemyState::Windup) { row = 2; frame = std::clamp(int((1 - u.timer / d.windup) * 4), 0, 3); }
                if (u.state == EnemyState::Strike) { row = 2; frame = 4 + std::clamp(int(u.animationTime / .22F * 4), 0, 3); }
                BeginDrawing();
                ClearBackground(BLACK);
                renderer->DrawUnit(u, *font);
                EndDrawing();
                Image shot = LoadImageFromScreen();
                Color* px = LoadImageColors(shot);
                double sx = 0; long n = 0; int minx = shot.width, maxx = -1;
                for (int y = 0; y < shot.height; ++y)
                    for (int x = 0; x < shot.width; ++x) {
                        const Color c = px[y * shot.width + x];
                        if (c.r + c.g + c.b > 90) { sx += x; ++n; if (x < minx) minx = x; if (x > maxx) maxx = x; }
                    }
                if (n > 0) {
                    const double cx = sx / n - (minx + maxx) / 2.0;
                    const float rel = static_cast<float>(cx);
                    auto it = samples.find(frame);
                    if (it == samples.end()) samples[frame] = {rel, 1};
                    else { it->second.cx += rel; it->second.t += 1; }
                } else if (samples.find(-1) == samples.end()) {
                    samples[-1] = {0, 0};  // marks "render produced nothing"
                }
                UnloadImageColors(px);
                UnloadImage(shot);
            }
        }
        printf("\n=== player %s (guard facing %s) ===\n", side == 0 ? "LEFT" : "RIGHT",
               facingSeen > 0 ? "+1 right" : "-1 left");
        printf("windup ticks=%d  strike ticks=%d  samples=%d\n", windupSeen, strikeSeen,
               static_cast<int>(samples.size()));
        printf("frame   mean silhouette lean (px, negative = left of feet)\n");
        for (auto& [f, s] : samples)
            printf("  f%-2d   %+7.2f\n", f, s.cx / s.t);
    }

    delete font;
    delete renderer;
    CloseWindow();
    return 0;
}
