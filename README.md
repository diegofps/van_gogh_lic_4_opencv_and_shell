# van_gogh_lic_4_opencv_and_shell

A port of Gimp's van-gogh-LIC plugin to execute from shell scripts or cpp code

# Building it

```shell
# Install dependencies
sudo apt update
sudo apt install git build-essential libopencv-dev -y

# clone the repository
git clone git@github.com:diegofps/van_gogh_lic_4_opencv_and_shell.git
```

# Calling it from c++ and opencv

```cpp
#include "vglic.hpp"

...

cv::Mat effect_image;
cv::Mat input_image;
cv::Mat output_image;

...

VanGoghLIC lic;

lic.filter_length     = 6.0;
lic.noise_magnitude   = 4.0;
lic.integration_steps = 4.0;
lic.minimum_value     = -25.0;
lic.maximum_value     = +25.0;
lic.effect_channel    = 2;
lic.effect_operator   = 1;
lic.convolve          = 1;

lic.compute(input_image, effect_image, output_image);
```

Compiling example

```shell
g++ main.cpp -o main -O3 -Wall -std=c++11 \
    -I/path_to_cloned_repository/lib \
    `pkg-config opencv4 --libs` \
    `pkg-config --cflags opencv4`
```

# Calling it from shell

Access the shell folder and compile the executable `vglic`.

```shell
cd shell
make
```

Use it specifying the parameters you want. Only `--input` and `--effect` are required.

```shell
./vglic \
    --input ../images/input_image.png \
    --effect ../images/input_image.png \
    --output ./output_image.png \
    --effect-channel BRIGHTNESS \
    --effect-operator GRADIENT \
    --convolve-with WHITE_NOISE \
    --filter-length 10 \
    --noise-magnitude 20 \
    --integration-steps 10 \
    --minimum-value -50 \
    --maximum-value 50
```
