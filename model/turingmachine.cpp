#include "turingmachine.hpp"
#include <nlohmann/json.hpp>

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
  for (const auto &t : transitions_) {
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
  for (const auto &t : transitions_) {
    uniqueStates.insert(t.from());
    uniqueStates.insert(t.to());
  }
  for (const auto &st : unconnectedStates_) {
    uniqueStates.insert(st);
  }
  return std::vector<State>(uniqueStates.begin(), uniqueStates.end());
}

std::string core::TuringMachine::nextUniqueStateName() const
{
  int index = 0;
  std::set<std::string> existingNames;
  for (const auto &state : states()) {
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
    [&st](const Transition &t) { return t.from() == st || t.to() == st; }), transitions_.end());
  unconnectedStates_.erase(std::remove(unconnectedStates_.begin(), unconnectedStates_.end(), st), unconnectedStates_.end());
  // TOTHINK: move any unconnected state to unconnectedStates_ after transition(s) removal.
}

bool core::TuringMachine::hasTransitionsFrom(State st) const
{
  for (const auto &t : transitions_) {
    if (t.from() == st) {
      return true;
    }
  }
  return false;
}

std::string core::TuringMachine::toJson() const
{
  using nlohmann::json;

  auto stateTypeToStr = [](State::Type type) {
    switch (type) {
    case State::Type::START: return "START";
    case State::Type::ACCEPT: return "ACCEPT";
    case State::Type::REJECT: return "REJECT";
    case State::Type::NORMAL: return "NORMAL";
    }
    return "NORMAL";
    };

  auto stateToJson = [stateTypeToStr](const State &st) {
    return json{
        {"name", st.name()},
        {"type", stateTypeToStr(st.type())}
    };
    };

  auto dirToStr = [](Tape::Dir dir) {
    return dir == Tape::Dir::LEFT ? "LEFT" : "RIGHT";
    };

  json j;
  j["unconnectedStates"] = json::array();
  for (const auto &st : unconnectedStates_) {
    j["unconnectedStates"].push_back(stateToJson(st));
  }
  j["transitions"] = json::array();
  for (const auto &tr : transitions_) {
    j["transitions"].push_back({
        {"from", stateToJson(tr.from())},
        {"readSymbol", std::string(1, tr.readSymbol())},
        {"to", stateToJson(tr.to())},
        {"writeSymbol", std::string(1, tr.writeSymbol())},
        {"direction", dirToStr(tr.direction())}
      });
  }
  return j.dump();
}

void core::TuringMachine::fromJson(const std::string &jsonStr)
{
  using nlohmann::json;

  auto strToStateType = [](const std::string &s) {
    if (s == "START") return State::Type::START;
    if (s == "ACCEPT") return State::Type::ACCEPT;
    if (s == "REJECT") return State::Type::REJECT;
    return State::Type::NORMAL;
    };

  auto stateFromJson = [strToStateType](const json &j) {
    return State(j.at("name").get<std::string>(), strToStateType(j.at("type").get<std::string>()));
    };

  auto strToDir = [](const std::string &s) {
    return s == "LEFT" ? Tape::Dir::LEFT : Tape::Dir::RIGHT;
    };

  unconnectedStates_.clear();
  transitions_.clear();

  json j = json::parse(jsonStr);

  for (const auto &st : j.at("unconnectedStates")) {
    unconnectedStates_.push_back(stateFromJson(st));
  }
  for (const auto &tr : j.at("transitions")) {
    State from = stateFromJson(tr.at("from"));
    char readSymbol = tr.at("readSymbol").get<std::string>()[0];
    State to = stateFromJson(tr.at("to"));
    char writeSymbol = tr.at("writeSymbol").get<std::string>()[0];
    Tape::Dir dir = strToDir(tr.at("direction").get<std::string>());
    transitions_.emplace_back(from, to, readSymbol, writeSymbol, dir);
  }
}

void core::TuringMachine::addTransition(const Transition &tr)
{
  transitions_.push_back(tr);
}

void core::TuringMachine::removeTransition(const Transition &tr)
{
  transitions_.erase(std::remove(transitions_.begin(), transitions_.end(), tr), transitions_.end());
}
