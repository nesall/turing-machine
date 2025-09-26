#ifndef _DEFS_HPP_
#define _DEFS_HPP_

#include <imgui.h>
#include <format>
#include <string>
#include <iterator>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

template <typename T> T *UNCONST(const T *v) { return const_cast<T *>(v); }
template <typename T> T &UNCONST(const T &v) { return const_cast<T &>(v); }

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

namespace utils {

  struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(float px, float py) const {
      return (px >= x && px <= x + w && py >= y && py <= y + h);
    }
    bool intersects(const Rect &r) const {
      return !(r.x > x + w || r.x + r.w < x || r.y > y + h || r.y + r.h < y);
    }
    ImVec2 center() const {
      return ImVec2(x + w * 0.5f, y + h * 0.5f);
    }
  };

  // write a function to join a container into a string
  template <typename Container>
  std::string join(const Container &c, const std::string &sep = ", ") {
    std::string result;
    for (auto it = c.begin(); it != c.end(); ++it) {
      result += *it;
      if (std::next(it) != c.end()) {
        result += sep;
      }
    }
    return result;
  }

  // dynamically calculate a lighter color based on char value
  inline ImU32 colorFromChar(char c) {
    unsigned char r = (c * 123) % 92 + 164;
    unsigned char g = (c * 321) % 92 + 164;
    unsigned char b = (c * 213) % 92 + 164;
    return IM_COL32(r, g, b, 255);
  }
}

namespace Colors {
  const auto orange{ IM_COL32(255, 165, 0, 255) };
  const auto maroon{ IM_COL32(128, 0, 0, 255) };
  const auto magenta{ IM_COL32(255, 0, 255, 255) };
  const auto green{ IM_COL32(0, 255, 0, 255) };
  const auto red{ IM_COL32(255, 0, 0, 255) };
  const auto blue{ IM_COL32(0, 0, 255, 255) };
  const auto gray{ IM_COL32(128, 128, 128, 255) };
  const auto white{ IM_COL32_WHITE };
  const auto black{ IM_COL32_BLACK };

  const auto lightGray{ IM_COL32(192, 192, 192, 255) };
  const auto darkGreen{ IM_COL32(0, 164, 0, 255) };
  const auto darkRed{ IM_COL32(192, 0, 0, 255) };
  const auto royalBlue{ IM_COL32(65, 105, 225, 255) };
  const auto dodgerBlue{ IM_COL32(30, 144, 255, 255) };

  // Standard web colors
  const auto yellow{ IM_COL32(255, 255, 0, 255) };
  const auto cyan{ IM_COL32(0, 255, 255, 255) };
  const auto purple{ IM_COL32(128, 0, 128, 255) };
  const auto pink{ IM_COL32(255, 192, 203, 255) };
  const auto brown{ IM_COL32(165, 42, 42, 255) };
  const auto navy{ IM_COL32(0, 0, 128, 255) };
  const auto teal{ IM_COL32(0, 128, 128, 255) };
  const auto olive{ IM_COL32(128, 128, 0, 255) };
  const auto gold{ IM_COL32(255, 215, 0, 255) };
  const auto silver{ IM_COL32(192, 192, 192, 255) };
  const auto lime{ IM_COL32(50, 205, 50, 255) };
  const auto indigo{ IM_COL32(75, 0, 130, 255) };
  const auto violet{ IM_COL32(238, 130, 238, 255) };
  const auto coral{ IM_COL32(255, 127, 80, 255) };
  const auto salmon{ IM_COL32(250, 128, 114, 255) };
  const auto turquoise{ IM_COL32(64, 224, 208, 255) };
  const auto skyBlue{ IM_COL32(135, 206, 235, 255) };
  const auto khaki{ IM_COL32(240, 230, 140, 255) };

  // Pastel / Soft UI colors
  const auto pastelRed{ IM_COL32(255, 179, 186, 255) };
  const auto pastelOrange{ IM_COL32(255, 223, 186, 255) };
  const auto pastelYellow{ IM_COL32(255, 255, 186, 255) };
  const auto pastelGreen{ IM_COL32(186, 255, 201, 255) };
  const auto pastelCyan{ IM_COL32(186, 255, 255, 255) };
  const auto pastelBlue{ IM_COL32(186, 225, 255, 255) };
  const auto pastelPurple{ IM_COL32(218, 186, 255, 255) };
  const auto pastelPink{ IM_COL32(255, 186, 232, 255) };
  const auto pastelGray{ IM_COL32(220, 220, 220, 255) };
}


#endif // !_DEFS_HPP_
