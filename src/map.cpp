#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include "map.h"

using namespace std;

Map::Map(istream& mapstream)
    : sizeX { }, sizeY { }, lookupTable { } {
  char c;
  int rank = 0;
  Position::coord_type x { }, y { }, m =
      numeric_limits<Position::coord_type>::max();

  while (mapstream.get(c)) {
    Position pos(x % m, y);
    ++x;
    contenu[pos] = c;
    switch (c) {
    case '#':
      break;
    case 'F':
      listeFromage.push_back(pos);
      break;
    case 'C':
      lookupTable[rank++] = pos;
      break;
    case 'R':
      lookupTable[rank++] = pos;
      listeRat.push_back(pos);
      break;
    case ' ':
      if (pos.getX() == 0 || pos.getX() == m - 2 || pos.getY() == 0) {
        listeSortie.push_back(pos);
      }
      break;
    case '\r':
      --x;
      break;
    case '\n':
    default:
      ++y;
      m = x;
      x = 0;
      break;
    }
  }
  sizeX = m - 1;
  sizeY = y + 1;

  for (size_t xfin = 0; xfin < sizeX; ++xfin) {
    auto pos = Position { static_cast<Position::coord_type>(xfin), y };
    if (contenu[pos] == VIDE) {
      listeSortie.push_back(pos);
    }
  }
}

Map::~Map() {
}

void Map::updateListeRat(const Position& currentPos, const Position& nextPos) {
  for (size_t i { }; i != listeRat.size(); ++i) {
    if (listeRat[i] == currentPos) {
      listeRat[i] = nextPos;
      break;
    }
  }
}

void Map::updateListeFromage(const Position& nextPos) {
  for (unsigned int i = 0; i < listeFromage.size(); ++i) {
    if (listeFromage[i] == nextPos) {
      listeFromage.erase(listeFromage.begin() + i);
      break;
    }
  }
}

void Map::updateLookupTable(const Position& currentPos,
    const Position& nextPos) {
  for (auto p : lookupTable) {
    if (p.second == currentPos) {
      lookupTable[p.first] = nextPos;
      break;
    }
  }
}

void Map::updateListeRatMort(const Position& nextPos) {
  for (unsigned int i = 0; i < listeRat.size(); ++i) {
    if (listeRat[i] == nextPos) {
      listeRat.erase(listeRat.begin() + i);
      break;
    }
  }
}

Position Map::Move(const Position& currentPos, const Position& nextPos,
    stringstream& fichierStat) {
  ifstream fic_out;
  auto it = contenu.find(nextPos);
  if (it == end(contenu)) {
    return currentPos;
  }
  auto nextElem = it->second;
  auto currentElem = contenu[currentPos]; // Tjrs valide

  // On a trouve quelque chose dans le unordered map
  switch (nextElem) {
  case CHAT:
    return currentPos;
  case RAT:
    // CHAT MANGE RAT
    if (currentElem == CHAT) {
      // updater la lookuptable
      int chatRang, ratRang;
      for (auto p : lookupTable) {
        if (p.second == currentPos) {
          chatRang = p.first;
        } else if (p.second == nextPos) {
          ratRang = p.first;
        }
      }
      lookupTable[chatRang] = nextPos;
      lookupTable.erase(ratRang);
      // enlever le rat de la liste de rat
      updateListeRatMort(nextPos);
      // updater les mapelements
      contenu[nextPos] = currentElem;
      contenu[currentPos] = VIDE;
      fichierStat << "Chat " << chatRang << " a mangé le rat " << ratRang
          << endl;
      return nextPos;
    } else { // RAT MANGE RAT

      return currentPos;
    }
  case FROMAGE:
    // CHAT MANGE FROMAGE
    if (currentElem == CHAT) {
      return currentPos;
    } else { // RAT MANGE FROMAGE
      updateLookupTable(currentPos, nextPos);
      updateListeFromage(nextPos);
      updateListeRat(currentPos, nextPos);
      contenu[nextPos] = currentElem;
      contenu[currentPos] = VIDE;
      fichierStat << "Rat " << getRankPosition(nextPos)
          << " a mange un fromage a la position " << nextPos << endl;
      return nextPos;
    }
  case MUR:
    return currentPos;
  case VIDE: {
    if (inSequence(currentPos, listeSortie)) {
      // CHAT SORT
      if (currentElem == CHAT) {
        return currentPos;
      } else { // RAT SORT
        fichierStat << "Le rat " << getRankPosition(currentPos)
            << " a quitté par la sortie " << nextPos << endl;
        for (auto p : lookupTable) {
          if (p.second == currentPos) {
            lookupTable.erase(p.first);
            break;
          }
        }
        updateListeRatMort(currentPos);
        contenu[currentPos] = VIDE;
        return nextPos;
      }
    } else {
      // BOUGE CASE VIDE
      updateLookupTable(currentPos, nextPos);
      updateListeRat(currentPos, nextPos);
      auto tmp = contenu[nextPos];
      contenu[nextPos] = currentElem;
      contenu[currentPos] = tmp;
      return nextPos;
    }
  }
  default:
    return currentPos;
  }
}

std::ostream& Map::operator<<(std::ostream& os) const {
  auto Y = static_cast<Position::coord_type>(sizeY);
  for (Position::coord_type y = 0; y < Y; ++y) {
    for (Position::coord_type x = 0;
        x < static_cast<Position::coord_type>(sizeX); ++x) {
      auto it = contenu.find(Position { x, y });
      if (it == end(contenu)) {
        os << "E";
      } else {
        os << it->second;
      }
    }
    // do not put a newline at the end (this breaks (re)construction)
    if (y < Y - 1) {
      os << '\n'; //endl;
    } else {
      os << flush;
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Map& that) {
  that.operator <<(os);
  return os;
}

const std::vector<Position>& Map::getListeRat() const {
  return listeRat;
}

const std::vector<Position>& Map::getListeFromage() const {
  return listeFromage;
}

const std::vector<Position>& Map::getListeSortie() const {
  return listeSortie;
}

const std::map<int, Position>& Map::getLookupTable() const {
  return lookupTable;
}

char Map::showPosition(const Position& pos) const {
  return contenu.find(pos)->second;
}

int Map::getRankPosition(const Position& pos) const {
  for (auto p : getLookupTable()) {
    if (p.second == pos) {
      return p.first;
    }
  }
  return numeric_limits<int>::max(); // pouf
}

template<class T, class S>
bool inSequence(const T& e, const S& s) {
  auto it = find(begin(s), end(s), e);
  return it != end(s);
}

/**
 * \fn Map::AStarShortestPath(MapElement* source,MapElement* dest)
 *  \brief Retourne la premiere position du chemin le plus court,selon l'algorithme A*, vers une destination
 *
 *  \pre le carte est valide.
 *  \pre le point d'origine existe.
 *  \pre le point d'arrivee existe.
 *
 *  \post Retourne la premi�re position du chemin le plus cours vers une destination
 */
Position Map::AStarShortestPath(const Position& sourcePosition,
    const Position& destPosition) {
  //Init
  unordered_map<Position, int, PositionHasher> fCost; // distance from starting node + heuristic
  unordered_map<Position, int, PositionHasher> gCost; // distance from starting node
  unordered_map<Position, Position, PositionHasher> previous; // contains the previous positions
  unordered_set<Position, PositionHasher> closedSet; // The set of nodes already evaluated
  auto comp =
      [&] (const Position& f, const Position& s) {return fCost[f] > fCost[s];};
  std::priority_queue<Position, std::vector<Position>, decltype(comp)> priorityQueue(
      comp); // lowest cost value always on top
  int hCost = 0; // heuristic distance
  int totalCost = 0;

  //Init the first node
  fCost[Position { -1, -1 }] = numeric_limits<int>::max(); // Quick fix set to max int
  fCost[sourcePosition] = 0; // source cost is zero
  gCost[sourcePosition] = 0; // source cost is zero
  previous[sourcePosition] = sourcePosition; // previous position for the source is itself
  priorityQueue.push(Position { -1, -1 }); // Quick fix because broken queue
  priorityQueue.push(sourcePosition); // We add the first node in the queue
  while (!priorityQueue.empty()) {
    Position currentPosition = priorityQueue.top(); // We pick the first element
    priorityQueue.pop(); // We pop the first element
    closedSet.insert(currentPosition); // We add the node in the set

    if (currentPosition == destPosition) {
      break; //Destination reached we break out of the loop
    }
    std::vector<Position> nearbyPositions = FindNearbyPositions(currentPosition,
        contenu[sourcePosition]); // We retreive all the nearby Positions
    //We iterate through all the nearby positions

    for (std::vector<Position>::iterator it = nearbyPositions.begin();
        it != nearbyPositions.end(); ++it) {
      Position currentNearbyPosition = (*it);

      if (closedSet.find(currentNearbyPosition) != end(closedSet)) {
        continue; // Ignore the neighbor which is already evaluated.
      }

      //Create new map entry if position wasn't already there
      if (fCost.find(currentNearbyPosition) == end(fCost)) {
        //We add a large number to the node, if it doesn't exist
        fCost[currentNearbyPosition] = numeric_limits<int>::max(); //Infinity, Large number
      }

      //Create new map entry if position wasn't already there
      if (gCost.find(currentNearbyPosition) == end(gCost)) {
        //We add the cost from the currentNearbyPosition to the source
        gCost[currentNearbyPosition] = gCost.find(currentPosition)->second + 1;
      } else { //already exist
        if (gCost[currentNearbyPosition] > gCost[currentPosition] + 1) {
          gCost[currentNearbyPosition] = gCost[currentPosition] + 1;
        }
      }

      hCost = ManhattanDistance(currentNearbyPosition, destPosition); // distance betweeen currentNearbyPosition and destination
      totalCost = hCost + gCost[currentNearbyPosition];
      if (totalCost < fCost[currentNearbyPosition]) {
        fCost[currentNearbyPosition] = totalCost;
        previous[currentNearbyPosition] = currentPosition;
        priorityQueue.push(currentNearbyPosition);
      } // END IF

    } // END FOR
  } //END WHILE

  //Reserve path
  Position current = destPosition;
  int nbStep = 0;

  while (previous[current] != sourcePosition) {
    nbStep++;
    current = previous[current];
  }

  return current;
}

Position Map::GetClosestsDestination(const Position& sourcePosition,
    const std::vector<Position>& destSet) {
  int minDist = numeric_limits<int>::max(); // High cost to compare
  auto closestPos = destSet[0]; // we pick the first one

  for (auto it = begin(destSet); it != end(destSet); ++it) {
    auto manhattanDistance = ManhattanDistance(sourcePosition, *it);
    if (minDist > manhattanDistance) {
      minDist = manhattanDistance; // the current MapElement is closer so we keep him
      closestPos = *it;
    }
  }
  return closestPos;
}

/**
 * \fn Map::AStarShortestPathForDestinationSet(MapElement* source,std::vector<MapElement*> destSet)
 *  \brief Retourne la premiere position du chemin le plus court, selon l'algorithme A* et l'heuristic de la distance Manhattan, vers le fromage ou le rat le plus proche de la source.
 *
 *  \pre le carte est valide.
 *  \pre le point d'origine existe.
 *  \pre le point d'arrivee existe.
 *
 *  \post Retourne la premi�re position du chemin le plus cours vers la destination la plus proche
 */
Position Map::AStarShortestPathForDestinationSet(const Position& sourcePosition,
    const std::vector<Position>& destSet) {
  auto closest = GetClosestsDestination(sourcePosition, destSet);
  return AStarShortestPath(sourcePosition, closest);
}

/**
 * \fn int Map::ManhattanDistance(MapElement* source,MapElement* dest)
 *  \brief Retourne la distance Manhattan entre deux points.
 *
 *  \pre le carte existe
 *  \pre le point d'origine existe.
 *  \pre le point d'arrivee existe.
 *
 *  \post Retourne la distance calcul�
 */
int Map::ManhattanDistance(Position source, Position dest) {
  auto a = abs(static_cast<int>(dest.getX() - source.getX()));
  auto b = abs(static_cast<int>(dest.getY() - source.getY()));
  return a + b;
}

/**
 * \fn std::vector<Position> Map::FindNearbyPositions(MapElement* me)
 *  \brief Retourne un vector de position avoisinante pour un MapElement donn�
 *
 *  \pre le carte existe
 *  \pre l'�l�menet re�u existe.
 *
 *  \post Retourne un vecteur de position avoisinante
 */
std::vector<Position> Map::FindNearbyPositions(Position pos, char type) {

  std::vector<Position> nearbyPositions;
  if (type == 0) { // Rat
    //Diagonal
    nearbyPositions.push_back(Position { pos.getX() + 1, pos.getY() + 1 }); // NE
    nearbyPositions.push_back(Position { pos.getX() + 1, pos.getY() - 1 }); // SE
    nearbyPositions.push_back(Position { pos.getX() - 1, pos.getY() - 1 }); // SW
    nearbyPositions.push_back(Position { pos.getX() - 1, pos.getY() + 1 }); // NW
  }
//Vertical
  nearbyPositions.push_back(Position { pos.getX(), pos.getY() + 1 }); // N
  nearbyPositions.push_back(Position { pos.getX(), pos.getY() - 1 }); // S

//Horizontal
  nearbyPositions.push_back(Position { pos.getX() + 1, pos.getY() }); // E
  nearbyPositions.push_back(Position { pos.getX() - 1, pos.getY() }); // W

//Trim out-of-the-map positions, Trim walls positions and we trim special positions depending if the source MapElement is a Cat or a Rat
  nearbyPositions.erase(
      std::remove_if(nearbyPositions.begin(), nearbyPositions.end(),
          [&] (const Position& f) {
            auto valide = contenu.find(Position {f.getX(), f.getY()});
            if(valide == end(contenu)) {
              return true;
            }
            else if(type == RAT) { // nearby position for the Rat so we keep only empty, exits and cheese
              return !(contenu[f] == VIDE || contenu[f] == FROMAGE || contenu[f] == SORTIE );// If empty, exits or cheese we keep it (for the rat)
            }
            else { // nearby position for the Cat so we keep only empty and Rat
              return !(contenu[f] == VIDE || contenu[f] == RAT );// If empty or rat we keep it (for the cat)
            }
          }), nearbyPositions.end());

  return nearbyPositions;
}

