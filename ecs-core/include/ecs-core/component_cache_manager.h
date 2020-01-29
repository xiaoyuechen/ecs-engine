#ifndef COMPONENT_CACHE_MANAGER_H_
#define COMPONENT_CACHE_MANAGER_H_

#include <map>
#include <vector>

#include "ecs-core/component_array.h"
#include "ecs-core/component_setting.h"
#include "ecs-core/entity_id.h"

namespace ecs {

template <typename ComponentSetting>
class ComponentCacheManager {
 public:
  using ComponentMask = typename ComponentSetting::ComponentMask;
  using ComponentArray = ComponentArray<ComponentSetting>;

  void RegisterComponentMask(const ComponentMask& mask);
  void AddEntity(const EntityID& eid,
                 const ComponentMask& mask,
                 const ComponentArray& components);
  void RemoveEntity(const EntityID& eid, const ComponentMask& mask);

  const std::vector<ComponentArray>& GetComponentArrays(
      const ComponentMask& mask) const;
  std::vector<ComponentArray>& GetComponentArrays(const ComponentMask& mask);

 private:
  struct ComponentCache {
    std::map<EntityID, ComponentArray> ett_comp_map{};
    mutable std::vector<ComponentArray> comp_arrs{};
    mutable bool is_expired = false;
  };

  struct ComponentMaskCompare {
    bool operator()(const ComponentMask& lhs, const ComponentMask& rhs) const
        noexcept;
  };

  std::map<ComponentMask, ComponentCache, ComponentMaskCompare> cache_map_{};
};

//////////////////////////////////////////////////////////////////////////

template <typename ComponentSetting>
inline void ComponentCacheManager<ComponentSetting>::RegisterComponentMask(
    const ComponentMask& mask) {
  cache_map_.insert({mask, ComponentCache{}});
}

template <typename ComponentSetting>
inline void ComponentCacheManager<ComponentSetting>::AddEntity(
    const EntityID& eid,
    const ComponentMask& mask,
    const ComponentArray& components) {
  for (auto&& [required_mask, cache] : cache_map_) {
    if ((mask & required_mask) == required_mask) {
      cache.ett_comp_map.insert({eid, components});
      cache.is_expired = true;
    }
  }
}

template <typename ComponentSetting>
inline void ComponentCacheManager<ComponentSetting>::RemoveEntity(
    const EntityID& eid, const ComponentMask& mask) {
  for (auto&& [required_mask, cache] : cache_map_) {
    if ((mask & required_mask) == required_mask) {
      cache.ett_comp_map.erase(eid);
      cache.is_expired = true;
    }
  }
}

template <typename ComponentSetting>
inline const std::vector<
    typename ComponentCacheManager<ComponentSetting>::ComponentArray>&
ComponentCacheManager<ComponentSetting>::GetComponentArrays(
    const ComponentMask& mask) const {
  const auto& cache = cache_map_.at(mask);

  // refresh the component arrays if cache is expired
  if (cache.is_expired) {
    cache.comp_arrs.clear();
    for (const auto& [eid, comp_arr] : cache.ett_comp_map) {
      cache.comp_arrs.push_back(comp_arr);
    }
    cache.is_expired = false;
  }

  return cache.comp_arrs;
}

template <typename ComponentSetting>
inline std::vector<
    typename ComponentCacheManager<ComponentSetting>::ComponentArray>&
ComponentCacheManager<ComponentSetting>::GetComponentArrays(
    const ComponentMask& mask) {
  return const_cast<std::vector<ComponentArray>&>(
      static_cast<const ComponentCacheManager&>(*this).GetComponentArrays(
          mask));
}

template <typename ComponentSetting>
inline bool
ComponentCacheManager<ComponentSetting>::ComponentMaskCompare::operator()(
    const ComponentMask& lhs, const ComponentMask& rhs) const noexcept {
  /**
   * Comparators require strict weak orderings:
   * For all a, comp(a,a)==false
   * If comp(a,b)==true then comp(b,a)==false
   * if comp(a,b)==true and comp(b,c)==true then comp(a,c)==true
   */
  return (lhs & rhs) != lhs;
}

}  // namespace ecs

#endif  // COMPONENT_CACHE_MANAGER_H_
