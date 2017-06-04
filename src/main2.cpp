/*
 * main2.cpp
 *
 *  Created on: 5 ao�t 2016
 *      Author: Phil
 */


#include "map.h"
#include <chrono>
#include <ctime>

int main() {
  using namespace std::chrono;

  ifstream myfile("cartes/map3");
  if (myfile.is_open()) {
    return 1;
  }
  Map map = Map(myfile);
  cout << map;

  /*
  unordered_map<Position, Position, PositionHasher> previous; // contains the previous positions
  previous[Position{}] = Position{42, 24};
  previous[Position{2,4}] = Position{242, 224};
  previous[Position{2,4}] = Position{2421, 1224};
  unordered_map<Position, int, PositionHasher> fCost; // distance from starting node + heuristic
  auto comp =
      [&] (const Position& f, const Position& s) {
    return fCost[f] > fCost[s];};
  std::priority_queue<Position, std::vector<Position>, decltype(comp)> priorityQueue(
      comp); // lowest cost value always on top
  fCost[Position{1337,23}] = 0;
  fCost[Position{42, 24}] = 8;
  priorityQueue.push(Position{1337,23}); // We add the first node in the queue
  priorityQueue.push(Position{1337,223}); // We add the first node in the queue
  Position currentPosition = priorityQueue.top(); // We pick the first element
  priorityQueue.pop(); // We pop the first element
  priorityQueue.push(Position{42, 24}); // We add the first node in the queue*/
  auto avant = system_clock::now();
  auto pos = map.AStarShortestPath(map.getMapElement({2,4}), map.getMapElement({24,20}));
  auto apres = system_clock::now();
  cout << "Le prochain movement de la source sera:" << endl;
  cout << pos.getX() << " " << pos.getY() << endl;
  cout << "Chemin evaluer en " << duration_cast<milliseconds>(apres-avant).count() <<  " ms." << endl;
}



