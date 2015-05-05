#include "match_finder.hpp"

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace boost::asio;
using namespace boost::posix_time;
using boost::system::error_code;

io_service service;

bool prefix_match(const std::string &str, const std::string &prefix) {
  if (str.size() < prefix.size()) return false;
  for (size_t pos = 0; pos < prefix.size(); ++pos) {
    if (str[pos] != prefix[pos]) return false;
  }
  return true;
}

typedef boost::shared_ptr<shingle_storage_t> shingle_storage_ptr_t;
typedef std::vector<shingle_storage_ptr_t> shingle_storage_ptr_list_t;

void insert_nth_column(ip::tcp::socket &sock, size_t field_pos, const std::string &value, const std::string &str) {
  int state = 0;
  size_t pos = 0;
  size_t prev_pos = 0;
  size_t field_id = 0;
  const char *data = str.c_str();
  for (; pos < str.size(); ++pos) {
    switch(data[pos]) {
      case '\t':
        {
          field_id += 1;
        }
        break;
      case '\n':
        {
          field_id = 0;    
        }
        break;
      default:
        {
          if (field_id == field_pos) {
            sock.write_some(buffer(&data[prev_pos], pos - prev_pos));

            prev_pos = pos;
            sock.write_some(buffer(value));
            sock.write_some(buffer("\t", 1));
            ++field_id;
          }
        }
        break;
    }
  }
  sock.write_some(buffer(&data[prev_pos], pos - prev_pos));
} 

struct shingle_server_t {
  const std::vector<int> &context_list;
  shingle_storage_ptr_list_t shingle_storage_list;
  shingle_storage_ptr_list_t audit_storage_list;

  shingle_server_t(const std::vector<int> &_context_list)
      : context_list(_context_list)
  {
    for (size_t i = 0; i < context_list.size(); ++i) {
      std::string contest_id_str(6, '\0');
      sprintf(&contest_id_str[0], "%06d", context_list[i]);

      shingle_storage_list.push_back(boost::make_shared<shingle_storage_t>("shingles/"+contest_id_str+".all.txt"));
      audit_storage_list.push_back(boost::make_shared<shingle_storage_t>("shingles/"+contest_id_str+".audit.all.txt"));
    }
  }

  int do_audit(ip::tcp::socket &sock, const std::string& msg);
  void handle_connections();
};


    
int shingle_server_t::do_audit(ip::tcp::socket &sock, const std::string &msg) {
  std::vector<std::string> parts;
  boost::split(parts, msg, boost::is_any_of(" \t\n"));
 
  if (parts.size() < 2) { return 1; }
  
  int contest_id = 0;
  if(1 != sscanf(parts[1].c_str(), "%d", &contest_id)) {
    return 1;
  }
  std::vector<int>::const_iterator it = std::find(context_list.begin(), context_list.end(), contest_id);
  if (it == context_list.end()) { return 1; }
  int id = std::distance(context_list.begin(), it);

  const shingle_storage_t &audit_storage = *audit_storage_list[id];
  for (size_t i = 2; i < parts.size(); ++i) {
    if (parts[i].empty()) continue;
    std::string ans = audit_storage.find(parts[i]);
    sock.write_some(buffer(ans)); 
  }

  return 0;
}

void shingle_server_t::handle_connections() {
  ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(),8001));

  while (true) {
    ip::tcp::socket sock(service);
    acceptor.accept(sock);
    boost::asio::streambuf buf;
    read_until(sock, buf, "\n");
    std::string msg( (std::istreambuf_iterator<char>(&buf)), std::istreambuf_iterator<char>() );

    if (prefix_match(msg, "exit")) {
      break;
    } else if (prefix_match(msg, "echo")) {
      sock.write_some(buffer(msg));
    } else if (prefix_match(msg, "reload")) {
      for (size_t ctx_id = 0; ctx_id < context_list.size(); ++ctx_id) { 
        shingle_storage_list[ctx_id]->reload();
        audit_storage_list[ctx_id]->reload();
      }
    } else if (prefix_match(msg, "find")) {
      std::vector<std::string> parts;
      boost::split(parts, msg, boost::is_any_of(" \t\n"));
     
      for (size_t ctx_id = 0; ctx_id < context_list.size(); ++ctx_id) {
        std::string contest_id_str(6, '\0');
        sprintf(&contest_id_str[0], "%06d", context_list[ctx_id]);

        const shingle_storage_t &shingle_storage = *shingle_storage_list[ctx_id];
        for (size_t i = 1; i < parts.size(); ++i) {
          if (parts[i].empty()) continue;
          std::string ans = shingle_storage.find(parts[i]);
          //sock.write_some(buffer(ans));
          insert_nth_column(sock, 1, contest_id_str, ans);
        }
      }
    } else if (prefix_match(msg, "audit")) {
      do_audit(sock, msg);
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
  std::vector<int> contest_list;
  for (int i = 1; i < argc; ++i) {
    int value;
    if (1 != sscanf(argv[i], "%d", &value)) {
      std::cerr << "bad contest_id: " << argv[1] << std::endl;
      return 1;
    }
    contest_list.push_back(value);
  }

  shingle_server_t server(contest_list);
  server.handle_connections();

  return 0;
}

