#pragma once

#include <utility>
#include <string>
#include <locale>
#include <codecvt>

struct hash_pair {
  template <class T1, class T2>
  size_t operator()(const std::pair<T1, T2>& p) const
  {
    auto hash1 = std::hash<T1>{}(p.first);
    auto hash2 = std::hash<T2>{}(p.second);
	  return hash1 == hash2 ? hash1 : (hash1^hash2);
  }
};

namespace utils
{

inline std::string widestring2string(const std::wstring &string)
{
  using convert_type = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_type, wchar_t> converter;
  return converter.to_bytes(string);
}

inline std::wstring string2widestring(const std::string& string)
{
  using convert_type = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_type, wchar_t> converter;
  return converter.from_bytes(string);
}

}
