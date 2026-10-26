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

            game.RenderFrame();
        }
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
