#include "raylib.h"
#include <iostream>
int main() {

    InitWindow(800, 450, "Raylib OOP");
    SetTargetFPS(60);


    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
