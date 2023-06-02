/* LIBGIMP - The GIMP Library
 *
 * This is a derived work of the gimp's libgimpcolor library. It 
 * is adapted to work with opencv and has no dependency on the 
 * original gimp's libraries. Adaptations made in 2023, by Diego 
 * Souza.
 * 
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef LIB_GIMP_COLOR_HPP
#define LIB_GIMP_COLOR_HPP

#include <opencv2/opencv.hpp>
#include <cstdint>
#include <random>
#include <cmath>


/************/
/* Typedefs */
/************/

#define CLAMP(VALUE,MIN_VALUE,MAX_VALUE) (MAX((MIN_VALUE),MIN((MAX_VALUE),(VALUE))))
#define CLAMP0255(a)  CLAMP((a),0,255)
#define GIMP_HSL_UNDEFINED -1.0

#if defined (HAVE_RINT) && 0
/* note:  rint() depends on the current floating-point rounding mode.  when the
 * rounding mode is FE_TONEAREST, it, in parctice, breaks ties to even.  this
 * is different from 'floor (x + 0.5)', which breaks ties up.  in other words
 * 'rint (2.5) == 2.0', while 'floor (2.5 + 0.5) == 3.0'.  this is asking for
 * trouble, so let's just use the latter.
 */
#define RINT(x) rint(x)
#else
#define RINT(x) floor ((x) + 0.5)
#endif

#define gdouble  double
#define gint     int
#define gint32   int32_t
#define gboolean int
#define guchar   unsigned char
#define gfloat   float
#define glong    long
#define GimpRGBA cv::Vec4d
#define GimpHSL  cv::Vec4d


/****************/
/* libgimpcolor */
/****************/

inline void
gimp_rgba_multiply (GimpRGBA & rgb,
                    gdouble    factor)
{
  rgb[0] *= factor;
  rgb[1] *= factor;
  rgb[2] *= factor;
  rgb[3] *= factor;
}

inline void
gimp_rgba_add (GimpRGBA       & rgba1,
               const GimpRGBA & rgba2)
{
  rgba1[0] += rgba2[0];
  rgba1[1] += rgba2[1];
  rgba1[2] += rgba2[2];
  rgba1[3] += rgba2[3];
}

inline void
gimp_rgba_clamp (GimpRGBA & rgb)
{
  rgb[0] = CLAMP (rgb[0], 0.0, 1.0);
  rgb[1] = CLAMP (rgb[1], 0.0, 1.0);
  rgb[2] = CLAMP (rgb[2], 0.0, 1.0);
  rgb[3] = CLAMP (rgb[3], 0.0, 1.0);
}

inline GimpRGBA
gimp_bilinear_rgba (gdouble    x,
                    gdouble    y,
                    GimpRGBA * values)
{
  gdouble m0, m1;
  gdouble ix, iy;
  gdouble a0, a1, a2, a3, alpha;
  GimpRGBA v = { 0, };

  x = fmod (x, 1.0);
  y = fmod (y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  ix = 1.0 - x;
  iy = 1.0 - y;

  a0 = values[0][3];
  a1 = values[1][3];
  a2 = values[2][3];
  a3 = values[3][3];

  /* Alpha */

  m0 = ix * a0 + x * a1;
  m1 = ix * a2 + x * a3;

  alpha = v[3] = iy * m0 + y * m1;

  if (alpha > 0)
    {
      /* Red */

      m0 = ix * a0 * values[0][0] + x * a1 * values[1][0];
      m1 = ix * a2 * values[2][0] + x * a3 * values[3][0];

      v[0] = (iy * m0 + y * m1)/alpha;

      /* Green */

      m0 = ix * a0 * values[0][1] + x * a1 * values[1][1];
      m1 = ix * a2 * values[2][1] + x * a3 * values[3][1];

      v[1] = (iy * m0 + y * m1)/alpha;

      /* Blue */

      m0 = ix * a0 * values[0][2] + x * a1 * values[1][2];
      m1 = ix * a2 * values[2][2] + x * a3 * values[3][2];

      v[2] = (iy * m0 + y * m1)/alpha;
    }

  return v;
}

inline gdouble
gimp_rgba_max (const GimpRGBA & rgb)
{
  if (rgb[0] > rgb[1])
    return (rgb[0] > rgb[2]) ? rgb[0] : rgb[2];
  else
    return (rgb[1] > rgb[2]) ? rgb[1] : rgb[2];
}

inline gdouble
gimp_rgba_min (const GimpRGBA & rgb)
{
  if (rgb[0] < rgb[1])
    return (rgb[0] < rgb[2]) ? rgb[0] : rgb[2];
  else
    return (rgb[1] < rgb[2]) ? rgb[1] : rgb[2];
}

inline void
gimp_rgba_to_hsl (const GimpRGBA & rgb,
                  GimpHSL & hsl)
{
  gdouble max, min, delta;

  max = gimp_rgba_max (rgb);
  min = gimp_rgba_min (rgb);

  hsl[2] = (max + min) / 2.0;

  if (max == min)
  {
    hsl[1] = 0.0;
    hsl[0] = GIMP_HSL_UNDEFINED;
  }
  else
  {
    if (hsl[2] <= 0.5)
      hsl[1] = (max - min) / (max + min);
      
    else
      hsl[1] = (max - min) / (2.0 - max - min);

    delta = max - min;

    if (delta == 0.0)
      delta = 1.0;

    if (rgb[0] == max)
        hsl[0] = (rgb[1] - rgb[2]) / delta;

    else if (rgb[1] == max)
        hsl[0] = 2.0 + (rgb[2] - rgb[0]) / delta;

    else
        hsl[0] = 4.0 + (rgb[0] - rgb[1]) / delta;

    hsl[0] /= 6.0;

    if (hsl[0] < 0.0)
      hsl[0] += 1.0;
  }

  hsl[3] = rgb[3];
}

#endif /* LIB_GIMP_COLOR_HPP */
