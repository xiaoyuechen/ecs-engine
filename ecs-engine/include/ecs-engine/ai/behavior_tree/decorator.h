#pragma once

#include <memory>

#include "ecs-engine/ai/behavior_tree/detail/single_node_owner.h"
#include "ecs-engine/ai/behavior_tree/node.h"

namespace ecs {
namespace ai {
namespace bt {

class Decorator : public Node, public SingleNodeOwner {};

class Inverter : public Decorator {
 public:
  virtual const Result& Run(float dt, const EntitySet& etts) override;
};

class Succeeder : public Decorator {
 public:
  virtual const Result& Run(float dt, const EntitySet& etts) override;
};

}  // namespace bt
}  // namespace ai
}  // namespace ecs