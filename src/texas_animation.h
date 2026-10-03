#pragma once

// Matches texas_battle.png, baked from the client's front char_102_texas rig.
// A full Start/Loop/End swing fits one existing normal-attack interval.
namespace TexasBattleAnimation {
inline constexpr int Columns = 24;
inline constexpr int Rows = 3;
inline constexpr float Cell = 256.0F;
inline constexpr float AnchorX = 128.0F;
inline constexpr float AnchorY = 224.0F;
inline constexpr float IdleHeight = 164.0F;
inline constexpr float VisibleHeight = 94.23077F;
inline constexpr float GroundInset = 3.23077F;
inline constexpr float IdleDuration = 2.0F;
inline constexpr float AttackDuration = 0.32F;
inline constexpr float DefeatDuration = 1.0F;
}
