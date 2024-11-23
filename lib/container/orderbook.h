#ifndef CPPTRADING_LIB_CONTAINER_ORDERBOOK_H_
#define CPPTRADING_LIB_CONTAINER_ORDERBOOK_H_

#include <vector>
#include <map>

class orderbook {
public:
  struct level {
    double price;
    double size;
  };
  std::vector<int> bids;
  std::vector<int> asks;

  std::map<int, level> bids_;
};

#endif //CPPTRADING_LIB_CONTAINER_ORDERBOOK_H_
