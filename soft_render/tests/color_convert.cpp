

#include <iostream>

#include "lightsky/math/half.h"
#include "soft_render/SR_Color.hpp"



template <typename float_type>
void run_tests() noexcept
{
    SR_ColorRGBType<uint8_t> c1 = {10, 93, 173};
    std::cout << (unsigned)c1[0]  << ", " << (unsigned)c1[1]  << ", " << (unsigned)c1[2] << std::endl;

    SR_ColorRGBType<uint16_t> c2 = color_cast<uint16_t, uint8_t>(c1);
    std::cout << "RGB8 to RBG16: " << c2[0]  << ", " << c2[1]  << ", " << c2[2] << std::endl;

    SR_ColorTypeHSV<float_type> c3 = hsv_cast<float_type>(c2);
    std::cout << "RBG16 to HSVf: "  << c3.h  << ", " << c3.s  << ", " << c3.v << std::endl;

    SR_ColorTypeHSL<float_type> c4 = hsl_cast<float_type>(c1);
    std::cout << "RGB8 to HSL: "  << c4.h  << ", " << c4.s  << ", " << c4.l << std::endl;

    SR_ColorRGBType<float_type> c5 = rgb_cast<float_type>(c4);
    std::cout << "HSLf to RBGf: "  << c5[0]  << ", " << c5[1]  << ", " << c5[2] << std::endl;

    c1 = color_cast<uint8_t, float_type>(c5);
    std::cout << "RBG16 to RGB8: "  << (unsigned)c1[0]  << ", " << (unsigned)c1[1]  << ", " << (unsigned)c1[2] << std::endl;

    std::cout << std::endl;
}



/*-----------------------------------------------------------------------------
 * Main (for testing only)
 *
 * g++ -std=c++11 -Wall -Werror -Wextra -pedantic -pedantic-errors SR_ColorType.cpp -o color_cvt
-----------------------------------------------------------------------------*/
int main()
{
    //run_tests<ls::math::Half>();
    run_tests<float>();
    run_tests<double>();

    return 0;
}

