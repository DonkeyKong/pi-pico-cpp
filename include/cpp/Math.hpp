#pragma once

#include <algorithm>

struct Vect2f
{
  float x = 0;
  float y = 0;
};

struct Vect3f
{
  float x = 0;
  float y = 0;
  float z = 0;
};

template <typename T>
inline bool between(T val, T bound1, T bound2)
{
  if (bound1 > bound2) return val >= bound2 && val <= bound1;
  return val >= bound1 && val <= bound2;
}

template <typename T>
float normalize3Point(T val, T negOne, T zero, T posOne)
{
  val = std::clamp(val, negOne, posOne);
  if (val < zero)
  {
    return -std::abs((float)zero - (float)val) / std::abs((float)zero - (float)negOne);
  }
  else if (val > zero)
  {
    return std::abs((float)val - (float)zero) / std::abs((float)posOne - (float)zero);
  }
  return 0.0f;
}

// In-place value clamping
template <typename T>
void clamp(T& val, T lo, T hi)
{
  val = std::clamp(val, lo, hi);
}

template <typename T>
T moveTowards(T val, T dest, T inc)
{
  if (val < (dest-inc))
  {
    return val + inc;
  }
  if (val > (dest+inc))
  {
    return val - inc;
  }
  return dest;
}

template <typename InT, typename OutT>
inline OutT remap(InT val, InT inA, InT inB, OutT outA, OutT outB)
{
  return (OutT)(val - inA) / (OutT)(inB - inA) * (outB - outA) + outA;
}

// This isn't needed yet so it's untested but I think it's correct
// template <typename InT>
// float invLerpF(InT val, InT inA, InT inB, bool clamp = true)
// {
//   float t = (float)(val - inA) / (float)(inB - inA);
//   if (clamp) return std::clamp(t, std::min<float>(inA, inB), std::max<float>(inA, inB));
//   return t;
// }