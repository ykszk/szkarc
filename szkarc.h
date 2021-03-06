#ifndef SZKARC_H
#define SZKARC_H
#include <cstdint>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <filesystem>

#define CONCATENATE(e1, e2) e1 ## e2

#ifdef _WIN32
#define WCOUT std::wcout
#define WSTRING wstring
#define WPREFIX(s) CONCATENATE(L, s)
#else
#define WCOUT std::cout
#define WSTRING u8string
#define WPREFIX(s) s
#endif

int get_physical_core_counts();
int32_t stream_os_open(void* stream, const std::filesystem::path& path, int32_t mode);

template <typename T>
std::vector<T> flatten_nested(const std::vector<std::vector<T>>& nested) {
  using ListType = std::vector<T>;
  size_t total_size = std::transform_reduce(nested.cbegin(), nested.cend(), 0u, std::plus{}, [](const ListType& l) {return l.size(); });
  ListType flat;
  flat.reserve(total_size);
  for (auto& subd : nested) {
    flat.insert(
      flat.end(),
      std::make_move_iterator(subd.begin()),
      std::make_move_iterator(subd.end())
    );
  }
  return flat;
}

using PathList = std::vector<std::filesystem::path>;
PathList list_subdirs(const std::filesystem::path& indir, int depth, bool all, bool include_files);

#ifdef _WIN32
std::string wstr2utf8(std::wstring const& src);
class local_setmode {
private:
  int prev_mode;
public:
  local_setmode();
  ~local_setmode();
};
#else
class local_setmode {
  // do nothing
};
#endif


#endif /* SZKARC_H */
