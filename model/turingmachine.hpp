#pragma once

#include <memory>
#include <set>
#include <map>
#include <variant>
#include <string>

namespace core {

  class Alphabet {
    std::set<char> symbols_;    
  public:
    static const char Blank = 0;
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
  
  private:
    std::string name_;
    State::Type type_ = State::Type::NORMAL;
  };

  class TuringMachine {
  public:
    TuringMachine();

    State currentState() const { return currentState_; }
    const Tape &tape() const { return tape_; }
    void step();
    bool isAccepting() const;
    bool isRejecting() const;

  private:
    void addTransition(const State &from, char readSymbol, const State &to, std::variant<char, Tape::Dir> writeOrMove);

  private:
    std::map<std::tuple<State, char>, std::tuple<State, std::variant<char, Tape::Dir>>> transitions_;
    State currentState_;
    Tape tape_;
  };

}