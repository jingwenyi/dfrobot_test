/*
**
**author  jingwenyi create 2016.03.18
**
*/

/*
//这个在sdl 中已经声明
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long uintptr_t;
*/


struct RGBPixel
{
    RGBPixel()
    {
        m_r = m_g = m_b = 0;
    }

    RGBPixel(uint8_t r, uint8_t g, uint8_t b)
    {
        m_r = r;
        m_g = g;
        m_b = b;
    }

    uint8_t m_r;
	uint8_t m_g;
	uint8_t m_b;
};



struct Frame8
{
    Frame8()
    {
        m_pixels = (uint8_t *)NULL;
        m_width = m_height = 0;
    }

    Frame8(uint8_t *pixels, uint16_t width, uint16_t height)
    {
        m_pixels = pixels;
        m_width = width;
        m_height = height;
    }

    uint8_t *m_pixels;
    int16_t m_width;
    int16_t m_height;
};








