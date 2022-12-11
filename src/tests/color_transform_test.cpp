#include <yuv/Picture_buffer.h>

#include <gtest/gtest.h>

using namespace YUV_tool;

const Pixel_format rgb_planar{
    get_expanded_pixel_format(RGB_color_space, {{8}, {8}, {8}})};

TEST(color_transform_test, test_0)
{
    const Precalculated_pixel_format rgb_planar_precalculated {
        *Precalculated_pixel_format::create(rgb_planar)};
    EXPECT_TRUE(rgb_planar_precalculated.is_expanded());

    {
        Picture_buffer p0{Vector<Unit::pixel>{1, 1}, rgb_planar};
        p0.get_data()[0] = 0;
        p0.get_data()[1] = 0;
        p0.get_data()[2] = 0;

        p0.convert_color_space(ITU601_YCbCr_color_space);

        EXPECT_EQ(p0.get_data()[0], 16);
        EXPECT_EQ(p0.get_data()[1], 128);
        EXPECT_EQ(p0.get_data()[2], 128);
    }
}
