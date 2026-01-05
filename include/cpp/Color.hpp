#pragma once

#include "Vector.hpp"

#include <unordered_map>
#include <algorithm>
#include <vector>
#include <cmath>

struct RGBColor;
struct HSVColor;
struct LabColor;

#pragma pack(push, 1)

struct HSVColor
{
  float H = 0.0f;
  float S = 0.0f;
  float V = 0.0f;

  RGBColor toRGB() const;
};

struct YUVColor
{
  uint8_t Y = 0;
  uint8_t U = 0;
  uint8_t V = 0;

  RGBColor toRGB() const;
};

struct RGBColor
{
  uint8_t R = 0;
  uint8_t G = 0;
  uint8_t B = 0;

  RGBColor operator* (float c) const
  {
    return 
    {
      (uint8_t)std::clamp(c*R, 0.0f, 255.0f),
      (uint8_t)std::clamp(c*G, 0.0f, 255.0f),
      (uint8_t)std::clamp(c*B, 0.0f, 255.0f)
    };
  }

  bool operator== (const RGBColor& c) const
  {
    return R == c.R && G == c.G && B == c.B;
  }

  bool operator!= (const RGBColor& c) const
  {
    return R != c.R || G != c.G || B != c.B;
  }

  RGBColor operator* (const Vec3f& c) const
  {
    return
    {
      (uint8_t)std::clamp(c.X*R, 0.0f, 255.0f),
      (uint8_t)std::clamp(c.Y*G, 0.0f, 255.0f),
      (uint8_t)std::clamp(c.Z*B, 0.0f, 255.0f)
    };
  }

  void applyGamma(float gamma)
  {
    R = (uint8_t)std::clamp(powf((float)R / 255.0f, gamma) * 255.0f, 0.0f, 255.0f);
    G = (uint8_t)std::clamp(powf((float)G / 255.0f, gamma) * 255.0f, 0.0f, 255.0f);
    B = (uint8_t)std::clamp(powf((float)B / 255.0f, gamma) * 255.0f, 0.0f, 255.0f);
  }

  static RGBColor fromRGB565(uint16_t rgb565)
  {
    return
    {
      (uint8_t)  (rgb565 & (uint16_t)0b0000000011111000),
      (uint8_t)(((rgb565 & (uint16_t)0b0000000000000111) << 5) | 
                ((rgb565 & (uint16_t)0b1110000000000000) << 11)),
      (uint8_t) ((rgb565 & (uint16_t)0b0001111100000000) >> 5),
    };
  }

  static RGBColor blend(const RGBColor& a, const RGBColor& b, float t = 0.5f)
  {
    float invT = (1.0f - t);
    float R = (float)a.R * invT + (float) b.R * t;
    float G = (float)a.G * invT + (float) b.G * t;
    float B = (float)a.B * invT + (float) b.B * t;
    return { (uint8_t)R, (uint8_t)G, (uint8_t)B };
  }

  Vec3f toVec3f() const
  {
    return {(float)R / 255.0f, (float)G / 255.0f, (float)B / 255.0f};
  }

  HSVColor toHSV() const;
  LabColor toLab() const;
  uint8_t getBrightestChannel() const;
  uint8_t getDarkestChannel() const;
  uint8_t getGrayValue() const;
};

struct XYZColor
{
  float X = 0;
  float Y = 0;
  float Z = 0;
};

struct LabColor
{
  float L = 0;
  float a = 0;
  float b = 0;

  RGBColor toRGB() const;

  LabColor operator+ (const LabColor& c)  const
  {
    return {L + c.L, a + c.a, b + c.b};
  }

  LabColor operator* (const LabColor& c)  const
  {
    return {L * c.L, a * c.a, b * c.b};
  }

  void operator+= (const LabColor& c)
  {
    L += c.L;
    a += c.a;
    b += c.b;
  }

  LabColor operator- (const LabColor& c) const
  {
    return {L - c.L, a - c.a, b - c.b};
  }

  LabColor operator* (float c) const
  {
    return {c*L, c*a, c*b};
  }

  friend LabColor operator*(float c, const LabColor& rhs)
  {
      return {c*rhs.L, c*rhs.a, c*rhs.b};
  }

  float deltaE(const LabColor& other) const;
};
#pragma pack(pop)

static_assert(std::is_standard_layout<Vec3f>::value, "Vec3f must have standard layout.");
static_assert(std::is_trivially_copyable<Vec3f>::value, "Vec3f must be trivially copyable.");

static_assert(std::is_standard_layout<HSVColor>::value, "HSVColor must have standard layout.");
static_assert(std::is_trivially_copyable<HSVColor>::value, "HSVColor must be trivially copyable.");

static_assert(std::is_standard_layout<RGBColor>::value, "RGBColor must have standard layout.");
static_assert(std::is_trivially_copyable<RGBColor>::value, "RGBColor must be trivially copyable.");

static_assert(std::is_standard_layout<XYZColor>::value, "XYZColor must have standard layout.");
static_assert(std::is_trivially_copyable<XYZColor>::value, "XYZColor must be trivially copyable.");

static_assert(std::is_standard_layout<LabColor>::value, "LabColor must have standard layout.");
static_assert(std::is_trivially_copyable<LabColor>::value, "LabColor must be trivially copyable.");

// Get an RGBColor corresponding to a color teperature in kelvin.
// Works for all float values but returns colors are clamped 
// between 1000k and 12000k
RGBColor GetColorFromTemperature(float tempK);

RGBColor HSVColor::toRGB() const
{
  float r, g, b;
  int range = (int)std::floor(H / 60.0f);
  float c = V * S;
  float x = c * (1 - std::abs(fmod(H / 60.0f, 2.0f) - 1));
  float m = V - c;

  switch (range)
  {
  case 0:
    r = (c + m);
    g = (x + m);
    b = m;
    break;
  case 1:
    r = (x + m);
    g = (c + m);
    b = m;
    break;
  case 2:
    r = m;
    g = (c + m);
    b = (x + m);
    break;
  case 3:
    r = m;
    g = (x + m);
    b = (c + m);
    break;
  case 4:
    r = (x + m);
    g = m;
    b = (c + m);
    break;
  default: // case 5:
    r = (c + m);
    g = m;
    b = (x + m);
    break;
  }

  return {
      (uint8_t)std::clamp((r * 255.0f), 0.0f, 255.0f),
      (uint8_t)std::clamp((g * 255.0f), 0.0f, 255.0f),
      (uint8_t)std::clamp((b * 255.0f), 0.0f, 255.0f)};
}

HSVColor RGBColor::toHSV() const
{
  HSVColor hsv;

  float r = std::clamp(R / 255.0f, 0.0f, 1.0f);
  float g = std::clamp(G / 255.0f, 0.0f, 1.0f);
  float b = std::clamp(B / 255.0f, 0.0f, 1.0f);

  float min = std::min(r, std::min(g, b));
  float max = std::max(r, std::max(g, b));
  float delta = max - min;

  hsv.V = max;
  hsv.S = (max > 1e-3) ? (delta / max) : 0;

  if (delta == 0)
  {
    hsv.H = 0;
  }
  else
  {
    if (r == max)
      hsv.H = (g - b) / delta;
    else if (g == max)
      hsv.H = 2 + (b - r) / delta;
    else if (b == max)
      hsv.H = 4 + (r - g) / delta;

    hsv.H *= 60;
    hsv.H = fmod(hsv.H + 360, 360);
  }
  return hsv;
}

uint8_t RGBColor::getDarkestChannel() const
{
  return std::min({R, G, B});
}

uint8_t RGBColor::getBrightestChannel() const
{
  return std::max({R, G, B});
}


uint8_t RGBColor::getGrayValue() const
{
  return (uint8_t)(0.299f * (float)R + 0.587f * (float)G + 0.114f * (float)B);
}

void rgbToXyz(const RGBColor &rgb, XYZColor &xyz)
{
  float r = (float)rgb.R / 255.0f;
  float g = (float)rgb.G / 255.0f;
  float b = (float)rgb.B / 255.0f;

  r = ((r > 0.04045f) ? powf((r + 0.055f) / 1.055f, 2.4f) : (r / 12.92f)) * 100.0f;
  g = ((g > 0.04045f) ? powf((g + 0.055f) / 1.055f, 2.4f) : (g / 12.92f)) * 100.0f;
  b = ((b > 0.04045f) ? powf((b + 0.055f) / 1.055f, 2.4f) : (b / 12.92f)) * 100.0f;

  xyz.X = r * 0.4124564f + g * 0.3575761f + b * 0.1804375f;
  xyz.Y = r * 0.2126729f + g * 0.7151522f + b * 0.0721750f;
  xyz.Z = r * 0.0193339f + g * 0.1191920f + b * 0.9503041f;
}

void xyzToRgb(const XYZColor &xyz, RGBColor &rgb)
{
  float x = xyz.X / 100.0f;
  float y = xyz.Y / 100.0f;
  float z = xyz.Z / 100.0f;

  float r = x * 3.2404542f + y * -1.5371385f + z * -0.4985314f;
  float g = x * -0.9692660f + y * 1.8760108f + z * 0.0415560f;
  float b = x * 0.0556434f + y * -0.2040259f + z * 1.0572252f;

  r = ((r > 0.0031308f) ? (1.055f * powf(r, 1.0f / 2.4f) - 0.055f) : (12.92f * r)) * 255.0f;
  g = ((g > 0.0031308f) ? (1.055f * powf(g, 1.0f / 2.4f) - 0.055f) : (12.92f * g)) * 255.0f;
  b = ((b > 0.0031308f) ? (1.055f * powf(b, 1.0f / 2.4f) - 0.055f) : (12.92f * b)) * 255.0f;

  rgb.R = r;
  rgb.G = g;
  rgb.B = b;
}

void rgbToLab(const RGBColor &rgb, LabColor &lab)
{
  XYZColor xyz;
  rgbToXyz(rgb, xyz);

  float x = xyz.X / 95.047f;
  float y = xyz.Y / 100.00f;
  float z = xyz.Z / 108.883f;

  x = (x > 0.008856f) ? cbrtf(x) : (7.787f * x + 16.0f / 116.0f);
  y = (y > 0.008856f) ? cbrtf(y) : (7.787f * y + 16.0f / 116.0f);
  z = (z > 0.008856f) ? cbrtf(z) : (7.787f * z + 16.0f / 116.0f);

  lab.L = (116.0f * y) - 16.0f;
  lab.a = 500.0f * (x - y);
  lab.b = 200.0f * (y - z);
}

void labToRgb(const LabColor &lab, RGBColor &rgb)
{
  float y = (lab.L + 16.0) / 116.0f;
  float x = lab.a / 500.0f + y;
  float z = y - lab.b / 200.0f;

  float x3 = powf(x, 3.0f);
  float y3 = powf(y, 3.0f);
  float z3 = powf(z, 3.0f);

  x = ((x3 > 0.008856f) ? x3 : ((x - 16.0f / 116.0f) / 7.787f)) * 95.047f;
  y = ((y3 > 0.008856f) ? y3 : ((y - 16.0f / 116.0f) / 7.787f)) * 100.0f;
  z = ((z3 > 0.008856f) ? z3 : ((z - 16.0f / 116.0f) / 7.787f)) * 108.883f;

  xyzToRgb({x, y, z}, rgb);
}

LabColor RGBColor::toLab() const
{
  LabColor lab;
  rgbToLab(*this, lab);
  return lab;
}

RGBColor LabColor::toRGB() const
{
  RGBColor rgb;
  labToRgb(*this, rgb);
  return rgb;
}

RGBColor YUVColor::toRGB() const
{
  return 
  {
    (uint8_t)std::clamp((float)Y + 1.4075f * ((float)V - 128.0f), 0.0f, 255.0f),
    (uint8_t)std::clamp((float)Y - 0.3455f * ((float)U - 128.0f) - (0.7169f * ((float)V - 128.0f)), 0.0f, 255.0f),
    (uint8_t)std::clamp((float)Y + 1.7790f * ((float)U - 128.0f), 0.0f, 255.0f)
  };
}

float LabColor::deltaE(const LabColor& other) const
{
  return sqrtf(powf(L-other.L, 2) + powf(a-other.a, 2) + powf(b-other.b, 2));
}

static const std::vector<RGBColor> kelvinTable = 
{
  /* 1000:  */ {255, 56, 0},
  /* 1100:  */ {255, 71, 0},
  /* 1200:  */ {255, 83, 0},
  /* 1300:  */ {255, 93, 0},
  /* 1400:  */ {255, 101, 0},
  /* 1500:  */ {255, 109, 0},
  /* 1600:  */ {255, 115, 0},
  /* 1700:  */ {255, 121, 0},
  /* 1800:  */ {255, 126, 0},
  /* 1900:  */ {255, 131, 0},
  /* 2000:  */ {255, 138, 18},
  /* 2100:  */ {255, 142, 33},
  /* 2200:  */ {255, 147, 44},
  /* 2300:  */ {255, 152, 54},
  /* 2400:  */ {255, 157, 63},
  /* 2500:  */ {255, 161, 72},
  /* 2600:  */ {255, 165, 79},
  /* 2700:  */ {255, 169, 87},
  /* 2800:  */ {255, 173, 94},
  /* 2900:  */ {255, 177, 101},
  /* 3000:  */ {255, 180, 107},
  /* 3100:  */ {255, 184, 114},
  /* 3200:  */ {255, 187, 120},
  /* 3300:  */ {255, 190, 126},
  /* 3400:  */ {255, 193, 132},
  /* 3500:  */ {255, 196, 137},
  /* 3600:  */ {255, 199, 143},
  /* 3700:  */ {255, 201, 148},
  /* 3800:  */ {255, 204, 153},
  /* 3900:  */ {255, 206, 159},
  /* 4000:  */ {255, 209, 163},
  /* 4100:  */ {255, 211, 168},
  /* 4200:  */ {255, 213, 173},
  /* 4300:  */ {255, 215, 177},
  /* 4400:  */ {255, 217, 182},
  /* 4500:  */ {255, 219, 186},
  /* 4600:  */ {255, 221, 190},
  /* 4700:  */ {255, 223, 194},
  /* 4800:  */ {255, 225, 198},
  /* 4900:  */ {255, 227, 202},
  /* 5000:  */ {255, 228, 206},
  /* 5100:  */ {255, 230, 210},
  /* 5200:  */ {255, 232, 213},
  /* 5300:  */ {255, 233, 217},
  /* 5400:  */ {255, 235, 220},
  /* 5500:  */ {255, 236, 224},
  /* 5600:  */ {255, 238, 227},
  /* 5700:  */ {255, 239, 230},
  /* 5800:  */ {255, 240, 233},
  /* 5900:  */ {255, 242, 236},
  /* 6000:  */ {255, 243, 239},
  /* 6100:  */ {255, 244, 242},
  /* 6200:  */ {255, 245, 245},
  /* 6300:  */ {255, 246, 247},
  /* 6400:  */ {255, 248, 251},
  /* 6500:  */ {255, 249, 253},
  /* 6600:  */ {254, 249, 255},
  /* 6700:  */ {252, 247, 255},
  /* 6800:  */ {249, 246, 255},
  /* 6900:  */ {247, 245, 255},
  /* 7000:  */ {245, 243, 255},
  /* 7100:  */ {243, 242, 255},
  /* 7200:  */ {240, 241, 255},
  /* 7300:  */ {239, 240, 255},
  /* 7400:  */ {237, 239, 255},
  /* 7500:  */ {235, 238, 255},
  /* 7600:  */ {233, 237, 255},
  /* 7700:  */ {231, 236, 255},
  /* 7800:  */ {230, 235, 255},
  /* 7900:  */ {228, 234, 255},
  /* 8000:  */ {227, 233, 255},
  /* 8100:  */ {225, 232, 255},
  /* 8200:  */ {224, 231, 255},
  /* 8300:  */ {222, 230, 255},
  /* 8400:  */ {221, 230, 255},
  /* 8500:  */ {220, 229, 255},
  /* 8600:  */ {218, 229, 255},
  /* 8700:  */ {217, 227, 255},
  /* 8800:  */ {216, 227, 255},
  /* 8900:  */ {215, 226, 255},
  /* 9000:  */ {214, 225, 255},
  /* 9100:  */ {212, 225, 255},
  /* 9200:  */ {211, 224, 255},
  /* 9300:  */ {210, 223, 255},
  /* 9400:  */ {209, 223, 255},
  /* 9500:  */ {208, 222, 255},
  /* 9600:  */ {207, 221, 255},
  /* 9700:  */ {207, 221, 255},
  /* 9800:  */ {206, 220, 255},
  /* 9900:  */ {205, 220, 255},
  /* 10000: */ {207, 218, 255},
  /* 10100: */ {207, 218, 255},
  /* 10200: */ {206, 217, 255},
  /* 10300: */ {205, 217, 255},
  /* 10400: */ {204, 216, 255},
  /* 10500: */ {204, 216, 255},
  /* 10600: */ {203, 215, 255},
  /* 10700: */ {202, 215, 255},
  /* 10800: */ {202, 214, 255},
  /* 10900: */ {201, 214, 255},
  /* 11000: */ {200, 213, 255},
  /* 11100: */ {200, 213, 255},
  /* 11200: */ {199, 212, 255},
  /* 11300: */ {198, 212, 255},
  /* 11400: */ {198, 212, 255},
  /* 11500: */ {197, 211, 255},
  /* 11600: */ {197, 211, 255},
  /* 11700: */ {197, 210, 255},
  /* 11800: */ {196, 210, 255},
  /* 11900: */ {195, 210, 255},
  /* 12000: */ {195, 209, 255}
};

RGBColor GetColorFromTemperature(float tempK)
{
  float indexf = (tempK / 100.0f) - 10.0f;
  int indexLo = std::clamp((int) indexf, 0, (int)(kelvinTable.size() - 1));
  int indexHi = std::clamp((int) indexf + 1, 0, (int)(kelvinTable.size() - 1));
  float t = std::clamp(indexf - (float)indexLo, 0.0f, 1.0f);
  return RGBColor::blend(kelvinTable[indexLo], kelvinTable[indexHi], t);
}