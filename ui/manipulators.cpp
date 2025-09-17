#include "ui/manipulators.hpp"
#include "app.hpp"
#include "ui/drawobject.hpp"
#include <imgui.h>


void ui::StateManipulator::draw()
{
  ImDrawList *dr = ImGui::GetWindowDrawList();
  auto rc = dro_->boundingRect();
  dr->AddRect(ImVec2(rc.x, rc.y), ImVec2(rc.x + rc.w, rc.y + rc.h), IM_COL32(255, 0, 0, 255));
}

void ui::StateManipulator::setFirstPos(float x, float y)
{
}

void ui::StateManipulator::setNextPos(float x, float y, const ImVec2 &offset)
{
  dro_->translate(offset);
}

void ui::StateManipulator::setLastPos(float x, float y)
{
}



void ui::TransitionManipulator::draw()
{
  ImDrawList *dr = ImGui::GetWindowDrawList();
  auto rc = dro_->boundingRect();
  dr->AddRect(ImVec2(rc.x, rc.y), ImVec2(rc.x + rc.w, rc.y + rc.h), IM_COL32(255, 0, 0, 255));
}

void ui::TransitionManipulator::setFirstPos(float x, float y)
{
}

void ui::TransitionManipulator::setNextPos(float x, float y, const ImVec2 &offset)
{
}

void ui::TransitionManipulator::setLastPos(float x, float y)
{
}



void ui::TransitionLabelManipulator::draw()
{
  ImDrawList *dr = ImGui::GetWindowDrawList();
  auto rc = dro_->boundingRect();
  dr->AddRect(ImVec2(rc.x, rc.y), ImVec2(rc.x + rc.w, rc.y + rc.h), IM_COL32(255, 0, 0, 255));
}

void ui::TransitionLabelManipulator::setFirstPos(float x, float y)
{
}

void ui::TransitionLabelManipulator::setNextPos(float x, float y, const ImVec2 &offset)
{
  dro_->translate(offset);
}

void ui::TransitionLabelManipulator::setLastPos(float x, float y)
{
}
