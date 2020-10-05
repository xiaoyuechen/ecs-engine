#pragma once

#include "einu-engine/core/i_component"

namespace einu {

class ISystem {};

class IWorld {
 public:
  virtual void AddSystem(ISystem& system) = 0;
};

template <typename... Ts>
using ComponentList = tmp::SubtypeList<IComponent, Ts...>;

class Engine {
 public:
  template <typename... Ts>
  void RegisterComponents(ComponentList<Ts...>) {

  }

  virtual IWorld MakeWorld() = 0;
};

}  // namespace einu