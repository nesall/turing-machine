#ifndef _DRAWOBJECT_HPP_
#define _DRAWOBJECT_HPP_

#include "model/turingmachine.hpp"
#include "defs.hpp"
#include <memory>
#include <optional>
#include <functional>
#include <imgui.h>
#include <nlohmann/json.hpp>


class AppState;

namespace ui {

  class Manipulator;
  class TransitionManipulator;

  struct TransitionStyle {
    float arcHeight = 40.0f;          // Curve intensity
    float lineThickness = 3.0f;       // Line width
    float arrowSize = 10.0f;          // Arrow dimensions
    float selfLoopRadius = 15.0f;     // Self-loop circle radius
    float selfLoopOffset = 20.0f;     // Distance from state center
    ImU32 color = IM_COL32(255, 165, 0, 255);
    ImU32 textColor = IM_COL32(0, 0, 0, 255);
    int transitionIndex = 0;          // For multiple transitions between same states
    std::optional<ImU32> colorHightlight = std::nullopt;

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json &j);
  };

  struct TransitionControlPoints {
    enum PointIndex { START = 0, MID = 1, END = 2 };
    ImVec2 points[3];
    bool isValid = false; // False for degenerate cases
    std::optional<TransitionControlPoints::PointIndex> hitTest(float x, float y, float radius = 6) const {
      for (int i = 0; i <= END; ++i) {
        if (sqrtf((x - points[i].x) * (x - points[i].x) + (y - points[i].y) * (y - points[i].y)) <= radius) {
          return static_cast<TransitionControlPoints::PointIndex>(i);
        }
      }
      return std::nullopt;
    }
  };

  class StateDrawObject;
  class TransitionDrawObject;
  class TransitionLabelDrawObject;

  class DrawObject {
  public:
    DrawObject(AppState *a) : appState_(a) {}
    virtual ~DrawObject() = default;
    virtual void draw(ImDrawList *dr) const = 0;
    virtual bool containsPoint(float x, float y) const { return boundingRect().contains(x, y); }
    virtual utils::Rect boundingRect() const = 0;
    virtual ImVec2 centerPoint() const { return boundingRect().center(); }
    virtual void translate(const ImVec2 &delta) = 0;

    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    // Optional: type checking
    //virtual DrawObjectType getType() const = 0;

    virtual ui::Manipulator *getOrCreateManipulator(bool bCreate) = 0;
    ui::Manipulator *createManipulator() { return getOrCreateManipulator(true); }
    ui::Manipulator *getManipulator() const { return const_cast<DrawObject *>(this)->getOrCreateManipulator(false); }
    void removeManipulator() { manipulator_.reset(); }

    virtual StateDrawObject *asState() { return nullptr; }
    virtual TransitionDrawObject *asTransition() { return nullptr; }
    virtual TransitionLabelDrawObject *asTransitionLabel() { return nullptr; }

    const StateDrawObject *asState() const { return const_cast<DrawObject *>(this)->asState(); }
    const TransitionDrawObject *asTransition() const { return const_cast<DrawObject *>(this)->asTransition(); }
    const TransitionLabelDrawObject *asTransitionLabel() const { return const_cast<DrawObject *>(this)->asTransitionLabel(); }

    template <class T> T *asType() { return nullptr; }
    template <> StateDrawObject *asType() { return asState(); }
    template <> TransitionDrawObject *asType() { return asTransition(); }
    template <> TransitionLabelDrawObject *asType() { return asTransitionLabel(); }
    template <class T> const T *asType() const { return const_cast<DrawObject *>(this)->asType<T>(); }

    AppState *appState() const { return appState_; }

  protected:
    bool visible_ = true;
    AppState *appState_;
    std::unique_ptr<Manipulator> manipulator_;
  };

  class StateDrawObject : public DrawObject {
    core::State state_;
  public:
    StateDrawObject(const core::State &state, AppState *app);
    const core::State &getState() const { return state_; }

    void draw(ImDrawList *dr) const override;
    utils::Rect boundingRect() const override;
    void translate(const ImVec2 &delta) override;
    ui::Manipulator *getOrCreateManipulator(bool bCreate) override;
    StateDrawObject *asState() override { return this; }

  public:
    static void drawState(const core::State &state, ImVec2 pos, ImU32 clr, bool temp = false);
    static float radius();
  };

  class TransitionDrawObject : public DrawObject {
    friend class TransitionManipulator;

    core::Transition transition_;
    mutable TransitionControlPoints controlPoints_;
    TransitionStyle style_;
    std::vector<TransitionLabelDrawObject *> labels_; // weak references
  public:
    TransitionDrawObject(const core::Transition &trans, AppState *app);
    const core::Transition &getTransition() const { return transition_; }
    core::Transition &getTransition() { return transition_; }
    TransitionControlPoints controlPoints() const { return controlPoints_; }
    TransitionStyle transitionStyle() const { return style_; }
    void setTransitionStyle(const TransitionStyle &style) { style_ = style; }

    void draw(ImDrawList *dr) const override;
    bool containsPoint(float x, float y) const override;
    utils::Rect boundingRect() const override;
    void translate(const ImVec2 &delta) override;
    ui::Manipulator *getOrCreateManipulator(bool bCreate) override;
    TransitionDrawObject *asTransition() override { return this; }

    void addLabel(TransitionLabelDrawObject *label);
    void removeLabel(TransitionLabelDrawObject *label);
    const std::vector<TransitionLabelDrawObject *> &getLabels() const { return labels_; }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json &json);

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
    mutable utils::Rect rect_;
  public:
    TransitionLabelDrawObject(const ui::TransitionDrawObject *tdo, AppState *app);
    ImVec2 getAutoPosition() const;
    ImVec2 getFinalPosition() const;
    void setManualOffset(const ImVec2 &offset);
    void resetToAutoPosition();

    const ui::TransitionDrawObject *transitionDrawObject() const { return tdo_; }

    void draw(ImDrawList *dr) const override;
    utils::Rect boundingRect() const override;
    void translate(const ImVec2 &delta) override;
    ui::Manipulator *getOrCreateManipulator(bool bCreate) override;
    TransitionLabelDrawObject *asTransitionLabel() override { return this; }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json &json);

  };


  class TransitionLabelEditor {
  private:
    bool showDialog_ = false;
    std::unique_ptr<core::Transition> transition_;
    char readSymbol_[2] = "";
    char writeSymbol_[2] = "";
    int direction_ = 0;
    std::function<void(const core::Transition &)> onCommit_;
  public:
    void openEditor(const core::Transition &trans, std::function<void(const core::Transition &)> f);
    void render();
  private:
    void applyChanges();
  };

} // namespace ui

#endif // _DRAWOBJECT_HPP_