#ifndef _Util_h
#define _Util_h

#include <string>
#include <vector>
#include <chrono>

const int COORDINATE_SCALE=1000;
const int LIGHT_HITSTOP=12;
const int MEDIUM_HITSTOP=16;
const int HEAVY_HITSTOP=20;

using FPS = std::chrono::duration<int, std::ratio<1, 60>>;

class Util {
public:

  static std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start - 1);
        pos_start = pos_end + delim_len+1;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

private:
  Util();
  ~Util();
  /* data */
};

#endif
