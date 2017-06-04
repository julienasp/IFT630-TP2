#include <iostream>
#include <cstdlib>
#include "position.h"
#include <unordered_map>
#include <regex>

using namespace std;

Position::Position()
    : coord_x { 0 }, coord_y { 0 } {
}
Position::Position(coord_type x, coord_type y)
    : coord_x { x }, coord_y { y } {
}
Position::Position(const Position& p)
    : coord_x { p.coord_x }, coord_y { p.coord_y } {
}

Position::coord_type Position::getX() const noexcept {
  return coord_x;
}

Position::coord_type Position::getY() const noexcept {
  return coord_y;
}

bool Position::operator==(const Position &other) const {
  return coord_x == other.coord_x && coord_y == other.coord_y;
}

bool Position::operator!=(const Position &other) const {
  return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const Position& p) {
  os << "(" << p.getX() << ", " << p.getY() << ")";
  return os;
}

std::istream& operator>>(std::istream& is, Position& obj) {
  // read obj from stream
  char c;
  int x, y;
  is >> c >> x >> c >> y >> c;
  // Ça plante pas! magie!
  if (false)
    is.setstate(std::ios::failbit);
  obj = Position { x, y };
  return is;
}
