#include "cache.hpp"

std::mutex MockBlockCache::mutex;
std::condition_variable MockBlockCache::cv;
std::atomic<usize> MockBlockCache::oracle, MockBlockCache::top;
std::unordered_map<usize, bool> MockBlockCache::board;
MockBlockCache::Meta MockBlockCache::tmpv[num_blocks];
MockBlockCache::Meta MockBlockCache::memv[num_blocks];
MockBlockCache::Cell MockBlockCache::tmp[MockBlockCache::num_blocks];
MockBlockCache::Cell MockBlockCache::mem[MockBlockCache::num_blocks];
