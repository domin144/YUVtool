#include <yuv/Picture_buffer.h>

#include <gtest/gtest.h>

using namespace YUV_tool;

const Pixel_format rgb_planar{
    get_expanded_pixel_format(RGB_color_space, {{8}, {8}, {8}})};
const Pixel_format yuv_planar{
    get_expanded_pixel_format(ITU601_YCbCr_color_space, {{8}, {8}, {8}})};

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

TEST(color_transform_test, test_1)
{
    const Precalculated_pixel_format yuv_planar_precalculated {
        *Precalculated_pixel_format::create(yuv_planar)};
    EXPECT_TRUE(yuv_planar_precalculated.is_expanded());

    {
        Picture_buffer p0{Vector<Unit::pixel>{1, 3}, yuv_planar};
        /* chroma equal to 0x80 means gray */
        p0.get_data()[0] = 0x80; /* Y */
        p0.get_data()[3] = 0x80; /* Cb */
        p0.get_data()[6] = 0x80; /* Cr */
        p0.get_data()[1] = 16; /* Y */
        p0.get_data()[4] = 0x80; /* Cb */
        p0.get_data()[7] = 0x80; /* Cr */
        p0.get_data()[2] = 235; /* Y */
        p0.get_data()[5] = 0x80; /* Cb */
        p0.get_data()[8] = 0x80; /* Cr */

        p0.convert_color_space(RGB_color_space);

        /* (128-16)/219*255 ~= 130,41 */
        EXPECT_EQ(p0.get_data()[0], 130);
        EXPECT_EQ(p0.get_data()[3], 130);
        EXPECT_EQ(p0.get_data()[6], 130);
        EXPECT_EQ(p0.get_data()[1], 0x00);
        EXPECT_EQ(p0.get_data()[4], 0x00);
        EXPECT_EQ(p0.get_data()[7], 0x00);
        EXPECT_EQ(p0.get_data()[2], 0xff);
        EXPECT_EQ(p0.get_data()[5], 0xff);
        EXPECT_EQ(p0.get_data()[8], 0xff);
    }
}

