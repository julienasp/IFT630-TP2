/*
 * mpi.cpp
 *
 *  Created on: 2016-08-04
 *      Author: taig1501
 */

#include <algorithm>
#include <iostream>
#include <cstring>
#include <memory>
#include <unistd.h>

#include "mpi.h"

using namespace std;

mpi_message::mpi_message()
    : tag { }, source { }, comment { }, comm { } {
}

mpi_server::mpi_server(int *argc, char ***argv)
    : id_ { random_string(8) } {
  MPI_Init(argc, argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
  MPI_Comm_size(MPI_COMM_WORLD, &size_);
  MPI_Comm_get_parent(&parent_);
#if MPI_VERBOSE
  cout << id_ << "[" << rank_ << "] MPI Initialized" << endl;
#endif
}

mpi_server::~mpi_server() {
#if MPI_VERBOSE
  cout << id_ << "[" << rank_ << "] MPI Finalizing..." << endl;
#endif
  if (parent_ != MPI_COMM_NULL) {
    MPI_Comm_disconnect(&parent_);
  }
  MPI_Finalize();

#if MPI_VERBOSE
  cout << id_ << "[" << rank_ << "] MPI Finalized" << endl;
#endif
}

int mpi_server::rank() const noexcept {
  return rank_;
}

int mpi_server::size() const noexcept {
  return size_;
}

int mpi_server::size(MPI_Comm comm) const noexcept {
  int size;
  MPI_Comm_remote_size(comm, &size);
  return size;
}

const string& mpi_server::id() const noexcept {
  return id_;
}

MPI_Comm mpi_server::parent() const noexcept {
  return parent_;
}

size_t mpi_server::spawn(int qty, char* cmd, vector<char*> && args,
    MPI_Comm *intercomm) {
  int failed { qty };
  unique_ptr<int[]> merrc { new int[qty] }; // clean up automatically
  int *errc = merrc.get();

  MPI_Comm_spawn(cmd, &args[0], qty, MPI_INFO_NULL, rank(), MPI_COMM_SELF,
      intercomm, errc);
  for (int *it = errc; it != errc + qty; ++it) {
    if (*it == MPI_SUCCESS) {
      --failed;
    }
  }
  return failed;
}

void mpi_server::send_string(MPI_Comm comm, int target, int tag, string &&msg) {
  auto len = strlen(msg.c_str()) + 1;
  unique_ptr<char[]> buff { new char[len] { } };
  strcpy(buff.get(), msg.c_str());
  MPI_Send(buff.get(), len - 1, MPI_CHAR, target, tag, comm);
}

pair<char*, MPI_Request> mpi_server::forward_string(MPI_Comm comm, int target,
    int tag, string &&msg) {
  MPI_Request reqs { };
  char *buff;
  try {
    auto len = strlen(msg.c_str()) + 1;
    buff = new char[len] { };
    strcpy(buff, msg.c_str());
    MPI_Isend(buff, len - 1, MPI_CHAR, target, tag, comm, &reqs);
    return {buff, reqs};
  } catch (...) {
    delete[] buff;
    throw;
  }
}

string mpi_server::recv_string(MPI_Comm comm, int source, int tag,
    MPI_Status* status) {
  probe(comm, source, tag, status);
  int recv;
  MPI_Get_count(status, MPI_CHAR, &recv);
  unique_ptr<char[]> buff { new char[recv] }; // clean up automatically
  MPI_Recv(buff.get(), recv, MPI_CHAR, status->MPI_SOURCE, status->MPI_TAG,
      comm, status);
  string msg { buff.get(), buff.get() + recv };
  return msg;
}

string mpi_server::recv_string(MPI_Comm comm, int source, int tag) {
  MPI_Status status;
  return recv_string(comm, source, tag, &status);
}

void mpi_server::send_message(MPI_Comm comm, int target, int tag,
    string&& comment) {
  send_string(comm, target, tag, move(comment));
}

std::pair<char*, MPI_Request> mpi_server::forward_message(MPI_Comm comm,
    int target, int tag, std::string&& comment) {
  return forward_string(comm, target, tag, move(comment));
}

mpi_message mpi_server::recv_message(MPI_Comm comm, int source, int tag) {
  mpi_message mm { };
  MPI_Status status;
  mm.comment = recv_string(comm, source, tag, &status);
  mm.source = status.MPI_SOURCE;
  mm.tag = status.MPI_TAG;
  mm.comm = comm;
  return mm;
}

bool mpi_server::probe(const mpi_message& msg) const noexcept {
  return probe(msg.comm, msg.source, msg.tag);
}
bool mpi_server::probe(MPI_Comm comm, int source, int tag) const noexcept {
  int flag;
  // Iprobe (almost) never returns true on the first call
  // http://stackoverflow.com/q/34059832
  for (int i = 0; i != 9; ++i) {
    MPI_Iprobe(source, tag, comm, &flag, MPI_STATUS_IGNORE);
  }
  return flag;
}

bool mpi_server::probe(MPI_Comm comm, int source, int tag,
    MPI_Status* status) const noexcept {
  int flag = MPI_Probe(source, tag, comm, status);
  return flag;
}

mpi_endpoint::mpi_endpoint(mpi_server* s, handler_type&& dh)
    : listenning { }, server { s }, pending_replies { max_reply_slots,
    MPI_REQUEST_NULL }, pending_replies_buffer { max_reply_slots, nullptr }, reply_slots { }, routing_table { }, default_handler {
        dh } {
}

mpi_endpoint::~mpi_endpoint() {
  MPI_Waitall(pending_replies.size(), &pending_replies[0], MPI_STATUSES_IGNORE);
  for (int i = 0; i != reply_slots; ++i) {
    delete[] pending_replies_buffer[i];
  }
}

void mpi_endpoint::add_handler(message_tag_type mt, handler_type&& h) {
  routing_table.emplace(mt, h);
}

void mpi_endpoint::start(MPI_Comm comm) {
  listenning = true;
  while (listenning) {
    mpi_message msg = server->recv_message(comm, MPI_ANY_SOURCE, MPI_ANY_TAG);
    auto handler_range = routing_table.equal_range(msg.tag);
    if (handler_range.first == handler_range.second) {
      default_handler(msg);
    } else {
      for (auto it = handler_range.first; it != handler_range.second; ++it) {
        it->second(msg);
      }
    }
  }
}

void mpi_endpoint::prune(MPI_Comm comm) {
  while (server->probe(comm, MPI_ANY_SOURCE, MPI_ANY_TAG)) {
    server->recv_message(comm, MPI_ANY_SOURCE, MPI_ANY_TAG);
  }
  cout << *server << "Waiting Barrier..." << endl;
  MPI_Barrier(comm);
  cout << *server << "Jumped Barrier" << endl;
  /*
   for (int i = 0; i != reply_slots; ++i) {
   if (pending_replies[i] != MPI_REQUEST_NULL) {
   MPI_Cancel(&pending_replies[i]);
   cout << *server << "delete[] in mpi_endpoint::prune()" << endl;
   delete[] pending_replies_buffer[i];
   }
   }*/
}

void mpi_endpoint::reply(const mpi_message& msg, std::string&& comment) {
  reply(msg, msg.source, msg.tag, move(comment));
}

void mpi_endpoint::reply(const mpi_message& msg, int tag,
    std::string&& comment) {
  reply(msg, msg.source, tag, move(comment));
}

void mpi_endpoint::reply(const mpi_message& msg, int source, int tag,
    std::string&& comment) {
  auto reqs = server->forward_message(msg.comm, source, tag, move(comment));
  int i = ++reply_slots;
  if (i >= max_reply_slots) {
    --reply_slots;
    MPI_Waitany(pending_replies.size(), &pending_replies[0], &i,
    MPI_STATUS_IGNORE);
    delete[] pending_replies_buffer[i];
  }
  pending_replies_buffer[i] = reqs.first;
  pending_replies[i] = reqs.second;
}

void mpi_endpoint::request_stop() {
  listenning = false;
#if MPI_VERBOSE
  cout << *server << "Stopping..." << endl;
#endif
  MPI_Waitall(pending_replies.size(), &pending_replies[0], MPI_STATUSES_IGNORE);
}

mpi_unique_comm::~mpi_unique_comm() {
  MPI_Comm_disconnect(&comm);
}

std::ostream& operator<<(std::ostream& os, const mpi_server& that) {
  os << that.id() << "[" << that.rank() << "](" << getpid() << ") ";
  return os;
}

//http://stackoverflow.com/a/12468109
std::string random_string(size_t length) {
  auto randchar = []() -> char
  {
    const char charset[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);
    return charset[ rand() % max_index ];
  };
  std::string str(length, 0);
  std::generate_n(str.begin(), length, randchar);
  return str;
}

