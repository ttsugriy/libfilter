#include <immintrin.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "filter/taffy-block.hpp"
#include "filter/taffy-cuckoo.hpp"

using namespace std;

using namespace filter;

inline long now() {
  _mm_lfence();
  auto point = chrono::steady_clock::now();
  auto duration = point.time_since_epoch();
  auto nanos = chrono::duration_cast<chrono::nanoseconds>(duration);
  return nanos.count();
}


int FromHex(char x) {
  if (x <= '9') return x - '0';
  else return 10 + x - 'A';
}

void PrintFpp(const TaffyBlockFilter& tbf, const TaffyCuckooFilter& tcf,
              const vector<uint64_t>& raw) {
  random_device d;
  auto d64 = [&d]() { return (static_cast<uint64_t>(d()) << 32) | d(); };
  size_t tbf_found = 0, tcf_found = 0, ftcf_found = 0, raw_found = 0;
  vector<uint64_t> randoms(1000 * 1000);
  for (int i = 0; i < 1000 * 1000; ++i) {
    randoms[i] = d64();
  }
  auto ts1 = now();
  for (int i = 0; i < 1000 * 1000; ++i) {
    tbf_found += tbf.FindHash(randoms[i]);
  }
  auto ts2 = now();
  for (int i = 0; i < 1000 * 1000; ++i) {
    tcf_found += tcf.FindHash(randoms[i]);
  }
  auto ts3 = now();
  auto frozen = tcf.Freeze();
  auto ts4 = now();
  for (int i = 0; i < 1000 * 1000; ++i) {
    ftcf_found += frozen.FindHash(randoms[i]);
  }
  auto ts5 = now();
  for (int i = 0; i < 1000 * 1000; ++i) {
    raw_found += binary_search(raw.begin(), raw.end(), randoms[i]);
  }
  auto ts6 = now();
  cout << tbf_found << "\t" << tcf_found << "\t" << ftcf_found << "\t"  << raw_found << "\t";
  cout << ts2 - ts1 << "\t" << ts3 - ts2 << "\t" << ts4 - ts3 << "\t" << ts5 - ts4 << "\t"
       << ts6 - ts5 << "\t";
  cout << tbf.SizeInBytes() << "\t" << tcf.SizeInBytes() << "\t" << frozen.SizeInBytes() << endl;
}

int main(int argc, char** argv) {
  if (2 != argc) {
    cerr << "no file" << endl;
    return 1;
  }
  ifstream f(argv[1]);
  char sha1[40];
  TaffyBlockFilter tbf = TaffyBlockFilter::CreateWithNdvFpp(1, 0.044);
  TaffyCuckooFilter tcf = TaffyCuckooFilter::CreateWithBytes(1);
  vector<uint64_t> raw;
  size_t count = 0, tbf_nanos = 0, tcf_nanos = 0, raw_nanos = 0;
  uint64_t buffer[1024];
  int buffer_fill = 0;
  while (true) {

    while (buffer_fill < 1024 && f.get(sha1, 41, ':')) {
      uint64_t x = 0;
      for (int i = 0; i < 16; ++i) {
        x = x << 4;
        x = x | FromHex(sha1[39 - i]);
      }
      buffer[buffer_fill] = x;
      ++buffer_fill;
      f.ignore(40, '\n');
    }


    auto ts1 = now();
    for (int i = 0; i < buffer_fill; ++i) tbf.InsertHash(buffer[i]);
    auto ts2 = now();
    tbf_nanos += ts2 - ts1;
    // if (not tbf.FindHash(x)) {
    //   cerr << "not found " << dec << count << hex << x << endl;
    //   return 1;
    // }
    auto ts3 = now();
    for (int i = 0; i < buffer_fill; ++i) tcf.InsertHash(buffer[i]);
    auto ts4 = now();
    tcf_nanos += ts4 - ts3;
    // if (not tcf.FindHash(x)) {
    //   cerr << "not found " << dec << count << hex << x << endl;
    //   return 1;
    // }
    for (int i = 0; i < buffer_fill; ++i) raw.push_back(buffer[i]);
    auto ts5 = now();
    raw_nanos += ts5 - ts4;
    count += buffer_fill;
    if (not(count & (count - 1))) {
      // cout << count << "\t" << tbf.SizeInBytes() << "\t" << tcf.SizeInBytes();
      // cout << "\t" << 1.0 * tbf_nanos / count << "\t" << 1.0 * tcf_nanos / count << endl;
      // PrintFpp(tbf, tcf);
    }
    if (buffer_fill < 1023) break;
    buffer_fill = 0;
  }
  auto ts6 = now();
  sort(raw.begin(), raw.end());
  auto ts7 = now();
  raw_nanos += ts7 - ts6;
  // cout << "done\n";
  // cout << count << "\t" << tbf.SizeInBytes() << "\t" << tcf.SizeInBytes();
  cout << "\t" << 1.0 * tbf_nanos << "\t" << 1.0 * tcf_nanos << "\t" << 1.0 * raw_nanos << "\t";
  PrintFpp(tbf, tcf, raw);
}
