#ifndef _Util_h
#define _Util_h

#include <string>
#include <vector>
#include <chrono>
#include <godot_cpp/variant/utility_functions.hpp>

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


//Fletcher 32 checksum for rollbacks
static int fletcher32_checksum(short *data, size_t len) {
   int sum1 = 0xffff, sum2 = 0xffff;

   while (len) {
      size_t tlen = len > 360 ? 360 : len;
      len -= tlen;
      do {
         sum1 += *data++;
         sum2 += sum1;
      } while (--tlen);
      sum1 = (sum1 & 0xffff) + (sum1 >> 16);
      sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   /* Second reduction step to reduce sums to 16 bits */
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   return sum2 << 16 | sum1;
}


private:
  Util();
  ~Util();
  /* data */
};

#endif
