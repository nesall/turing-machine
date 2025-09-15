#include "turingmachine.hpp"
#include <memory>

core::TuringMachine::TuringMachine()
{
  // Example transitions for a simple Turing machine
  //State q0("q0", State::Type::START);
  //State q1("q1");
  //State qAccept("qAccept", State::Type::ACCEPT);
  //State qReject("qReject", State::Type::REJECT);
  //addTransition(q0, '0', q1, '1', Tape::Dir::RIGHT);
  //addTransition(q1, '1', qAccept, '1', Tape::Dir::LEFT);
  //addTransition(q1, Alphabet::Blank, qReject, '0', Tape::Dir::RIGHT);
  //currentState_ = q0;
}

void core::TuringMachine::step()
{
  char currentSymbol = tape_.read();
  bool found = false;
  for (const auto& t : transitions_) {
    if (t.from() == currentState_ && t.readSymbol() == currentSymbol) {
      currentState_ = t.to();
      tape_.write(t.writeSymbol());
      tape_.move(t.direction());
      found = true;
      break;
    }
  }
  if (!found) {
    // No valid transition, halt the machine (could also set to a reject state)
    currentState_ = State("HALT", State::Type::REJECT);
  }
}

bool core::TuringMachine::isAccepting() const
{
  return currentState_.isAccept();
}

bool core::TuringMachine::isRejecting() const
{
  return currentState_.isReject();
}

std::vector<core::State> core::TuringMachine::states() const
{
  std::set<State> uniqueStates;
  for (const auto& t : transitions_) {
    uniqueStates.insert(t.from());
    uniqueStates.insert(t.to());
  }
  for (const auto& st : unconnectedStates_) {
    uniqueStates.insert(st);
  }
  return std::vector<State>(uniqueStates.begin(), uniqueStates.end());
}

std::string core::TuringMachine::nextUniqueStateName() const
{
  int index = 0;
  std::set<std::string> existingNames;
  for (const auto& state : states()) {
    existingNames.insert(state.name());
  }
  std::string name;
  do {
    name = "q" + std::to_string(index++);
  } while (existingNames.count(name) > 0);
  return name;
}

void core::TuringMachine::addUnconnectedState(const State &st)
{
  unconnectedStates_.push_back(st);
}

void core::TuringMachine::removeState(State st)
{
  transitions_.erase(std::remove_if(transitions_.begin(), transitions_.end(),
    [&st](const Transition& t) { return t.from() == st || t.to() == st; }), transitions_.end());
  unconnectedStates_.erase(std::remove(unconnectedStates_.begin(), unconnectedStates_.end(), st), unconnectedStates_.end());
  // TOTHINK: move any unconnected state to unconnectedStates_ after transition(s) removal.
}

bool core::TuringMachine::hasTransitionsFrom(State st) const
{
  for (const auto& t : transitions_) {
    if (t.from() == st) {
      return true;
    }
  }
  return false;
}

void core::TuringMachine::addTransition(const State &from, char readSymbol, const State &to, char write, Tape::Dir move)
{
  transitions_.push_back({from, readSymbol, to, write, move});
}
