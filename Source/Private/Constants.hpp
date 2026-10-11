#pragma once
#include <Private/SmallVector.hpp>

namespace RHI
{
template<typename T>
using MultibufferVector = SmallVector<T, 3>;
}
