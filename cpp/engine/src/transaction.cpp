#include "engine/core.hpp"

namespace engine {
Game::Game(const Game &other) : GameState(other), QueryCaches(other), floors(other.floors) {}
Game &Game::movable(Game &other) {
  if (other.transaction_)
    throw Error("Cannot move a game with an active transaction");
  return other;
}
Game::Game(Game &&other)
    : GameState(std::move(static_cast<GameState &>(movable(other)))),
      QueryCaches(std::move(static_cast<QueryCaches &>(other))), floors(std::move(other.floors)) {}
Game &Game::operator=(const Game &other) {
  if (this == &other)
    return *this;
  if (transaction_)
    for (int f = 0; f < floors_count; ++f)
      if (floors[f] != other.floors[f])
        record_floor(f);
  GameState::operator=(other);
  QueryCaches::operator=(other);
  floors = other.floors;
  return *this;
}
Game &Game::operator=(Game &&other) {
  if (this == &other)
    return *this;
  movable(other);
  if (transaction_)
    for (int f = 0; f < floors_count; ++f)
      if (floors[f] != other.floors[f])
        record_floor(f);
  GameState::operator=(std::move(other));
  QueryCaches::operator=(std::move(other));
  floors = std::move(other.floors);
  return *this;
}
void Game::record_floor(int f) {
  for (auto *scope = transaction_; scope; scope = scope->parent_)
    scope->capture(f);
}
Transaction Game::transaction() {
  return Transaction(*this);
}
void Game::apply(const Token &input, bool strict) {
  auto scope = transaction();
  token(input, strict);
  scope.commit();
}
Transaction::Transaction(Game &game)
    : game_(&game), parent_(game.transaction_), before_(std::in_place, static_cast<const GameState &>(game)),
      caches_(game) {
  game.transaction_ = this;
}
Transaction::~Transaction() {
  if (game_)
    rollback();
}
void Transaction::capture(int f) {
  const auto bit = std::uint64_t(1) << f;
  if (captured_ & bit)
    return;
  floors_.push_back({f, game_->floors[f]});
  captured_ |= bit;
}
void Transaction::finish() {
  game_->transaction_ = parent_;
  game_ = nullptr;
  before_.reset();
  caches_ = {};
  floors_.clear();
}
void Transaction::commit() {
  if (!game_ || game_->transaction_ != this)
    throw Error("Transactions require LIFO completion");
  finish();
}
void Transaction::rollback() {
  if (!game_ || game_->transaction_ != this)
    throw Error("Transactions require LIFO completion");
  static_cast<GameState &>(*game_) = std::move(*before_);
  static_cast<QueryCaches &>(*game_) = std::move(caches_);
  for (auto &entry : floors_)
    game_->floors[entry.floor] = std::move(entry.before);
  finish();
}
} // namespace engine
