#pragma once

#include <memory>
#include <tuple>
#include <map>

template <typename RttiObj, typename... Args>
class RttiCache
{
  using IdentifierTuple = std::tuple<Args...>;
  using ItemWeakPtr = std::weak_ptr<RttiObj>;
  using ItemStrongPtr = std::shared_ptr<RttiObj>;
  std::map<IdentifierTuple, ItemWeakPtr> cache_;

public:
  void clean()
  {
    for(auto iter = cache_.begin(); iter != cache_.end(); ) 
    {
      if ((*iter).second.expired()) 
      {
        iter = cache_.erase(iter);
      } 
      else
      {
        ++iter;
      }
    }
  }

  bool contains(Args ...args)
  {
    clean();
    IdentifierTuple id = std::make_tuple(args...);
    return cache_.count(id) > 0;
  }

  ItemStrongPtr get(Args ...args)
  {
    clean();
    IdentifierTuple id = std::make_tuple(args...);
    ItemStrongPtr item;
    if (cache_.count(id) > 0)
    {
      item = cache_[id].lock();
    }
    return item;
  }

  ItemStrongPtr getOrCreate(Args ...args)
  {
    clean();
    IdentifierTuple id = std::make_tuple(args...);
    ItemStrongPtr item;

    if (cache_.count(id) > 0)
    {
      item = cache_[id].lock();
    }

    if (!item)
    {
      item = ItemStrongPtr(new RttiObj(args...));
      cache_[id] = item;
    }

    return item;
  }
};