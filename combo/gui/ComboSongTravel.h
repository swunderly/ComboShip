#pragma once
namespace ComboRando {
// Each game now owns its warp dialogue through its native message renderer.
// Retained as a no-op for the combined menu ABI; never capture game input here.
inline void DrawSongTravel() {}
}
