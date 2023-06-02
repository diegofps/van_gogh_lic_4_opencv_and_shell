/* Line Integral Convolution (LIC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This program implements the Line Integral Convolution (LIC) as described in
 * Cabral et al. "Imaging vector fields using line integral convolution" in the
 * Proceedings of ACM SIGGRAPH 93. Publ. by ACM, New York, NY, USA. p. 263-270.
 * (See http://www8.cs.umu.se/kurser/TDBD13/VT00/extra/p263-cabral.pdf)
 *
 * Some of the code is based on the gimp's plugin van-gogh-lic, developed by 
 * Tom Bech, which also used pieces from Steinar Haugen. This program is a port 
 * of their work to opencv.
 */

#ifndef VAN_GOGH_LIC_HPP
#define VAN_GOGH_LIC_HPP

#include "libgimpcolor.hpp"

/*****************************/
/* Global variables and such */
/*****************************/

#define numx 40              
#define numy 40


typedef enum
{
  HUE,
  SATURATION,
  BRIGHTNESS
} EffectChannel;

typedef enum
{
  DERIVATIVE,
  GRADIENT
} EffectOperator;

typedef enum
{
  WHITE_NOISE,
  SOURCE_IMAGE
} ConvolveWith;

static gdouble G[numx][numy][2];

class VanGoghLIC
{
public:

  gdouble        filter_length;
  gdouble        noise_magnitude;
  gdouble        integration_steps;
  gdouble        minimum_value;
  gdouble        maximum_value;
  EffectChannel  effect_channel;
  EffectOperator effect_operator;
  ConvolveWith   convolve_with;

public:

  VanGoghLIC() 
  {
    // public parameters
    
    filter_length     = 5;
    noise_magnitude   = 2;
    integration_steps = 25;
    minimum_value     = -25;
    maximum_value     = 25;
    effect_channel    = BRIGHTNESS;
    effect_operator   = GRADIENT;
    convolve_with     = SOURCE_IMAGE;

    // private parameters

    l      = 10.0;
    dx     =  2.0;
    dy     =  2.0;
    minv   = -2.5;
    maxv   =  2.5;
    isteps = 20.0;

  }

  void
  compute (cv::Mat & input_image, 
           cv::Mat & effect_image, 
           cv::Mat & output_image)
  {
    if (input_image.type() != CV_64FC4)
      throw std::invalid_argument("VanGoghLIC requires an input_image with type CV_64FC4");

    if (effect_image.type() != CV_64FC4)
      throw std::invalid_argument("VanGoghLIC requires an effect_image with type CV_64FC4");

    output_image = cv::Mat(input_image.rows, input_image.cols, CV_64FC4);

    if (convolve_with == WHITE_NOISE)
      generatevectors ();

    if (filter_length < 0.1)
      filter_length = 0.1;

    l      = filter_length;
    dx     = noise_magnitude;
    dy     = noise_magnitude;
    minv   = minimum_value / 10.0;
    maxv   = maximum_value / 10.0;
    isteps = integration_steps;

    effect_width  =  effect_image.cols;
    effect_height = effect_image.rows;

    if (effect_channel == HUE)
      rgb_to_hsl (effect_image, HUE, scalarfield);
    else if (effect_channel == SATURATION)
      rgb_to_hsl (effect_image, SATURATION, scalarfield);
    else if (effect_channel == BRIGHTNESS)
      rgb_to_hsl (effect_image, BRIGHTNESS, scalarfield);
    else
      throw std::invalid_argument("Invalid value for effect_channel");

    compute_lic (input_image, output_image, scalarfield, effect_operator);
  }

private:

  gdouble l;
  gdouble dx;
  gdouble dy;
  gdouble minv;
  gdouble maxv;
  gdouble isteps;

  gint effect_width;
  gint effect_height;
      
  std::vector<uchar> scalarfield;

private:

  /************************/
  /* Convenience routines */
  /************************/

  void
  peek (cv::Mat & buffer,
        gint x,
        gint y,
        GimpRGBA & color)
  {
    color = buffer.at<GimpRGBA>(y, x);
  }

  void
  poke (cv::Mat & buffer,
        gint x,
        gint y,
        GimpRGBA & color)
  {
    buffer.at<GimpRGBA>(y, x) = color;
  }

  gint
  peekmap (const std::vector<guchar> & scalarfield,
           gint x,
           gint y)
  {
    while (x < 0)
      x += effect_width;
    x %= effect_width;

    while (y < 0)
      y += effect_height;
    y %= effect_height;

    return (gint) scalarfield[x + effect_width * y];
  }

  /*************/
  /* Main part */
  /*************/

  /***************************************************/
  /* Compute the derivative in the x and y direction */
  /* We use these convolution kernels:               */
  /*     |1 0 -1|     |  1   2   1|                  */
  /* DX: |2 0 -2| DY: |  0   0   0|                  */
  /*     |1 0 -1|     | -1  -2  -1|                  */
  /* (It's a variation of the Sobel kernels, really)  */
  /***************************************************/

  gint
  gradx (const std::vector<guchar> & scalarfield,
         gint x,
         gint y)
  {
    gint val = 0;

    val = val + peekmap (scalarfield, x-1, y-1);
    val = val - peekmap (scalarfield, x+1, y-1);

    val = val + peekmap (scalarfield, x-1, y  ) * 2;
    val = val - peekmap (scalarfield, x+1, y  ) * 2;

    val = val + peekmap (scalarfield, x-1, y+1);
    val = val - peekmap (scalarfield, x+1, y+1);

    return val;
  }

  gint
  grady (const std::vector<guchar> & scalarfield,
         gint x,
         gint y)
  {
    gint val = 0;

    val = val + peekmap (scalarfield, x-1, y-1);
    val = val + peekmap (scalarfield, x,   y-1) * 2;
    val = val + peekmap (scalarfield, x+1, y-1);

    val = val - peekmap (scalarfield, x-1, y+1);
    val = val - peekmap (scalarfield, x,   y+1) * 2;
    val = val - peekmap (scalarfield, x+1, y+1);

    return val;
  }

  /************************************/
  /* A nice 2nd order cubic spline :) */
  /************************************/

  gdouble
  cubic (gdouble t)
  {
    gdouble at = fabs (t);

    return (at < 1.0) ? at * at * (2.0 * at - 3.0) + 1.0 : 0.0;
  }

  gdouble
  omega (gdouble u,
         gdouble v,
         gint i,
         gint j)
  {
    while (i < 0)
      i += numx;

    while (j < 0)
      j += numy;

    i %= numx;
    j %= numy;

    return cubic (u) * cubic (v) * (G[i][j][0]*u + G[i][j][1]*v);
  }

  /*************************************************************/
  /* The noise function (2D variant of Perlins noise function) */
  /*************************************************************/

  gdouble
  noise (gdouble x,
         gdouble y)
  {
    gint i, sti = (gint) floor (x / dx);
    gint j, stj = (gint) floor (y / dy);

    gdouble sum = 0.0;

    /* ========================= */
    /* Calculate the gdouble sum */
    /* ========================= */

    for (i = sti; i <= sti + 1; i++)
      for (j = stj; j <= stj + 1; j++)
      {
        sum += omega ((x - (gdouble) i * dx) / dx,
                      (y - (gdouble) j * dy) / dy,
                      i, j);
      }

    return sum;
  }

  /*************************************************/
  /* Generates pseudo-random vectors with length 1 */
  /*************************************************/

  void
  generatevectors (void)
  {
    gdouble alpha;
    gint i, j;

    std::mt19937 generator;
    std::uniform_real_distribution<double> uniform1;

    for (i = 0; i < numx; i++)
      for (j = 0; j < numy; j++)
      {
        alpha = uniform1(generator) * 2 * M_PI;
        G[i][j][0] = cos (alpha);
        G[i][j][1] = sin (alpha);
      }
  }

  /* ======================== */
  /* A simple triangle filter */
  /* ======================== */

  gdouble
  filter (gdouble u)
  {
    gdouble f = 1.0 - fabs (u) / l;
    return (f < 0.0) ? 0.0 : f;
  }

  /******************************************************/
  /* Compute the Line Integral Convolution (LIC) at x,y */
  /******************************************************/

  gdouble
  lic_noise (gint x,
             gint y,
             gdouble vx,
             gdouble vy)
  {
    gdouble i = 0.0;
    gdouble f1 = 0.0, f2 = 0.0;
    gdouble u, step = 2.0 * l / isteps;
    gdouble xx = (gdouble) x, yy = (gdouble) y;
    gdouble c, s;

    /* ================= */
    /* Get vector at x,y */
    /* ================= */

    c = vx;
    s = vy;

    /* ============================== */
    /* Calculate integral numerically */
    /* ============================== */

    f1 = filter (-l) * noise (xx + l * c , yy + l * s);

    for (u = -l + step; u <= l; u += step)
    {
      f2 = filter (u) * noise ( xx - u * c , yy - u * s);
      i += (f1 + f2) * 0.5 * step;
      f1 = f2;
    }

    i = (i - minv) / (maxv - minv);

    i = CLAMP (i, 0.0, 1.0);

    i = (i / 2.0) + 0.5;

    return i;
  }

  void
  getpixel (cv::Mat & buffer,
            GimpRGBA & p,
            gdouble u,
            gdouble v)
  {
    register gint x1, y1, x2, y2;
    static GimpRGBA pp[4];

    gint width  = buffer.cols;
    gint height = buffer.rows;

    x1 = (gint)u;
    y1 = (gint)v;

    if (x1 < 0)
      x1 = width - (-x1 % width);
    
    else
      x1 = x1 % width;

    if (y1 < 0)
      y1 = height - (-y1 % height);

    else
      y1 = y1 % height;

    x2 = (x1 + 1) % width;
    y2 = (y1 + 1) % height;

    peek (buffer, x1, y1, pp[0]);
    peek (buffer, x2, y1, pp[1]);
    peek (buffer, x1, y2, pp[2]);
    peek (buffer, x2, y2, pp[3]);

    p = gimp_bilinear_rgba (u, v, pp);
  }

  void
  lic_image (cv::Mat  & buffer,
             gint       x,
             gint       y,
             gdouble    vx,
             gdouble    vy,
             GimpRGBA & color)
  {
    GimpRGBA col1, col2, col3;
    GimpRGBA col = { 0, 0, 0, 0 };
    gdouble step = 2.0 * l / isteps;
    gdouble xx = (gdouble) x;
    gdouble yy = (gdouble) y;
    gdouble u;
    gdouble c;
    gdouble s;

    /* ================= */
    /* Get vector at x,y */
    /* ================= */

    c = vx;
    s = vy;

    /* ============================== */
    /* Calculate integral numerically */
    /* ============================== */

    getpixel (buffer, col1, xx + l * c, yy + l * s);
    gimp_rgba_multiply (col1, filter (-l));
    
    for (u = -l + step; u <= l; u += step)
    {
      getpixel (buffer, col2, xx - u * c, yy - u * s);
      gimp_rgba_multiply (col2, filter (u));

      col3 = col1;

      gimp_rgba_add (col3, col2);
      gimp_rgba_multiply (col3, 0.5 * step);
      gimp_rgba_add (col, col3);

      col1 = col2;
    }
  
    gimp_rgba_multiply (col, 1.0 / l);
    gimp_rgba_clamp (col);

    color = col;
  }

  void
  rgb_to_hsl (cv::Mat & effect_image,
              EffectChannel effect_channel,
              std::vector<guchar> & themap)
  {
    gint      x;
    gint      y;
    GimpRGBA  color;
    GimpHSL   color_hsl;
    gdouble   val = 0.0;
    glong     index = 0;

    std::mt19937 generator;
    std::uniform_real_distribution<double> uniform1;

    themap.resize(effect_image.cols * effect_image.rows);

    int channel_idx = effect_channel == HUE ? 0 :
                      effect_channel == SATURATION ? 1 :
                      effect_channel == BRIGHTNESS ? 2 : -1;
    
    assert(channel_idx != -1);

    for (y = 0; y < effect_image.rows; y++)
      for (x = 0; x < effect_image.cols; x++)
      {
        peek (effect_image, x, y, color);
        gimp_rgba_to_hsl (color, color_hsl);
        val = color_hsl[channel_idx] * 255;
        val += uniform1(generator) * 2.0 - 1.0;
        themap[index++] = (guchar) CLAMP0255 (RINT (val));
      }
  }

  void
  compute_lic (cv::Mat & input_image,
               cv::Mat & output_image,
               const std::vector<guchar> & scalarfield,
               EffectOperator effect_operator)
  {
    gint xcount;
    gint ycount;
    GimpRGBA color;
    gdouble vx;
    gdouble vy;
    gdouble tmp;

    for (ycount = 0; ycount < input_image.rows; ycount++)
      for (xcount = 0; xcount < input_image.cols; xcount++)
      {
        /* ======================================== */
        /* Get derivative at (x,y) and normalize it */
        /* ======================================== */

        vx = gradx (scalarfield, xcount, ycount);
        vy = grady (scalarfield, xcount, ycount);

        /* Rotate if needed */
        if (effect_operator == GRADIENT)
        {
          tmp = vy;
          vy = -vx;
          vx = tmp;
        }

        tmp = sqrt (vx * vx + vy * vy);

        if (tmp >= 0.000001)
        {
          tmp = 1.0 / tmp;
          vx *= tmp;
          vy *= tmp;
        }

        /* ============================== */
        /* Convolve with the LIC at (x,y) */
        /* ============================== */

        if (convolve_with == WHITE_NOISE)
        {
          peek (input_image, xcount, ycount, color);
          tmp = lic_noise (xcount, ycount, vx, vy);
          gimp_rgba_multiply (color, tmp);
        }
        else if (convolve_with == SOURCE_IMAGE)
        {
          lic_image (input_image, xcount, ycount, vx, vy, color);
        }

        poke (output_image, xcount, ycount, color);
      }
  }

};

#endif /* VAN_GOGH_LIC_HPP */