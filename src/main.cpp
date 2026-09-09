#include "game.h"

#include "game_types.h"
#include "platform_input.h"
#include "raylib.h"

#include <algorithm>

int main() {
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_VSYNC_HINT |
                   FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "arknights-go");
    SetExitKey(KEY_NULL);
    PrepareGameWindowInput(GetWindowHandle());
    InitAudioDevice();
    SetTargetFPS(60);

    {
        Game game;
        while (!WindowShouldClose() && !game.ShouldQuit()) {
            const float deltaTime = std::min(GetFrameTime(), 1.0F / 30.0F);
            game.Update(deltaTime);

            const float displayScale = std::min(
                static_cast<float>(GetScreenWidth()) /
                    static_cast<float>(GameConfig::kScreenWidth),
                static_cast<float>(GetScreenHeight()) /
                    static_cast<float>(GameConfig::kScreenHeight));
            const Vector2 displayOffset{
                (static_cast<float>(GetScreenWidth()) -
                 static_cast<float>(GameConfig::kScreenWidth) * displayScale) /
                    2.0F,
                (static_cast<float>(GetScreenHeight()) -
                 static_cast<float>(GameConfig::kScreenHeight) * displayScale) /
                    2.0F};
            const Camera2D logicalCamera{displayOffset, {}, 0.0F, displayScale};

            BeginDrawing();
            ClearBackground(BLACK);
            BeginMode2D(logicalCamera);
            game.Draw();
            EndMode2D();
            EndDrawing();
        }
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
