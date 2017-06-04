#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "mpi.h"
#include "map.h"
#include "position.h"

using namespace std;
using namespace std::chrono;

using Args = vector<string>;

struct rat_etat {
  bool panique;
  int alzheimer;
  rat_etat()
      : panique { }, alzheimer { } {
  }
};
struct Compt {
  int nbDemandes;
  int nbMouvAcceptes;
  int nbMiaulement;
  Compt()
      : nbDemandes { }, nbMouvAcceptes { }, nbMiaulement { } {
  }
};

stringstream statistique;

// setup the endpoint to be chasseur
void init_chasseur(const mpi_server& mpi, mpi_endpoint& ep) {
  ep.add_handler(MMT_DO, [&](const mpi_message& msg) {
    if (mpi.probe(msg)) {
      return;
    }

    stringstream mapss {};
    mapss << msg.comment;
    Position curr;
    mapss >> curr;
    Map map {mapss};
    auto cibles = map.getListeRat();
    Position dest = map.AStarShortestPathForDestinationSet(curr, cibles);
    stringstream positions {};
    positions << dest;
    auto closestRat = map.GetClosestsDestination(curr, map.getListeRat());
    auto closestRatDist = map.ManhattanDistance(curr, closestRat);
    if (closestRatDist < 11) {
      ep.reply(msg, MMT_SPECIAL, positions.str());
    }
    positions << curr;
    ep.reply(msg, positions.str());
  });
}

// setup the endpoint to be rat
void init_rat(const mpi_server& mpi, mpi_endpoint& ep, rat_etat& re) {
  ep.add_handler(MMT_DO, [&](const mpi_message& msg) {
    if (mpi.probe(msg)) {
      return;
    }
    stringstream mapss {};
    mapss << msg.comment;
    Position curr;
    mapss >> curr;
    Map map {mapss};
    if (--re.alzheimer < 1) {
      re.panique = false;
    }
    auto cibles = re.panique ? map.getListeSortie() : map.getListeFromage();
    Position dest = map.AStarShortestPathForDestinationSet(curr, cibles);
    stringstream positions {};
    positions << dest << curr;
    ep.reply(msg, positions.str());
  });
  ep.add_handler(MMT_SPECIAL, [&](const mpi_message& msg) {
    Position chat, moi;
    stringstream sss {};
    sss << msg.comment;
    sss >> chat >> moi;
    Map map {sss};
    auto distance = map.ManhattanDistance(chat, moi);
    if (distance < 8) {
      re.panique = true;
      re.alzheimer = 5;
    }
  });
}

void SUICIDE_COLLECTIF(mpi_server& mpi, const mpi_unique_comm& space,
    mpi_endpoint& ep, const mpi_message& msg,
    const map<int, Position>& lookupTable) {
  cout << mpi << "SUICIDE_COLLECTIF" << endl;
  for (auto p : lookupTable) {
    ep.reply(msg, p.first, MMT_STOP, { });
  }
  ep.request_stop();
}
void EXTERMINER(mpi_endpoint& ep, const map<int, Position>& lookupTable,
    const Position& pos, const mpi_message& msg) {
  for (auto p : lookupTable) {
    if (p.second == pos) {
      cout << "EXTERMINATED " << p.first << " " << pos << endl;
      ep.reply(msg, p.first, MMT_STOP, { });
      break;
    }
  }
}

int main(int argc, char **argv) {
  // seed rand precisely so every process can have almost-unique seeds.
  srand(
      time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count());

  // init MPI
  mpi_server mpi { &argc, &argv };
  mpi_endpoint ep(&mpi, [&](const mpi_message& msg) {
    cout << mpi << "kessez ça!? : " << msg.comment << endl;
    ep.reply(msg, MMT_STOP, {}); // arrête don tes niaiseries
    });
  Args args { argv, argv + argc };

  if (mpi.parent() != MPI_COMM_NULL) {
    if (args[1] == "Joueur") {
      MPI_Barrier(mpi.parent());
      // Joueur process
      rat_etat re { };
      // add an handler to be remotely stopped
      ep.add_handler(MMT_STOP, [&](const mpi_message&) {
#if MPI_VERBOSE
          cout << mpi << "Je meurt!" << endl;
#endif
          ep.request_stop();
        });

      // add an handler to setup ourselves
      ep.add_handler(MMT_BECOME, [&](const mpi_message& msg) {
        if (msg.comment == "R") {
          init_rat(mpi, ep, re);
          string t {"Je suis un init_rat! -"};
          t.append(mpi.id());
          ep.reply(msg, move(t));
        } else if (msg.comment == "C") {
          init_chasseur(mpi, ep);
          string t {"Je suis un chasseur! -"};
          t.append(mpi.id());
          ep.reply(msg, move(t));
        }
      });

      // start handling received messages
      ep.start(mpi.parent());
      ep.prune(mpi.parent());
    }
  } else {
    // Root process

    // fail if invoked incorrectly
    if (argc < 4) {
      cerr << mpi << "<path carte> <|chasseurs|> <|rats|>" << endl;
      return 1;
    }

    // fail if the map can't be openned
    ifstream myfile(args[1]);
    if (!myfile.is_open()) {
      cerr << mpi << args[1] << " : carte pas ouvrable" << endl;
      return 1;
    }

    // init Map and |processes|
    Map map { myfile };
    int qty_c { atoi(argv[2]) }, qty_r { atoi(argv[3]) };
    unique_ptr<Compt[]> m { new Compt[map.getLookupTable().size()] }; // clean up automatically

    // decl a self disconnecting communicator
    mpi_unique_comm space;
    auto debut_root = system_clock::now();
    auto dernier_map_stat = debut_root;

    // if all spawns are OK
    if (!mpi.spawn(qty_r + qty_c, argv[0], vector<char*> {
        const_cast<char*>("Joueur"), nullptr }, &space.comm)) {

      MPI_Barrier(space.comm);
      for (auto r : map.getLookupTable()) {
        mpi.send_message(space.comm, r.first, MMT_BECOME,
            string { map.showPosition(r.second) });
      }

      // add an handler to receive the replies to the BECOME messages just sent,
      // messages don't get dropped so they'll wait, no worries
      ep.add_handler(MMT_BECOME, [&](const mpi_message& msg) {
        // child replied -> it is ready
          stringstream mapss {};
          mapss << map.getLookupTable().at(msg.source);
          mapss << map;
          cout << mpi << "Ding!:" << map.getLookupTable().at(msg.source) << " " << msg.comment << endl;
          // send it some work
          ep.reply(msg, MMT_DO, mapss.str());
        });

      // add an handler to process their move requests
      ep.add_handler(MMT_DO,
          [&](const mpi_message& msg) {
            Position posDest, posCour;
            stringstream doss {};
            doss << msg.comment;
            doss >> posDest >> posCour;

            auto rats_avant = map.getListeRat();
            std::map<int, Position> lt_avant = {begin(map.getLookupTable()), end(map.getLookupTable())};
            Position newPos = map.Move(posCour,posDest, statistique);
            auto rats_apres = map.getListeRat();
            ++m[msg.source].nbDemandes;
            if (newPos==posDest) {
              ++m[msg.source].nbMouvAcceptes;
            }
            auto maintenant = system_clock::now();
            auto diff_temps = maintenant - dernier_map_stat;
            auto diff_secondes = duration_cast<seconds>(diff_temps).count();
            if (diff_secondes > 0) {
              statistique << duration_cast<milliseconds>(diff_temps).count() << "ms depuis la derniere carte" << endl << map << endl;
              dernier_map_stat = maintenant;
            }

            if (rats_apres.empty()) {
              SUICIDE_COLLECTIF(mpi, space, ep, msg, lt_avant);
              return;
            }

            if (rats_avant.size() != rats_apres.size()) {
              size_t i = 0;
              for (; i != rats_apres.size(); ++i) {
                if (rats_avant[i] != rats_apres[i]) {
                  EXTERMINER(ep, lt_avant, rats_avant[i], msg);
                  break;
                }
              }
              if (i == rats_apres.size()) {
                EXTERMINER(ep, lt_avant, rats_avant[i], msg);
              }
            }

            if (map.getListeFromage().empty()) {
              SUICIDE_COLLECTIF(mpi, space, ep, msg, lt_avant);
              return;
            } else {
              if (newPos==posDest) {
                //cout << mpi << newPos << msg.source << endl << map << endl;
                // Broadcast de la map
                for (auto r: map.getLookupTable()) {
                  stringstream mapss {};
                  mapss << map.getLookupTable().at(r.first);
                  mapss << map;
                  ep.reply(msg, r.first, MMT_DO, mapss.str());
                }
              }
            }
          });
      ep.add_handler(MMT_SPECIAL,
          [&](const mpi_message& msg) {
            statistique << "Le processus " << msg.source << " a fait MIAOUX à " << msg.comment << endl;
            for (auto r : map.getListeRat()) {
              auto rank = map.getRankPosition(r);
              stringstream sss {};
              sss << msg.comment << r << map;
              ep.reply(msg, rank, MMT_SPECIAL, sss.str());
            }
          });

      // start handling received messages
      ep.start(space.comm);
      ep.prune(space.comm);
      cout << map << endl;

      ofstream fichier("diagnostic.txt", ios::out | ios::trunc);

      if (fichier) {
        int numeroProcessus = 0;
        for (auto it = m.get(); it != m.get() + qty_r + qty_c; ++it) {
          double proportion = (double) it->nbMouvAcceptes / it->nbDemandes;
          statistique << "Le processus " << numeroProcessus << " a fait "
              << it->nbDemandes << " demandes de mouvements" << endl;
          statistique << "----Sa proportion de mouvements acceptés est: "
              << proportion << endl;
          ++numeroProcessus;
        }
        auto diff_temps = system_clock::now() - debut_root;
        auto diff_secondes = duration_cast<milliseconds>(diff_temps).count();
        statistique << "Le temps total d'éxécution est " << diff_secondes
            << "ms" << endl;
        fichier << statistique.str() << std::flush;
        fichier << map << endl;
      } else {
        cerr << "Erreur à l'ouverture !" << endl;
      }
    }
  }
}

