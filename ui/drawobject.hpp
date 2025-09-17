#ifndef _DRAWOBJECT_HPP_
#define _DRAWOBJECT_HPP_

#include "model/turingmachine.hpp"
#include <memory>
#include <optional>
#include <imgui.h>



class AppState;

namespace ui {

  class Manipulator;

  struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(float px, float py) const {
      return (px >= x && px <= x + w && py >= y && py <= y + h);
    }
    ImVec2 center() const {
      return ImVec2(x + w * 0.5f, y + h * 0.5f);
    }
  };

  struct TransitionStyle {
    float arcHeight = 40.0f;          // Curve intensity
    float lineThickness = 3.0f;       // Line width
    float arrowSize = 10.0f;          // Arrow dimensions
    float selfLoopRadius = 15.0f;     // Self-loop circle radius
    float selfLoopOffset = 20.0f;     // Distance from state center
    ImU32 color = IM_COL32(255, 165, 0, 255);
    ImU32 textColor = IM_COL32(0, 0, 0, 255);
    int transitionIndex = 0;          // For multiple transitions between same states
  };

  struct TransitionControlPoints {
    ImVec2 startPoint;    // Where arc begins (edge of from-state)
    ImVec2 midPoint;      // Where label is (bezier midpoint)
    ImVec2 endPoint;      // Where arc ends (edge of to-state)
    bool isValid = false; // False for degenerate cases
  };

#if 0
  class DrawObject {
    struct Impl;
    std::unique_ptr<Impl> imp;
  public:
    DrawObject(const core::State &state, AppState *app);
    DrawObject(const core::Transition &tr, AppState *app);
    ~DrawObject();
    
    DrawObject(const DrawObject &) = delete;
    DrawObject &operator = (const DrawObject &) = delete;

    ui::Manipulator *getOrCreateManipulator(bool bCreate);
    ui::Manipulator *createManipulator() { return getOrCreateManipulator(true); }
    ui::Manipulator *getManipulator() const { return const_cast<DrawObject *>(this)->getOrCreateManipulator(false); }
    void removeManipulator();
    ui::Rect boundingRect() const;
    ImVec2 centerPoint() const;
    bool containsPoint(float x, float y) const;
    void draw() const;
    void translate(const ImVec2 &d);
    bool isState() const;
    std::optional<core::State> getState() const;
    std::optional<core::Transition> getTransition() const;

  public:
    struct TransitionStyle {
      float arcHeight = 40.0f;          // Curve intensity
      float lineThickness = 3.0f;       // Line width
      float arrowSize = 10.0f;          // Arrow dimensions
      float selfLoopRadius = 15.0f;     // Self-loop circle radius
      float selfLoopOffset = 20.0f;     // Distance from state center
      ImU32 color = IM_COL32(255, 165, 0, 255);
      ImU32 textColor = IM_COL32(0, 0, 0, 255);
      int transitionIndex = 0;          // For multiple transitions between same states
    };

    struct TransitionControlPoints {
      ImVec2 startPoint;    // Where arc begins (edge of from-state)
      ImVec2 midPoint;      // Where label is (bezier midpoint)
      ImVec2 endPoint;      // Where arc ends (edge of to-state)
      bool isValid = false; // False for degenerate cases
    };

    static void drawState(const core::State &state, ImVec2 pos, ImU32 clr, bool temp = false);
    static TransitionControlPoints drawTransition(const core::Transition &trans, ImVec2 posFrom, ImVec2 posTo, const TransitionStyle &style = TransitionStyle{});

  private:
    static TransitionControlPoints drawSelfLoop(const core::Transition &trans, ImVec2 pos, const TransitionStyle &style);
    static void drawArrowhead(ImVec2 tipPos, ImVec2 controlPos, const TransitionStyle &style);
    static void drawArrowheadAtPoint(ImVec2 tipPos, ImVec2 direction, const TransitionStyle &style);
    static void drawTransitionLabel(const core::Transition &trans, ImVec2 start, ImVec2 control, ImVec2 end, const TransitionStyle &style);
  };
#endif

  class StateDrawObject;
  class TransitionDrawObject;
  class TransitionLabelDrawObject;

  class DrawObject {
  public:
    DrawObject(AppState *a) : appState_(a) {}
    virtual ~DrawObject() = default;
    virtual void draw() const = 0;
    virtual bool containsPoint(float x, float y) const { return boundingRect().contains(x, y); }
    virtual ui::Rect boundingRect() const = 0;
    virtual ImVec2 centerPoint() const { return boundingRect().center(); }
    virtual void translate(const ImVec2 &delta) = 0;

    // Optional: type checking
    //virtual DrawObjectType getType() const = 0;

    virtual ui::Manipulator *getOrCreateManipulator(bool bCreate) = 0;
    ui::Manipulator *createManipulator() { return getOrCreateManipulator(true); }
    ui::Manipulator *getManipulator() const { return const_cast<DrawObject *>(this)->getOrCreateManipulator(false); }
    void removeManipulator() { manipulator_.reset(); }

    virtual StateDrawObject *asState() { return nullptr; }
    virtual TransitionDrawObject *asTransition() { return nullptr; }
    virtual TransitionLabelDrawObject *asTransitionLabel() { return nullptr; }

  protected:
    AppState *appState_;
    std::unique_ptr<Manipulator> manipulator_;
  };

  class StateDrawObject : public DrawObject {
    core::State state_;
  public:
    StateDrawObject(const core::State &state, AppState *app);
    const core::State &getState() const { return state_; }

    void draw() const override;
    ui::Rect boundingRect() const override;
    void translate(const ImVec2 &delta) override;
    ui::Manipulator *getOrCreateManipulator(bool bCreate) override;
    StateDrawObject *asState() override { return this; }

  public:
    static void drawState(const core::State &state, ImVec2 pos, ImU32 clr, bool temp = false);
  };

  class TransitionDrawObject : public DrawObject {
    core::Transition transition_;
    mutable TransitionControlPoints controlPoints_;
    TransitionStyle style_;
  public:
    TransitionDrawObject(const core::Transition &trans, AppState *app);
    const core::Transition &getTransition() const { return transition_; }
    TransitionControlPoints controlPoints() const { return controlPoints_; }
    TransitionStyle transitionStyle() const { return style_; }
    void setTransitionStyle(const TransitionStyle &style) { style_ = style; }

    void draw() const override;
    bool containsPoint(float x, float y) const override;
    ui::Rect boundingRect() const override;
    void translate(const ImVec2 &delta) override;
    ui::Manipulator *getOrCreateManipulator(bool bCreate) override;
    TransitionDrawObject *asTransition() override { return this; }

  public:
    static TransitionControlPoints drawTransition(const core::Transition &trans, ImVec2 posFrom, ImVec2 posTo, const TransitionStyle &style = TransitionStyle{});

  private:
    static TransitionControlPoints drawSelfLoop(const core::Transition &trans, ImVec2 pos, const TransitionStyle &style);
    static void drawArrowhead(ImVec2 tipPos, ImVec2 controlPos, const TransitionStyle &style);
    static void drawArrowheadAtPoint(ImVec2 tipPos, ImVec2 direction, const TransitionStyle &style);
    //static void drawTransitionLabel(const core::Transition &trans, ImVec2 start, ImVec2 control, ImVec2 end, const TransitionStyle &style);
  };

  class TransitionLabelDrawObject : public DrawObject {
    const ui::TransitionDrawObject *tdo_;
    ImVec2 manualOffset_{ 0, 0 };
    bool hasManualPosition_ = false;
    mutable ui::Rect rect_;
  public:
    TransitionLabelDrawObject(const ui::TransitionDrawObject *tdo, AppState *app);
    ImVec2 getAutoPosition() const;
    ImVec2 getFinalPosition() const;
    void setManualOffset(const ImVec2 &offset);
    void resetToAutoPosition();

    void draw() const override;
    ui::Rect boundingRect() const override { return rect_; }
    void translate(const ImVec2 &delta) override;
    ui::Manipulator *getOrCreateManipulator(bool bCreate) override;
    TransitionLabelDrawObject *asTransitionLabel() override { return this; }
  };

} // namespace ui

#endif // _DRAWOBJECT_HPP_