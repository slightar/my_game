#include "game.h"

#include "game_types.h"
#include "platform_input.h"
#include "raylib.h"

#include <algorithm>

int main() {
    SetConfigFlags(FLAG_FULLSCREEN_MODE | FLAG_VSYNC_HINT);
    InitWindow(GameConfig::kScreenWidth, GameConfig::kScreenHeight, "arknights-go");
    PrepareGameWindowInput(GetWindowHandle());
    InitAudioDevice();
    SetTargetFPS(60);

    {
        Game game;
        while (!WindowShouldClose()) {
            const float deltaTime = std::min(GetFrameTime(), 1.0F / 30.0F);
            game.Update(deltaTime);

            BeginDrawing();
            game.Draw();
            EndDrawing();
        }
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
