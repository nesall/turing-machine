#pragma once

#include <memory>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>


namespace core {

  //class Alphabet {
  //  std::set<char> symbols_;
  //public:
  //  static const char Blank = 0;
  //  void addSymbol(char c) { symbols_.insert(c); }
  //  bool contains(char c) const { return symbols_.count(c) > 0; }
  //  const std::set<char> &symbols() const { return symbols_; }
  //};


  class Tape {
  private:
    std::map<int, char> cells_;
    int headPosition_ = 0;
    mutable std::set<char> alphabet_;
  public:
    static const char Blank = 0;
    enum class Dir { STAY, LEFT, RIGHT };

    int head() const { return headPosition_; }
    char read() const { return cells_.contains(head()) ? cells_.at(head()) : Tape::Blank; }
    void write(char symbol) { writeAt(head(), symbol); }
    void move(Dir dir);
    void moveLeft() { headPosition_ --; }
    void moveRight() { headPosition_ ++; }
    void moveToLeftMost() { if (cells_.empty()) return; headPosition_ = cells_.begin()->first; }
    void moveToRightMost() { if (cells_.empty()) return; headPosition_ = cells_.rbegin()->first; }
    char readAt(int index) const { return cells_.contains(index) ? cells_.at(index) : Tape::Blank; }
    void writeAt(int index, char c);
    std::set<char> alphabet() const;
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json &j);
    size_t getNonBlankCellCount() const;
    std::pair<int, int> getUsedRange() const;
  };

  std::string dirToStr(core::Tape::Dir d);

  class State {
  public:
    enum class Type { TEMP, START, ACCEPT, REJECT, NORMAL };

    State() = default;
    State(const State &) = default;
    explicit State(std::string name, Type type = Type::NORMAL) : name_(name), type_(type) {}
    bool operator<(const State &other) const { return name_ < other.name_; }
    bool operator==(const State &other) const { return name_ == other.name_; }
    std::string name() const { return name_; }
    void setName(const std::string &name);
    Type type() const { return type_; }
    void setType(Type type);
    bool isAccept() const { return type_ == Type::ACCEPT; }
    bool isReject() const { return type_ == Type::REJECT; }
    bool isStart() const { return type_ == Type::START; }
    bool isTemporary() const { return type_ == Type::TEMP; }
  
  private:
    std::string name_;
    State::Type type_ = State::Type::NORMAL;
  };

  class Transition {
    State from_;
    State to_;
    char readSymbol_;
    char writeSymbol_;
    Tape::Dir direction_;
    
  public:
    Transition(const State& from, const State &to, char readSymbol, char writeSymbol, Tape::Dir direction)
        : from_(from), readSymbol_(readSymbol), to_(to), writeSymbol_(writeSymbol), direction_(direction) {}

    bool operator==(const Transition &rhs) const { return uniqueKey() == rhs.uniqueKey(); }

    std::string uniqueKey() const;

    // Getters
    const State &from() const { return from_; }
    char readSymbol() const { return readSymbol_; }
    const State &to() const { return to_; }
    char writeSymbol() const { return writeSymbol_; }
    Tape::Dir direction() const { return direction_; }

    // Setters
    void setFrom(const State &st) { from_ = st; }
    void setReadSymbol(char c) { readSymbol_ = c; }
    void setTo(const State &st) { to_ = st; }
    void setWriteSymbol(char c) { writeSymbol_ = c; }
    void setDirection(Tape::Dir dir) { direction_ = dir; }
  };

  class TuringMachine {
  public:
    TuringMachine();

    State currentState() const { return currentState_; }
    std::string lastExecutedTransition() const { return lastExecutedTransition_; }
    const Tape &tape() const { return tape_; }
    Tape &tape() { return tape_; }
    void step();
    void reset();
    bool isAccepting() const;
    bool isRejecting() const;
    std::vector<State> states() const;
    const std::vector<Transition> &transitions() const { return transitions_; }
    std::string nextUniqueStateName() const;
    void addUnconnectedState(const State &st);
    void removeState(State st);
    bool updateState(const State &o, const State &n);
    bool hasTransitionsFrom(State st) const;
    void addTransition(const Transition &tr);
    void removeTransition(const Transition &tr);
    void updateTransition(const Transition &o, const Transition &n);
    std::vector<State> unconnectedStates() const { return unconnectedStates_; }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json &j);

  private:
    std::vector<State> unconnectedStates_;
    std::vector<Transition> transitions_;
    State currentState_;
    std::string lastExecutedTransition_;
    Tape tape_;
    std::optional<Tape> tapeBackup_;
  };


  enum class ExecutionState {
    STOPPED,      // Ready to run, not started
    RUNNING,      // Actively executing steps
    PAUSED,       // Execution suspended, can resume
    STEP_MODE,    // Manual step-by-step execution
    FINISHED,     // Reached accept/reject state
    ERROR         // Invalid configuration or runtime error
  };

  std::string executionStateToStr(ExecutionState s);

  class MachineExecutor {
  private:
    ExecutionState state_ = ExecutionState::STOPPED;
    std::chrono::milliseconds stepDelay_{ 500 };
    float speedFactor_ = 1.f;
    std::chrono::steady_clock::time_point lastStepTime_;
    size_t stepCount_ = 0;
    size_t maxSteps_ = 10000;
    mutable std::chrono::steady_clock::time_point executionStartTime_;
    mutable std::chrono::milliseconds totalExecutionTime_{ 0 };
    bool wasRunning_ = false;
    int minTapePosition_ = 0;
    int maxTapePosition_ = 0;
    size_t maxCellsUsed_ = 0;

  public:
    void start(core::TuringMachine &tm);
    void pause();
    void stop(core::TuringMachine &tm);
    void stepOnce(core::TuringMachine &tm);
    void update(core::TuringMachine &tm);
    ExecutionState state() const { return state_; }
    bool isRunning() const { return state_ == ExecutionState::RUNNING; }
    float speedFactor() const { return speedFactor_; }
    void setSpeedFactor(float f) { speedFactor_ = f; }
    size_t stepCount() const { return stepCount_; }
    size_t cellsUsed() const { return maxCellsUsed_; }
    std::chrono::milliseconds getElapsedTime() const;
    std::string getFormattedTime() const;

  private:
    bool canStep(const core::TuringMachine &tm) const;
    void executeStep(core::TuringMachine &tm);
    bool validateMachine(const core::TuringMachine &tm) const;
    void resetMachine(core::TuringMachine &tm) { tm.reset(); }
    void resetSpaceTracking();
    void updateSpaceTracking(const core::Tape &tape);
  };


  class ExecutionValidator {
  public:
    struct ValidationResult {
      bool isValid = true;
      std::vector<std::string> errors;
      std::vector<std::string> warnings;
    };
    static ValidationResult validate(const core::TuringMachine &tm);
  private:
    static std::vector<core::State> findUnreachableStates(const core::TuringMachine &tm);
    static std::vector<std::string> findNonUniqueStates(const core::TuringMachine &tm);
    static bool hasNonDeterministicTransitions(const core::TuringMachine &tm);
  };

} // namespace core