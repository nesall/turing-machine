#pragma once

#include <memory>
#include <set>
#include <map>
#include <vector>
#include <string>

namespace core {

  class Alphabet {
    std::set<char> symbols_;    
  public:
    static const char Blank = 0;
    void addSymbol(char c) { symbols_.insert(c); }
    bool contains(char c) const { return symbols_.count(c) > 0; }
  };

  class Tape {
  private:
    std::map<int, char> cells_;
    int headPosition_ = 0;
  public:
    enum class Dir { LEFT, RIGHT };
    char read() const { return cells_.contains(headPosition()) ? cells_.at(headPosition()) : Alphabet::Blank; }
    void write(char symbol) { cells_[headPosition()] = symbol; }
    void move(Dir dir) { dir == Dir::LEFT ? headPosition_ -- : headPosition_ ++; }
    int headPosition() const { return headPosition_; }
  };

  class State {
  public:
    enum class Type { START, ACCEPT, REJECT, NORMAL };

    State() = default;
    State(std::string name, Type type = Type::NORMAL) : name_(name), type_(type) {}
    bool operator<(const State &other) const { return name_ < other.name_; }
    bool operator==(const State &other) const { return name_ == other.name_; }
    std::string name() const { return name_; }
    Type type() const { return type_; }
    bool isAccept() const { return type_ == Type::ACCEPT; }
    bool isReject() const { return type_ == Type::REJECT; }
    bool isStart() const { return type_ == Type::START; }
  
  private:
    std::string name_;
    State::Type type_ = State::Type::NORMAL;
  };

  class Transition {
    State from_;
    char readSymbol_;
    State to_;
    char writeSymbol_;
    Tape::Dir direction_;
    
  public:
    Transition(const State& from, char readSymbol, const State& to, char writeSymbol, Tape::Dir direction)
        : from_(from), readSymbol_(readSymbol), to_(to), writeSymbol_(writeSymbol), direction_(direction) {}

    // Getters
    const State &from() const { return from_; }
    char readSymbol() const { return readSymbol_; }
    const State &to() const { return to_; }
    char writeSymbol() const { return writeSymbol_; }
    Tape::Dir direction() const { return direction_; }

    // Optionally, add comparison operators, print methods, etc.
  };

  class TuringMachine {
  public:
    TuringMachine();

    State currentState() const { return currentState_; }
    const Tape &tape() const { return tape_; }
    void step();
    bool isAccepting() const;
    bool isRejecting() const;
    std::vector<State> states() const;
    const std::vector<Transition> &transitions() const { return transitions_; }
    std::string nextUniqueStateName() const;
    void addUnconnectedState(const State &st);
    // Also removes connected transitions
    void removeState(State st);
    bool hasTransitionsFrom(State st) const;
    std::vector<State> unconnectedStates() const { return unconnectedStates_; }
  private:
    void addTransition(const State &from, char readSymbol, const State &to, char writeSymbol, Tape::Dir dir);

  private:
    std::vector<State> unconnectedStates_;
    std::vector<Transition> transitions_;
    State currentState_;
    Tape tape_;
  };

}