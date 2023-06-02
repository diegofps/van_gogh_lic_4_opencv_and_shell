
#include "vglic.hpp"

void
error(const char * msg)
{
    std::cout << msg << std::endl;
    exit(1);
}

void
error(char const * msg, char const * token)
{
    std::cout << msg << " - " << token << std::endl;
    exit(1);
}

class BasicArgumentParser
{
private:

    int const argc;
    char const * const * const argv;
    int pos;

public:

    BasicArgumentParser(int const argc, char const * const * const argv) :
        argc(argc),
        argv(argv),
        pos(0)
    { }

    bool hasNext() { 
        return pos+1 < argc; 
    }

    void
    next() {
        pos += 1;
        if (pos >= argc)
            error("Missing parameter");
    }

    bool has(const char * value) { 
        return strcmp(value, argv[pos]) == 0; 
    }

    char const * current() { 
        return argv[pos]; 
    }
    
    int nextInt() {
        next();
        return std::stoi(argv[pos]);
    }

    int nextDouble() {
        next();
        return std::stod(argv[pos]);
    }

    char const * nextCharPtr() {
        next();
        return argv[pos];
    }
    
    template <typename TYPE>
    TYPE nextChoice(std::map<std::string, TYPE> & choices) {
        next();

        char const * value = current();
        auto it = choices.find(value);

        if (it == choices.end())
            error("Invalid choice", value);
        
        return it->second;
    }
    
};

void
read_image_as_64FC4(const char * filepath, cv::Mat & result)
{
    cv::Mat original = cv::imread(filepath);

    if (original.empty())
        error("Failed to open image file", filepath);

    cv::Mat as_rgba;
    cv::cvtColor(original, as_rgba, cv::COLOR_BGR2BGRA);
    as_rgba.convertTo(result, CV_64FC4, 1.0/255.0);
}

int main(int argc, char * argv[])
{

    // Configuration parameters

    char const * input_filepath = nullptr;
    char const * effect_filepath = nullptr;
    char const * output_filepath = nullptr;

    VanGoghLIC lic;


    // Basic argument parser

    std::map<std::string, EffectChannel> effect_channel_choices;
    effect_channel_choices["HUE"] = HUE;
    effect_channel_choices["SATURATION"] = SATURATION;
    effect_channel_choices["BRIGHTNESS"] = BRIGHTNESS;

    std::map<std::string, EffectOperator> effect_operator_choices;
    effect_operator_choices["DERIVATIVE"] = DERIVATIVE;
    effect_operator_choices["GRADIENT"]   = GRADIENT;

    std::map<std::string, ConvolveWith> convolve_with_choices;
    convolve_with_choices["WHITE_NOISE"]  = WHITE_NOISE;
    convolve_with_choices["SOURCE_IMAGE"] = SOURCE_IMAGE;

    BasicArgumentParser parser(argc, argv);

    while (parser.hasNext())
    {
        parser.next();

        if (parser.has("--input"))
            input_filepath = parser.nextCharPtr();

        else if (parser.has("--effect"))
            effect_filepath = parser.nextCharPtr();
        
        else if (parser.has("--output"))
            output_filepath = parser.nextCharPtr();

        else if (parser.has("--filter-length"))
            lic.filter_length = parser.nextDouble();
        
        else if (parser.has("--noise-magnitude"))
            lic.noise_magnitude = parser.nextDouble();
            
        else if (parser.has("--integration-steps"))
            lic.integration_steps = parser.nextDouble();
            
        else if (parser.has("--minimum-value"))
            lic.minimum_value = parser.nextDouble();
            
        else if (parser.has("--maximum-value"))
            lic.maximum_value = parser.nextDouble();
            
        else if (parser.has("--effect-channel"))
            lic.effect_channel = parser.nextChoice(effect_channel_choices);
            
        else if (parser.has("--effect-operator"))
            lic.effect_operator = parser.nextChoice(effect_operator_choices);
            
        else if (parser.has("--convolve-with"))
            lic.convolve_with = parser.nextChoice(convolve_with_choices);
        
        else
            error("Unexpected parameter", parser.current());
    }

    if (input_filepath == nullptr)
        error("Missing parameter --input");
    
    if (effect_filepath == nullptr)
        error("Missing parameter --effect");
    

    // Load images. They must by in CV_64FC4

    cv::Mat effect_image;
    cv::Mat input_image;

    read_image_as_64FC4(effect_filepath, effect_image);
    read_image_as_64FC4(input_filepath, input_image);
    

    // Apply VanGoghLIC

    cv::Mat output_image;

    lic.compute(input_image, effect_image, output_image);


    // Display image if output filepath is not set

    if (output_filepath == nullptr)
    {
        cv::namedWindow("LIC");
        cv::imshow("LIC", output_image);
        cv::waitKey();
        cv::destroyAllWindows();
    }


    // Otherwise, export the image to output_filepath

    else
    {
        cv::Mat tmp;
        output_image.convertTo(tmp, CV_8UC4, 255.0);
        cv::imwrite(output_filepath, tmp);
    }

    return 0;
}
