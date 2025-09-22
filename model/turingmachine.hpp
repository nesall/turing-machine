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
    int head() const { return headPosition_; }
    char read() const { return cells_.contains(head()) ? cells_.at(head()) : Alphabet::Blank; }
    void write(char symbol) { cells_[head()] = symbol; }
    void move(Dir dir) { dir == Dir::LEFT ? moveLeft() : moveRight(); }
    void moveLeft() { headPosition_ --; }
    void moveRight() { headPosition_ ++; }
    void moveToLeftMost() { if (cells_.empty()) return; headPosition_ = cells_.begin()->first; }
    void moveToRightMost() { if (cells_.empty()) return; headPosition_ = cells_.rbegin()->first; }

    char readAt(int index) const { return cells_.contains(index) ? cells_.at(index) : Alphabet::Blank; }
  };

  class State {
  public:
    enum class Type { START, ACCEPT, REJECT, NORMAL };

    State() = default;
    State(const State &) = default;
    explicit State(std::string name, Type type = Type::NORMAL) : name_(name), type_(type) {}
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
    State to_;
    char readSymbol_;
    char writeSymbol_;
    Tape::Dir direction_;
    
  public:
    Transition(const State& from, const State &to, char readSymbol, char writeSymbol, Tape::Dir direction)
        : from_(from), readSymbol_(readSymbol), to_(to), writeSymbol_(writeSymbol), direction_(direction) {}

    bool operator==(const Transition &rhs) const { 
      return from_ == rhs.from_ && to_ == rhs.to_ && readSymbol_ == rhs.readSymbol_ && writeSymbol_ == rhs.writeSymbol_ && direction_ == rhs.direction_;
    }

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
    const Tape &tape() const { return tape_; }
    Tape &tape() { return tape_; }
    void step();
    bool isAccepting() const;
    bool isRejecting() const;
    std::vector<State> states() const;
    const std::vector<Transition> &transitions() const { return transitions_; }
    std::string nextUniqueStateName() const;
    void addUnconnectedState(const State &st);
    void removeState(State st);
    bool hasTransitionsFrom(State st) const;
    void addTransition(const Transition &tr);
    void removeTransition(const Transition &tr);
    std::vector<State> unconnectedStates() const { return unconnectedStates_; }

    std::string toJson() const;
    void fromJson(const std::string &json);

  private:
    std::vector<State> unconnectedStates_;
    std::vector<Transition> transitions_;
    State currentState_;
    Tape tape_;
  };

}