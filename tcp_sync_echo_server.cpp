#include "match_finder.hpp"

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::asio;
using namespace boost::posix_time;
using boost::system::error_code;

io_service service;
size_t read_complete(char * buff, const error_code & err, size_t bytes) {
  if ( err) return 0;
  bool found = std::find(buff, buff + bytes, '\n') < buff + bytes;
  // we read one-by-one until we get to enter, no buffering
  return found ? 0 : 1;
}

bool prefix_match(const std::string &str, const std::string &prefix) {
  if (str.size() < prefix.size()) return false;
  for (size_t pos = 0; pos < prefix.size(); ++pos) {
    if (str[pos] != prefix[pos]) return false;
  }
  return true;
}

void handle_connections(int context_id) {
  std::string contest_id_str(6, '\0');
  sprintf(&contest_id_str[0], "%06d", context_id);

  shingle_storage_t shingle_storage("shingles/"+contest_id_str+".all.txt");
  shingle_storage_t audit_storage("shingles/"+contest_id_str+".audit.all.txt");

  ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(),8001));
  char buff[1024];

  while (true) {
    ip::tcp::socket sock(service);
    acceptor.accept(sock);
    boost::asio::streambuf buf;
    read_until(sock, buf, "\n");
    std::string msg( (std::istreambuf_iterator<char>(&buf)), std::istreambuf_iterator<char>() );
    //std::string msg(buff, bytes);

    if (prefix_match(msg, "exit")) {
      break;
    } else if (prefix_match(msg, "echo")) {
      sock.write_some(buffer(msg));
    } else if (prefix_match(msg, "reload")) {
      shingle_storage.reload();
    } else if (prefix_match(msg, "find")) {
      std::vector<std::string> parts;
      boost::split(parts, msg, boost::is_any_of(" \t\n"));
      
      for (size_t i = 1; i < parts.size(); ++i) {
        if (parts[i].empty()) continue;
        std::string ans = shingle_storage.find(parts[i]);
        sock.write_some(buffer(ans));
//        sock.write_some(buffer("\n")); 
      }
    } else if (prefix_match(msg, "audit")) {
      std::vector<std::string> parts;
      boost::split(parts, msg, boost::is_any_of(" \t\n"));
      
      for (size_t i = 1; i < parts.size(); ++i) {
        if (parts[i].empty()) continue;
        std::string ans = audit_storage.find(parts[i]);
        sock.write_some(buffer(ans)); 
//        sock.write_some(buffer("\n")); 
      }
    } else {
      sock.write_some(buffer("Unexpected command"));
    }
    sock.close();
  }
}

int main(int argc, char* argv[]) {
 
  if (argc < 2) {
    std::cerr << "USAGE: " << argv[0] << " <contest_id>" << std::endl; 
    return 1;
  }
  int contest_id = 0;
  if (1 != sscanf(argv[1], "%d", &contest_id)) {
    std::cerr << "bad contest_id: " << argv[1] << std::endl;
    return 1;
  }
  handle_connections(contest_id);

  return 0;
}

