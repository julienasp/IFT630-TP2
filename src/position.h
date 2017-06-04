#ifndef POSITION_H_
#define POSITION_H_

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

class Position {
public:
  using coord_type = int32_t;
  Position();
  Position(coord_type, coord_type);
  Position(const Position&);
  coord_type getX() const noexcept;
  coord_type getY() const noexcept;
  bool operator==(const Position&) const;
  bool operator!=(const Position&) const;
private:
  coord_type coord_x, coord_y;
};

std::ostream& operator<<(std::ostream& os, const Position&);
std::istream& operator>>(std::istream& is, Position& obj);

#endif /* POSITION_H_ */
