#ifndef _MANIPULATORS_HPP_
#define _MANIPULATORS_HPP_

#include "defs.hpp"
#include <imgui.h>
#include <memory>

namespace ui {

  class DrawObject;
  class TransitionStyle;

  class Manipulator {
  public:
    Manipulator(ui::DrawObject *o) : dro_(o) {}
    virtual ~Manipulator() = default;
    virtual void draw() = 0;
    virtual bool isActive() const { return active_; }

    virtual void setFirstPos(float x, float y) = 0;
    virtual void setNextPos(float x, float y, const ImVec2 &offset) = 0;
    virtual void setLastPos(float x, float y) = 0;

    void setFirstPos(const ImVec2 &pt) { setFirstPos(pt.x, pt.y); }
    void setNextPos(const ImVec2 &pt, const ImVec2 &offset) { setNextPos(pt.x, pt.y, offset); }
    void setLastPos(const ImVec2 &pt) { setLastPos(pt.x, pt.y); }

    bool handleMouse(const ImGuiIO &io, ImDrawList *drawList);

  protected:
    ui::DrawObject *dro_;
    ImVec2 prevPos_;
    bool active_ = false;
  };


  class StateManipulator : public Manipulator {
  public:
    StateManipulator(ui::DrawObject *p) : Manipulator(p) {}
    void draw() override;

    void setFirstPos(float x, float y) override;
    void setNextPos(float x, float y, const ImVec2 &offset) override;
    void setLastPos(float x, float y) override;
  };


  class TransitionManipulator : public Manipulator {
    struct Impl;
    std::unique_ptr<Impl> imp;
  public:
    TransitionManipulator(ui::DrawObject *p);
    ~TransitionManipulator();
    void draw() override;
    void setFirstPos(float x, float y) override;
    void setNextPos(float x, float y, const ImVec2 &offset) override;
    void setLastPos(float x, float y) override;
  private:
    void handleCurveManipulation(float x, float y, const ImVec2 &offset);
    void handleSelfLoopManipulation(float x, float y, const ImVec2 &offset, ui::TransitionStyle &style);
    void startReconnection(DragState::Mode mode, float x, float y);
    void handleReconnection(DragState::Mode mode, float x, float y);
  };


  class TransitionLabelManipulator : public Manipulator {
  public:
    TransitionLabelManipulator(ui::DrawObject *p) : Manipulator(p) {}

    void draw() override;
    void setFirstPos(float x, float y) override;
    void setNextPos(float x, float y, const ImVec2 &offset) override;
    void setLastPos(float x, float y) override;
  };


  //class SelectionManipulator : public Manipulator {
  //  // Handle selection box, multi-select
  //};

}

#endif // _MANIPULATORS_HPP_