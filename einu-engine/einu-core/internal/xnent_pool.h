#pragma once

#include <einu-util/object_pool.h>

#include <array>
#include <cassert>
#include <variant>

#include "einu-core/i_xnent_pool.h"
#include "einu-core/xnent_list.h"

namespace einu {
namespace internal {

template <typename Comp>
class OneXnentPool {
 public:
  using size_type = std::size_t;
  using GrowthFunc = std::function<size_type(size_type)>;

  void SetValue(std::unique_ptr<Xnent> value) noexcept {
    auto ptr = value.release();
    auto v = std::unique_ptr<Comp>(reinterpret_cast<Comp*>(ptr));
    pool_.SetValue(std::move(v));
  }

  void SetGrowth(GrowthFunc growth) noexcept { pool_.SetGrowth(growth); }

  void GrowExtra(size_type delta_size) { pool_.GrowExtra(delta_size); }

  Xnent& Acquire() { return pool_.Acquire(); }

  void Release(Xnent& obj) noexcept {
    auto& comp = reinterpret_cast<Comp&>(obj);
    auto val = pool_.GetValue();
    if (val) {
      comp = *val;
    } else {
      comp = Comp{};
    }
    pool_.Release(comp);
  }

  size_type Size() const noexcept { return pool_.Size(); }

 private:
  using Pool = util::DynamicPool<Comp>;
  Pool pool_;
};

template <typename ComponentList>
class XnentPool;

template <typename... Xnents>
class XnentPool<XnentList<Xnents...>> final : public IXnentPool {
 public:
  XnentPool()
      : pool_table_{OneXnentPool<Xnents>{}...} {}

 private:
  using TypeList = tmp::TypeList<Xnents...>;
  using PoolVariant = std::variant<OneXnentPool<Xnents>...>;
  using PoolTable = std::array<PoolVariant, tmp::Size<TypeList>::value>;

  void AddPolicyImpl(size_type init_size, std::unique_ptr<Xnent> value,
                     GrowthFunc growth_func, XnentTypeID id) override {
    std::visit(
        [&](auto&& arg) {
          arg.SetValue(std::move(value));
          arg.SetGrowth(growth_func);
          arg.GrowExtra(init_size);
        },
        pool_table_[id]);
  }

  Xnent& AcquireImpl(XnentTypeID id) override {
    return std::visit(
        [](auto&& arg) -> auto& { return arg.Acquire(); }, pool_table_[id]);
  }

  void ReleaseImpl(XnentTypeID id, Xnent& comp) noexcept override {
    std::visit([&comp](auto&& arg) { arg.Release(comp); }, pool_table_[id]);
  }

  size_type OnePoolSizeImpl(XnentTypeID id) const noexcept override {
    return std::visit([](auto&& arg) { return arg.Size(); }, pool_table_[id]);
  }

  PoolTable pool_table_;
};

}  // namespace internal
}  // namespace einu