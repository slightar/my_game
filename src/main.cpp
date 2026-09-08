//
// Created by 34844 on 2026/9/8.
//
#include "raylib.h"
#include<iostream>
int main() {
    InitWindow(800, 600, "arknights-go");      // 创建 800x600 窗口
    SetTargetFPS(60);                          // 限制帧率为 60

    while (!WindowShouldClose()) {             // 主循环：窗口不关闭就一直运行
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Hello, my game!", 300, 280, 24, DARKGRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
