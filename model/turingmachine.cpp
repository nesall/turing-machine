#include "turingmachine.hpp"


core::TuringMachine::TuringMachine()
{
  // Example transitions for a simple Turing machine
  State q0("q0", State::Type::START);
  State q1("q1");
  State qAccept("qAccept", State::Type::ACCEPT);
  State qReject("qReject", State::Type::REJECT);
  addTransition(q0, '0', q1, '1'); // On reading '0' in state q0, write '1' and move to state q1
  addTransition(q1, '1', qAccept, Tape::Dir::RIGHT); // On reading '1' in state q1, move right and go to accepting state
  addTransition(q1, Alphabet::Blank, qReject, Tape::Dir::RIGHT); // On reading blank in state q1, move right and go to rejecting state
  currentState_ = q0; // Set initial state
}

void core::TuringMachine::step()
{
  char currentSymbol = tape_.read();
  auto transitionKey = std::make_tuple(currentState_, currentSymbol);
  if (transitions_.find(transitionKey) != transitions_.end()) {
    auto [nextState, writeOrMove] = transitions_[transitionKey];
    currentState_ = nextState;
    if (std::holds_alternative<char>(writeOrMove)) {
      tape_.write(std::get<char>(writeOrMove));
    } else {
      tape_.move(std::get<Tape::Dir>(writeOrMove));
    }
  } else {
    // No valid transition, halt the machine (could also set to a reject state)
    currentState_ = State("HALT", State::Type::REJECT);
  }
}

bool core::TuringMachine::isAccepting() const
{
  return currentState_.type() == State::Type::ACCEPT;
}

bool core::TuringMachine::isRejecting() const
{
  return currentState_.type() == State::Type::REJECT;  
}

void core::TuringMachine::addTransition(const State &from, char readSymbol, const State &to, std::variant<char, Tape::Dir> writeOrMove)
{
  transitions_[{from, readSymbol}] = { to, writeOrMove };
}
