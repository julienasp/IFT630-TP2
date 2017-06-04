/*
 * mpi.h
 *
 *  Created on: 2016-08-04
 *      Author: taig1501
 */

#ifndef MPI_H_
#define MPI_H_

#include <mpi/mpi.h>

#include <functional>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#define MPI_VERBOSE 1

/*
 #helper
 */
enum mpi_message_tag {
  MMT_LOG, MMT_STOP, MMT_BECOME, MMT_DO, MMT_SPECIAL
};

/*
 message receipt for the mpi_message™ protocol.
 #helper
 */
struct mpi_message {
  int tag, source; // not an MPI_Status!
  std::string comment;
  MPI_Comm comm;
  mpi_message();
};

/*
 Mid-level wrapper over MPI calls and environment setup.
 */
class mpi_server {
  std::string id_;
  MPI_Comm parent_;
  int rank_;
  int size_;
  /*
   Internal functions. close your eyes.
   */
  void send_string(MPI_Comm comm, int target, int tag, std::string&&);
  std::pair<char*, MPI_Request> forward_string(MPI_Comm comm, int target,
      int tag, std::string&&);
  std::string recv_string(MPI_Comm comm, int source, int tag,
      MPI_Status* status);
  std::string recv_string(MPI_Comm comm, int source, int tag);
public:
  mpi_server(int *argc, char ***argv);
  mpi_server(const mpi_server&) = delete;
  ~mpi_server();

  /*
   MPI rank of current process.
   */
  int rank() const noexcept;

  /*
   Size of home of current process.
   */
  int size() const noexcept;

  /*
   Size of remote world of given intercommunicator.
   */
  int size(MPI_Comm comm) const noexcept;

  /*
   Pseudo-random almost unique identifier, very useful for debugging across multiple worlds.
   */
  const std::string& id() const noexcept;

  /*
   MPI_COMM_NULL if current process has no parent, intercommunicator to parent otherwise.
   */
  MPI_Comm parent() const noexcept;

  /*
   Spawns <qty> MPI processes of program <cmd> using <args>. Sets <intercomm> to spawned group.
   */
  size_t spawn(int qty, char* cmd, std::vector<char*>&& args,
      MPI_Comm *intercomm);

  /*
   Sends an std::string using the mpi_message™ protocol.
   #Blocking-call until the message is received on the other end.
   */
  void send_message(MPI_Comm comm, int target, int tag, std::string&&);

  /*
   Forwards an std::string using the mpi_message™ protocol.
   #Non-Blocking-call returns immediately and the message is not guaranteed to be
   sent until all <MPI_Request>s returned have been acted upon.
   */
  std::pair<char*, MPI_Request> forward_message(MPI_Comm comm, int target,
      int tag, std::string&&);

  /*
   Receives an mpi_message™.
   #Blocking-call until a message is received.
   */
  mpi_message recv_message(MPI_Comm comm, int source, int tag);

  /*
   Probes the channel (communicator, tag, source) used the send an mpi_message™ and
   returns true if another message is waiting to be received.
   */
  bool probe(const mpi_message& msg) const noexcept;

  /*
   Probes a channel (communicator, tag, source) and
   returns true if another message is waiting to be received.
   */
  bool probe(MPI_Comm comm, int source, int tag) const noexcept;
  bool probe(MPI_Comm comm, int source, int tag, MPI_Status * status) const
      noexcept;
};

/*
 High-level wrapper using an !!!active instance!!! of mpi_server.

 Automatically handles #Non-Blocking mpi_message™ replies.
 Automatically handles receiving mpi_message™s and calls set
 corresponding mpi_message™ handlers or the default one set during
 construction if no handler is available for the received mpi_message™ tag.
 More than one handler can be added for any given tag, the order in
 which they are called is unspecified.
 */
class mpi_endpoint {
public:
  using message_tag_type = int;
  using handler_type = std::function<void(const mpi_message&)>;
  mpi_endpoint(mpi_server*, handler_type&&);
  mpi_endpoint(const mpi_endpoint&) = delete;
  ~mpi_endpoint();

  /*
   Adds an handler to received an mpi_message™ of <tag>.
   */
  void add_handler(message_tag_type tag, handler_type&&);

  /*
   Starts the endpoint and continuously receives then handles messages.
   Returns only once an handler calls request_stop().
   */
  void start(MPI_Comm);

  /*
   Prunes any remaining messages on the communicator and cancels all remaining requests.
   */
  void prune(MPI_Comm);

  /*
   Replies to an mpi_message™.
   #Non-Blocking-call returns immediately and the message is not guaranteed to be
   sent until the mpi_endpoint has acted upon all of the <MPI_Request>s.
   */
  void reply(const mpi_message& msg, std::string&&);

  /*
   Replies to an mpi_message™ on a different tag.
   #Non-Blocking-call returns immediately and the message is not guaranteed to be
   sent until the mpi_endpoint has acted upon all of the <MPI_Request>s.
   */
  void reply(const mpi_message& msg, int tag, std::string&&);
  void reply(const mpi_message& msg, int source, int tag, std::string&&);

  /*
   Kindly asks the mpi_endpoint to stop. All remaining handlers for the current mpi_message™
   will be called before the mpi_endpoint stops but, no more mpi_message™ will be received.
   */
  void request_stop();
  bool listenning;
  mpi_server* server;

  static const int max_reply_slots = 1024;
  using pending_replies_type = std::vector<MPI_Request>;
  using pending_replies_buffer_type = std::vector<char*>;
  pending_replies_type pending_replies;
  pending_replies_buffer_type pending_replies_buffer;
  int reply_slots;

  using routing_table_type = std::unordered_multimap<message_tag_type, handler_type>;
  routing_table_type routing_table;
  handler_type default_handler;
};

/*
 Single use communicator, get's disconnected by the dtor.
 */
struct mpi_unique_comm {
  MPI_Comm comm;
  mpi_unique_comm() = default;
  mpi_unique_comm(const mpi_unique_comm&) = delete;
  mpi_unique_comm& operator=(const mpi_unique_comm&) = delete;
  ~mpi_unique_comm();
};

std::ostream& operator<<(std::ostream& os, const mpi_server& that);
std::string random_string(size_t length);

#endif /* MPI_H_ */
