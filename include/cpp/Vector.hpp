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
typedef Vec2<int> Vec2i;

template <typename T>
struct Vec3
{
  T X = 0;
  T Y = 0;
  T Z = 0;

  bool operator==(const Vec3& other) const
  {
    return X == other.X && Y == other.Y && Z == other.Z;
  }

  bool operator!=(const Vec3& other) const
  {
    return X != other.X || Y != other.Y || Z != other.Z;
  }

  Vec3 operator+(const Vec3& other) const
  {
    return {X + other.X , Y + other.Y , + other.Z};
  }

  void operator+=(const Vec3& other)
  {
    X += other.X;
    Y += other.Y;
    Z += other.Z;
  }

  Vec3 operator-(const Vec3& other) const
  {
    return {X - other.X , Y - other.Y , - other.Z};
  }

  Vec3 operator* (float c) const
  {
    return {(T)(c*(float)X), (T)(c*(float)Y), (T)(c*(float)Z)};
  }

  Vec3 normalize() const
  {
    float n = norm();
    return {(T)((float)X / n), (T)((float)Y / n), (T)((float)Z / n)};
  }

  float norm() const
  {
    return std::sqrt(std::pow((float)X, 2.0f) + std::pow((float)Y, 2.0f) + std::pow((float)Z, 2.0f));
  }
};

typedef Vec3<float> Vec3f;
typedef Vec3<double> Vec3d;
typedef Vec3<int> Vec3i;