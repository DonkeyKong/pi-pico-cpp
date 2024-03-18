#pragma once

#include "Math.hpp"
#include <stdint.h>
#include <cmath>

template <typename T>
struct Vec2
{
  T x;
  T y;

  template <typename U>
  Vec2(const Vec2<U>& other)
  {
    x = (T)other.x;
    y = (T)other.y;
  }

  template <typename U>
  Vec2(U x, U y)
  {
    x = (T)x;
    y = (T)y;
  }

  inline Vec2<T> operator-(const Vec2<T>& rhs) const
  {
    return {x-rhs.x, y-rhs.y};
  }

  inline Vec2<T> operator+(const Vec2<T>& rhs) const
  {
    return {x+rhs.x, y+rhs.y};
  }

  inline Vec2<T> operator*(T rhs) const
  {
    return {x*rhs, y*rhs};
  }

  inline Vec2<T> operator*(const Vec2<T>& rhs) const
  {
    return {x*rhs.x, y*rhs.y};
  }

  inline Vec2<T> operator/(const Vec2<T>& rhs) const
  {
    return {x/rhs.x, y/rhs.y};
  }

  inline Vec2<T> transpose() const
  {
    return {y, x};
  }

  inline T getElementCloserToZero() const
  {
    return std::abs(x) < std::abs(y) ? x : y;
  }

  inline T getElementFartherFromZero() const
  {
    return std::abs(x) > std::abs(y) ? x : y;
  }

  template <typename OutT = T>
  inline OutT norm() const
  {
    return std::sqrt(std::pow((OutT)x, (OutT)2) + std::pow((OutT)y, (OutT)2));
  }

  template <typename OutT = T>
  inline OutT dist(const Vec2<T>& v) const
  {
    return std::sqrt(std::pow((OutT)(x-v.x), (OutT)2) + std::pow((OutT)(y-v.y), (OutT)2));
  }

  bool inside(const Vec2<T>& v) const
  {
    return between(x, 0, v.x) && between(y, (T)0, v.y);
  }
};

typedef Vec2<float> Vec2f;
typedef Vec2<double> Vec2d;
typedef Vec2<int64_t> Vec2i;

struct Vec3f
{
  float X = 0.0f;
  float Y = 0.0f;
  float Z = 0.0f;

  bool operator==(const Vec3f& other)
  {
    return X == other.X && Y == other.Y && Z == other.Z;
  }

  bool operator!=(const Vec3f& other)
  {
    return X != other.X || Y != other.Y || Z != other.Z;
  }

  Vec3f operator* (float c) const
  {
    return {c*X, c*Y, c*Z};
  }

  Vec3f normalize()
  {
    float n = norm();
    return {X / n, Y / n, Z / n};
  }

  float norm() const
  {
    return std::sqrt(std::pow(X, 2.0f) + std::pow(Y, 2.0f) + std::pow(Z, 2.0f));
  }
};