#ifndef _DEFS_HPP_
#define _DEFS_HPP_

#include <imgui.h>
#include <format>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline std::string nextRandomId() {
  static int counter = 0;
  return std::format("_random_q{}", counter++);
}

inline ImVec2 operator -(const ImVec2 &a, const ImVec2 &b) {
  return ImVec2(a.x - b.x, a.y - b.y);
}

inline ImVec2 operator +(const ImVec2 &a, const ImVec2 &b) {
  return ImVec2(a.x + b.x, a.y + b.y);
}

inline ImVec2 &operator+=(ImVec2 &a, const ImVec2 &b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}

inline ImVec2 posOffset(const ImVec2 &pt, float x = -5, float y = -1) { return { pt.x - x, pt.y - y }; }

struct DragState {
  enum class Mode { NONE, TRANSITION_CONNECT_START, TRANSITION_CONNECT_END };
  Mode mode = Mode::NONE;
  bool isTransitionConnecting() const {
    return mode == DragState::Mode::TRANSITION_CONNECT_START || mode == DragState::Mode::TRANSITION_CONNECT_END;
  }
  bool isDragging = false;
  ImVec2 dragStartPos;
};


namespace Colors {
  const auto orange{ IM_COL32(255, 165, 0, 255) };
  const auto maroon{ IM_COL32(128, 0, 0, 255) };
  const auto magenta{ IM_COL32(255, 0, 255, 255) };
  const auto green{ IM_COL32(0, 255, 0, 255) };
  const auto red{ IM_COL32(255, 0, 0, 255) };
  const auto blue{ IM_COL32(0, 0, 255, 255) };
  const auto white{ IM_COL32_WHITE };
  const auto black{ IM_COL32_BLACK };

}

#endif // !_DEFS_HPP_
