#ifndef _MANIPULATORS_HPP_
#define _MANIPULATORS_HPP_

#include "model/turingmachine.hpp"
#include <imgui.h>

namespace ui {

  class Manipulator {
  public:
    virtual ~Manipulator() = default;
    virtual bool handleMouse(const ImGuiIO &io, ImDrawList *drawList) = 0;
    virtual void draw(ImDrawList *drawList) = 0;
    virtual bool isActive() const { return active_; }
  protected:
    bool active_ = false;
  };


  class StateManipulator : public Manipulator {
    core::State *state_;
    ImVec2 dragOffset_;
  public:
    StateManipulator(core::State *state) : state_(state) {}
    bool handleMouse(const ImGuiIO &io, ImDrawList *drawList) override;
    void draw(ImDrawList *drawList) override;
  };


  class TransitionManipulator : public Manipulator {
    // Handle arc creation/editing
  };


  class SelectionManipulator : public Manipulator {
    // Handle selection box, multi-select
  };

}

#endif // _MANIPULATORS_HPP_