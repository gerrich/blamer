#include <vector>
#include <string>
#include <utility>
#include "plane_text_index/mmap.hpp"
#include "plane_text_index/algo.hpp"
#include "plane_text_index/slice-tools.hpp"

typedef std::vector<std::string> str_list_t;

struct shingle_storage_t {
  std::string fname;
  mmap_t mmap;
 
  shingle_storage_t(const std::string& _fname)
    :fname(_fname)
  {
    mmap.load(fname.c_str());
  }

  ~shingle_storage_t() {
    mmap.free();
  }

  std::string find(std::string &key) const {
    slice_t key_slice(&key[0], key.size());
    word_start_record_less_t less;
    slice_t ans = lower_bound_line((char*)mmap.data, mmap.size, key_slice, less);
    std::string result;
    for(; !less(key, ans); ans = next_slice(ans, (char*)mmap.data + mmap.size)) {
      result += std::string(ans.ptr, ans.size) + "\n";
    }
    return result;
  }
  
  void reload() {
    mmap_t new_mmap;
    new_mmap.load(fname.c_str());
    mmap.free();
    std::swap(mmap, new_mmap);
  }
};


