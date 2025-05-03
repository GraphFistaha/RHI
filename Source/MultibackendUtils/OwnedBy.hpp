#pragma once
#include <cassert>
#include <utility>


namespace RHI
{
/// @brief object owned by another object
template<typename OwnerObjectT>
struct OwnedBy
{
  /// constructor
  explicit OwnedBy(OwnerObjectT & ctx)
    : m_owner(&ctx)
  {
  }

  /// destructor
  virtual ~OwnedBy() = default;

  /// move constructor
  OwnedBy(OwnedBy && rhs) noexcept
    : m_owner(std::move(rhs.m_owner))
  {
    //rhs.m_owner = nullptr;
  }

  /// move operator
  OwnedBy & operator=(OwnedBy && rhs) noexcept
  {
    if (this != &rhs)
      std::swap(m_owner, rhs.m_owner);
    return *this;
  }

  OwnedBy(const OwnedBy & rhs)
    : m_owner(rhs.m_owner)
  {
  }

  OwnedBy & operator=(const OwnedBy & rhs)
  {
    m_owner = rhs.m_owner;
    return *this;
  }

protected:
  const OwnerObjectT & GetOwner() const & noexcept
  {
    // TODO: insert check that m_owner is valid
    assert(m_owner);
    return *m_owner;
  }

  OwnerObjectT & GetOwner() & noexcept
  {
    // TODO: insert check that m_owner is valid
    assert(m_owner);
    return *m_owner;
  }

private:
  OwnerObjectT * m_owner = nullptr;
};
} // namespace RHI

#define MAKE_ALIAS_FOR_GET_OWNER(Type, NewName)                                                    \
  inline decltype(auto) NewName() const & noexcept                                                 \
  {                                                                                                \
    return OwnedBy<Type>::GetOwner();                                                              \
  }                                                                                                \
  inline decltype(auto) NewName() & noexcept                                                       \
  {                                                                                                \
    return OwnedBy<Type>::GetOwner();                                                              \
  }
