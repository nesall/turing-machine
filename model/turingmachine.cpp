#include "turingmachine.hpp"
#include <nlohmann/json.hpp>
#include <format>


void core::Tape::move(Dir dir)
{
  switch (dir) {
  case Dir::LEFT: moveLeft(); break;
  case Dir::RIGHT: moveRight(); break;
  case Dir::STAY: break;
  }
}

void core::Tape::writeAt(int index, char c)
{
  if (cells_[index] != c) alphabet_.clear();
  else if (c != Tape::Blank) alphabet_.insert(c);
  cells_[index] = c;
}

std::set<char> core::Tape::alphabet() const
{
  if (alphabet_.empty()) {
    for (const auto &p : cells_)
      if (p.second != Tape::Blank) alphabet_.insert(p.second);
  }
  return alphabet_;
}

nlohmann::json core::Tape::toJson() const
{
  using nlohmann::json;
  json j;
  j["headPosition"] = headPosition_;
  j["cells"] = json::array();
  for (const auto &p : cells_) {
    if (p.second != Tape::Blank) {
      j["cells"].push_back({ {"index", p.first}, {"symbol", std::string(1, p.second)} });
    }
  }
  return j;
}

void core::Tape::fromJson(const nlohmann::json &j)
{
  headPosition_ = j.value("headPosition", 0);
  cells_.clear();
  alphabet_.clear();
  for (const auto &item : j["cells"]) {
    int index = item.value("index", 0);
    std::string symbolStr = item.value("symbol", "");
    char symbol = symbolStr.empty() ? Tape::Blank : symbolStr[0];
    writeAt(index, symbol);
  }
}

size_t core::Tape::getNonBlankCellCount() const
{
  size_t count = 0;
  for (const auto &[pos, symbol] : cells_) {
    if (symbol != Blank) {
      count++;
    }
  }
  return count;
}

std::pair<int, int> core::Tape::getUsedRange() const
{
  if (cells_.empty()) return { 0, 0 };
  auto [minIt, maxIt] = std::minmax_element(cells_.begin(), cells_.end(),
    [](const auto &a, const auto &b) { return a.first < b.first; });
  return { minIt->first, maxIt->first };
}


//------------------------------------------------------------------------------------------


void core::State::setName(const std::string &name)
{
  name_ = name;
  // update transitions as well
  // This is handled in TuringMachine::updateState to avoid complexity here

}

void core::State::setType(Type type)
{
  type_ = type;
}


//------------------------------------------------------------------------------------------


std::string core::dirToStr(core::Tape::Dir d)
{
  switch (d) {
  case core::Tape::Dir::LEFT: return "<=";
  case core::Tape::Dir::RIGHT: return "=>";
  default: return "=";
  }
}

std::string core::executionStateToStr(ExecutionState s)
{
  switch (s) {
  case ExecutionState::STOPPED: return "STOPPED";
  case ExecutionState::RUNNING: return "RUNNING";
  case ExecutionState::PAUSED: return "PAUSED";
  case ExecutionState::STEP_MODE: return "STEP_MODE";
  case ExecutionState::FINISHED: return "FINISHED";
  case ExecutionState::ERROR: return "ERROR";
  }
  return "UNKNOWN";
}


//------------------------------------------------------------------------------------------


std::string core::Transition::uniqueKey() const
{
  auto key = from().name() + "_" + std::string(1, readSymbol()) +
    "_" + to().name() + "_" + std::string(1, writeSymbol()) +
    "_" + dirToStr(direction());
  //std::hash<std::string> hasher;
  //return std::to_string(hasher(key));
  return key;
}


//------------------------------------------------------------------------------------------


core::TuringMachine::TuringMachine()
{
  // Example transitions for a simple Turing machine
  //State q0("q0", State::Type::START);
  //State q1("q1");
  //State qAccept("qAccept", State::Type::ACCEPT);
  //State qReject("qReject", State::Type::REJECT);
  //addTransition(q0, '0', q1, '1', Tape::Dir::RIGHT);
  //addTransition(q1, '1', qAccept, '1', Tape::Dir::LEFT);
  //addTransition(q1, Tape::Blank, qReject, '0', Tape::Dir::RIGHT);
  //currentState_ = q0;
}

void core::TuringMachine::step()
{
  if (!tapeBackup_.has_value())
    tapeBackup_ = tape_;
  char currentSymbol = tape_.read();
  bool found = false;
  for (const auto &t : transitions_) {
    if (t.from() == currentState_ && t.readSymbol() == currentSymbol) {
      currentState_ = t.to();
      lastExecutedTransition_ = t.uniqueKey();
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

void core::TuringMachine::reset()
{
  for (const auto &st : states()) {
    if (st.isStart()) {
      currentState_ = st;
      break;
    }
  }
  if (tapeBackup_.has_value())
    tape_ = tapeBackup_.value();
  tapeBackup_.reset();
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
  if (st.isStart()) currentState_ = st;
}

void core::TuringMachine::removeState(State st)
{
  transitions_.erase(std::remove_if(transitions_.begin(), transitions_.end(),
    [&st](const Transition &t) { return t.from() == st || t.to() == st; }), transitions_.end());
  unconnectedStates_.erase(std::remove(unconnectedStates_.begin(), unconnectedStates_.end(), st), unconnectedStates_.end());
  // TOTHINK: move any unconnected state to unconnectedStates_ after transition(s) removal.
}

bool core::TuringMachine::updateState(const State &o, const State &n)
{
  for (auto &t : transitions_) {
    if (t.from() == o) t.setFrom(n);
    if (t.to() == o) t.setTo(n);
  }
  for (auto &st : unconnectedStates_) {
    if (st == o) {
      st = n;
      return true;
    }
  }
  return true;
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

nlohmann::json core::TuringMachine::toJson() const
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
    switch (dir) {
    case core::Tape::Dir::LEFT: return "LEFT";
    case core::Tape::Dir::RIGHT: return "RIGHT";
    default: return "STAY";
    }
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
  return j;
}

void core::TuringMachine::fromJson(const nlohmann::json &j)
{
  auto strToStateType = [](const std::string &s) {
    if (s == "START") return State::Type::START;
    if (s == "ACCEPT") return State::Type::ACCEPT;
    if (s == "REJECT") return State::Type::REJECT;
    return State::Type::NORMAL;
    };
  auto stateFromJson = [strToStateType](const nlohmann::json &j) {
    return State(j.at("name").get<std::string>(), strToStateType(j.at("type").get<std::string>()));
    };
  auto strToDir = [](const std::string &s) {
    if (s == "LEFT") return Tape::Dir::LEFT;
    if (s == "RIGHT") return Tape::Dir::RIGHT;
    return Tape::Dir::STAY;
    };
  unconnectedStates_.clear();
  transitions_.clear();
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

void core::TuringMachine::updateTransition(const Transition &o, const Transition &n)
{
  auto it = std::find(transitions_.begin(), transitions_.end(), o);
  if (it != transitions_.end()) {
    *it = n;
  }
}


//------------------------------------------------------------------------------------------


void core::MachineExecutor::start(core::TuringMachine &tm)
{
  if (validateMachine(tm)) {
    if (state_ == ExecutionState::STOPPED) {
      stepCount_ = 0;
      executionStartTime_ = std::chrono::steady_clock::now();
      totalExecutionTime_ = {};
      resetSpaceTracking();
    } else if (state_ == ExecutionState::PAUSED) {
      //executionStartTime_ = std::chrono::steady_clock::now();
    }
    state_ = ExecutionState::RUNNING;
    lastStepTime_ = std::chrono::steady_clock::now();
  } else {
    state_ = ExecutionState::ERROR;
  }
}

void core::MachineExecutor::pause()
{
  //if (state_ == ExecutionState::RUNNING) {
  //  auto now = std::chrono::steady_clock::now();
  //  totalExecutionTime_ += std::chrono::duration_cast<std::chrono::milliseconds>(now - executionStartTime_);
  //}
  state_ = ExecutionState::PAUSED;
}

void core::MachineExecutor::stop(core::TuringMachine &tm)
{
  state_ = ExecutionState::STOPPED;
  resetMachine(tm);
}

void core::MachineExecutor::stepOnce(core::TuringMachine &tm)
{
  state_ = ExecutionState::STEP_MODE;
  if (canStep(tm)) executeStep(tm);
}

void core::MachineExecutor::update(core::TuringMachine &tm)
{
  bool currentlyRunning = (state_ == ExecutionState::RUNNING);
  if (currentlyRunning) {
    updateSpaceTracking(tm.tape());
    auto now = std::chrono::steady_clock::now();
    if (now - lastStepTime_ >= (stepDelay_ * (1.f / speedFactor_))) {
      if (canStep(tm)) {
        executeStep(tm);
        lastStepTime_ = now;
      } else {
        state_ = tm.isAccepting() || tm.isRejecting()
          ? ExecutionState::FINISHED
          : ExecutionState::ERROR;
      }
    }
  }
}

bool core::MachineExecutor::canStep(const core::TuringMachine &tm) const
{
  return stepCount_ < maxSteps_
    && !tm.isAccepting()
    && !tm.isRejecting()
    && state_ != ExecutionState::ERROR;
}

void core::MachineExecutor::executeStep(core::TuringMachine &tm)
{
  try {
    tm.step();
    stepCount_++;
  } catch (const std::exception &) {
    state_ = ExecutionState::ERROR;
  }
}

bool core::MachineExecutor::validateMachine(const core::TuringMachine &tm) const
{
  auto result = ExecutionValidator::validate(tm);
  return result.isValid;
}

void core::MachineExecutor::resetSpaceTracking()
{
  minTapePosition_ = 0;
  maxTapePosition_ = 0;
  maxCellsUsed_ = 0;
}

void core::MachineExecutor::updateSpaceTracking(const core::Tape &tape)
{
  // Track head position extremes
  int currentHead = tape.head();
  minTapePosition_ = (std::min)(minTapePosition_, currentHead);
  maxTapePosition_ = (std::max)(maxTapePosition_, currentHead);
  // Count non-blank cells - you'll need to add this method to Tape
  size_t currentCellsUsed = tape.getNonBlankCellCount();
  maxCellsUsed_ = std::max(maxCellsUsed_, currentCellsUsed);
}

std::chrono::milliseconds core::MachineExecutor::getElapsedTime() const
{
  if (state_ == ExecutionState::RUNNING) {
    auto now = std::chrono::steady_clock::now();
    auto currentSession = std::chrono::duration_cast<std::chrono::milliseconds>(now - executionStartTime_);
    totalExecutionTime_ += currentSession;
    executionStartTime_ = now;
  }
  return totalExecutionTime_;
}

std::string core::MachineExecutor::getFormattedTime() const
{
  auto ms = getElapsedTime();
  auto sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
  auto minutes = std::chrono::duration_cast<std::chrono::minutes>(ms);
  return std::format("{}m {:02}.{:03}s", minutes.count(), (sec - minutes).count(), (ms - sec).count());
}


//------------------------------------------------------------------------------------------


core::ExecutionValidator::ValidationResult core::ExecutionValidator::validate(const core::TuringMachine &tm)
{
  ValidationResult result;
  if (tm.transitions().empty()) {
    result.errors.push_back("Machine has no transitions defined");
  }
  bool hasStart = false;
  for (const auto &state : tm.states()) {
    if (state.isStart()) {
      if (hasStart) {
        result.errors.push_back("Multiple start states found");
      }
      hasStart = true;
    }
  }
  if (!hasStart) {
    result.errors.push_back("No start state defined");
  }
  auto unreachable = findUnreachableStates(tm);
  for (const auto &state : unreachable) {
    result.warnings.push_back("State '" + state.name() + "' is unreachable");
  }
  if (hasNonDeterministicTransitions(tm)) {
    result.warnings.push_back("Machine has non-deterministic transitions");
  }
  auto nonUnique = findNonUniqueStates(tm);
  for (const auto &name : nonUnique) {
    result.errors.push_back("State name '" + name + "' is not unique");
  }
  result.isValid = result.errors.empty();
  return result;
}

std::vector<core::State> core::ExecutionValidator::findUnreachableStates(const core::TuringMachine &tm)
{
  // TODO:
  return {};
}

std::vector<std::string> core::ExecutionValidator::findNonUniqueStates(const core::TuringMachine &tm)
{
  std::vector<std::string> res;
  std::set<std::string> names;
  for (const auto &st : tm.states()) {
    if (names.contains(st.name())) {
      res.push_back(st.name());
    } else {
      names.insert(st.name());
    }
  }
  return res;
}

bool core::ExecutionValidator::hasNonDeterministicTransitions(const core::TuringMachine &tm)
{
  // TODO:
  
  return false;
}
