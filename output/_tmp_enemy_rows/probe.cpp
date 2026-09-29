// Standalone QA probe: renders the shield soldier through the real game renderer.
#include "raylib.h"
#include "enemy_renderer.h"
#include "enemy_system.h"
#include "enemy_data.h"
#include "ui_font.h"

#include <cstdio>

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1180, 1560, "shield probe");
    if (!IsWindowReady()) {
        std::printf("window not ready\n");
        return 1;
    }

    UiFont font;
    EnemyRenderer renderer;

    struct RowSpec {
        const char* label;
        EnemyState state;
        float timer;
        bool moving;
        bool turning;
    };
    const RowSpec rows[] = {
        {"IDLE", EnemyState::Approach, 3.00F, false, false},
        {"WINDUP", EnemyState::Windup, .47F, false, false},
        {"STRIKE", EnemyState::Strike, .12F, false, false},
        {"RECOVER", EnemyState::Recover, .60F, false, false},
    };

    const Color paper{235, 238, 236, 255};
    const Color ink{20, 23, 27, 255};

    RenderTexture2D target = LoadRenderTexture(1180, 1560);
    BeginTextureMode(target);
    ClearBackground(paper);

    font.Draw("大盾兵", 24, 16, 24, ink);

    float feetY = 300.0F;
    for (const auto& row : rows) {
        for (int f = 0; f < 2; ++f) {
            const float cx = f == 0 ? 320.0F : 860.0F;
            DrawLineEx({cx - 170, feetY}, {cx + 170, feetY}, 2, Fade(ink, .30F));
            DrawLineEx({cx, feetY - 260}, {cx, feetY}, 1, Fade(RED, .45F));

            EnemyUnit u;
            u.kind = EnemyKind::Shield;
            u.health = EnemyData(EnemyKind::Shield).health;
            u.feet = {cx, feetY};
            u.facing = f == 0 ? -1 : 1;
            u.state = row.state;
            u.timer = row.timer;
            u.moving = row.moving;
            u.turning = row.turning;
            u.animationTime = .30F;
            renderer.DrawUnit(u, font);

            font.Draw(f == 0 ? "<<< facing -1 (raw art)" : "facing +1 (mirrored) >>>",
                      cx - 100, feetY + 16, 20, RED);
        }
        font.Draw(row.label, 24, feetY - 250, 24, ink);
        feetY += 320.0F;
    }

    EndTextureMode();

    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    ExportImage(image, "shield-probe.png");
    UnloadImage(image);
    UnloadRenderTexture(target);
    CloseWindow();
    std::printf("wrote shield-probe.png\n");
    return 0;
}
