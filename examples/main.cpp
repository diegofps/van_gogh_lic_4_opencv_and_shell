
#include "vglic.hpp"

void
read_image_as_64FC4(const char * filepath, cv::Mat & result)
{
    cv::Mat original = cv::imread(filepath);
    cv::Mat as_rgba;
    cv::cvtColor(original, as_rgba, cv::COLOR_BGR2BGRA);
    as_rgba.convertTo(result, CV_64FC4, 1.0/255.0);
}

int 
main(int argc, char * argv[])
{
    cv::Mat effect_image;
    cv::Mat input_image;
    cv::Mat output_image;

    // Load images. They must by in CV_64FC4

    read_image_as_64FC4("../images/effect_image.png", effect_image);
    read_image_as_64FC4("../images/input_image.png", input_image);
    
    // Apply VanGoghLIC

    VanGoghLIC lic;
    
    lic.filter_length     = 6.0;
    lic.noise_magnitude   = 4.0;
    lic.integration_steps = 4.0;
    lic.minimum_value     = -25.0;
    lic.maximum_value     = +25.0;
    lic.effect_channel    = BRIGHTNESS;
    lic.effect_operator   = GRADIENT;
    lic.convolve_with     = SOURCE_IMAGE;

    lic.compute(input_image, effect_image, output_image);

    // Display image

    cv::namedWindow("LIC");
    cv::imshow("LIC", output_image);
    cv::waitKey();

    // Close windows

    cv::destroyAllWindows();

    return 0;
}
