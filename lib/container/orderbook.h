#pragma once

#include <vector>
#include <map>
#include <cstdint>
#include <algorithm>
#include <absl/container/btree_map.h>

using Price = uint64_t;
using Volume = uint64_t;

class orderbook {
public:
  std::map<Price, Volume, std::greater<Price>> bids_;
  std::map<Price, Volume, std::less<Price>> asks_;
};

class btreeOrderbook {
  absl::btree_map<Price, Volume, std::greater<Price>> bids_;
  absl::btree_map<Price, Volume, std::less<Price>> asks_;
};

using VectorLevels = std::vector<std::pair<Price, Volume>>;

class vectorOrderbook {
  VectorLevels bids_;
  VectorLevels asks_;
};

template<class T = std::map<Price, Volume>>
typename T::iterator AddOrder(T& levels, Price price, Volume volume) {
  auto [it, inserted] = levels.try_emplace(price, volume);
  if (inserted == false) {
    it->second += volume;
  }
  return it;
}

template<class Compare = std::greater<Price>>
void AddOrder(VectorLevels& levels, Price price, Volume volume, Compare comp) {
  auto it = std::lower_bound(levels.begin(), levels.end(), price, [comp](const auto& p, Price price) {
    return comp(p.first, price);
  });
  if ((it != levels.end()) && (it->first == price)) {
    it->second += volume;
  } else {
    levels.insert(it, {price, volume});
  }

};

template<class T = std::map<Price, Volume>>
typename T::iterator ReplaceLevel(T& levels, Price price, Volume volume) {
  auto [it, _] = levels.insert_or_assign(price, volume);
  return it;
}

template<class Compare = std::greater<Price>>
void ReplaceLevel(VectorLevels& levels, Price price, Volume volume, Compare comp) {
  auto it = std::lower_bound(levels.begin(), levels.end(), price, [comp](const auto& p, Price price) {
    return comp(p.first, price);
  });
  if ((it != levels.end()) && (it->first == price)) {
    it->second = volume;
  } else {
    levels.insert(it, {price, volume});
  }
}

template<class T = std::map<Price, Volume>>
void DeleteOrder(typename T::iterator it, T& levels, Price price, Volume volume) {
  it->second -= volume;
  if (it->second <= 0) {
    levels.erase(it);
  }
}

template<class Compare = std::greater<Price>>
void DeleteOrder(VectorLevels& levels, Price price, Volume volume, Compare comp) {
  auto it = std::lower_bound(levels.begin(), levels.end(), price, [comp](const auto& p, Price price) {
    return comp(p.first, price);
  });
  if ((it != levels.end()) && (it->first == price)) {
    it->second -= volume;
    if (it->second <= 0) {
      levels.erase(it);
    }
  }
}

template<class T = std::map<Price, Volume>>
void RemoveLevel(typename T::iterator it, T& levels, Price price) {
  levels.erase(it);
}

template<class Compare = std::greater<Price>>
void RemoveLevel(VectorLevels& levels, Price price, Volume volume, Compare comp) {
  auto it = std::lower_bound(levels.begin(), levels.end(), price, [comp](const auto& p, Price price) {
    return comp(p.first, price);
  });
  if ((it != levels.end()) && (it->first == price)) {
    levels.erase(it);
  }
}