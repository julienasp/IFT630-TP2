#ifndef MAP_H_
#define MAP_H_

#include <fstream>
#include <sstream>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "position.h"

#define FROMAGE 'F'
#define CHAT 'C'
#define MUR '#'
#define RAT 'R'
#define VIDE ' '
#define SORTIE ' '

struct PositionHasher {
  std::size_t operator()(const Position& pos) const {
    std::size_t hash1 = hash<int>()(pos.getX() * 11);
    std::size_t hash2 = hash<int>()(pos.getY() * 13);
    return hash1 ^ (hash2 << 1);
  }
};

template<class T, class S>
bool inSequence(const T& e, const S& v);

class Map {
public:
  Map(std::istream&);
  ~Map();
  Position Move(const Position&, const Position&, stringstream&);
  Position AStarShortestPath(const Position&, const Position&);
  Position GetClosestsDestination(const Position&,
      const std::vector<Position>&);
  Position AStarShortestPathForDestinationSet(const Position&,
      const std::vector<Position>&);
  std::ostream& operator<<(std::ostream& os) const;
  const std::vector<Position>& getListeRat() const;
  const std::vector<Position>& getListeFromage() const;
  const std::vector<Position>& getListeSortie() const;
  const std::map<int, Position>& getLookupTable() const;

  char showPosition(const Position&) const;
  int getRankPosition(const Position&) const;
  static int ManhattanDistance(Position, Position);
private:
  std::vector<Position> FindNearbyPositions(Position, char);
  void updateListeRat(const Position& currentPos, const Position& nextPos);
  void updateListeFromage(const Position& nextPos);
  void updateLookupTable(const Position& currentPos, const Position& nextPos);
  void updateListeRatMort(const Position& nextPos);
  void updateLookupTableNext(const Position& currentPos,
      const Position& nextPos);

  std::size_t sizeX, sizeY;
  std::map<int, Position> lookupTable;
  std::vector<Position> listeRat;
  std::vector<Position> listeFromage;
  std::vector<Position> listeSortie;
  unordered_map<Position, char, PositionHasher> contenu;
};

std::ostream& operator<<(std::ostream& os, const Map&);

#endif /* MAPELEMENT_H_ */
