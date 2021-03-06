
#include "softlight/SL_Material.hpp"



/*-------------------------------------
 * Reset all material parameters
-------------------------------------*/
void sl_reset(SL_Material& m) noexcept
{
    for (unsigned i = SL_MaterialProperty::SL_MATERIAL_MAX_TEXTURES; i --> 0;)
    {
        m.pTextures[i] = nullptr;
    }

    m.ambient = SL_ColorRGBAf{0.f, 0.f, 0.f, 1.f};
    m.diffuse = SL_ColorRGBAf{0.f, 0.f, 0.f, 1.f};
    m.specular = SL_ColorRGBAf{0.f, 0.f, 0.f, 1.f};
    m.shininess = 0.f;
}



/*-------------------------------------
 * Material Validation
-------------------------------------*/
SL_MaterialStatus validate(const SL_Material& m) noexcept
{
    for (const SL_Texture* texA : m.pTextures)
    {
        if (!texA)
        {
            return SL_MATERIAL_STATUS_INVALID_TEXTURE;
        }

        for (const SL_Texture* texB : m.pTextures)
        {
            if (texA == texB)
            {
                return SL_MATERIAL_STATUS_DUPLICATE_TEXTURES;
            }
        }
    }

    if (m.ambient[0] < 0.f
    || m.ambient[1] < 0.f
    || m.ambient[2] < 0.f
    || m.ambient[3] < 0.f
    || m.diffuse[0] < 0.f
    || m.diffuse[1] < 0.f
    || m.diffuse[2] < 0.f
    || m.diffuse[3] < 0.f
    || m.specular[0] < 0.f
    || m.specular[1] < 0.f
    || m.specular[2] < 0.f
    || m.specular[3] < 0.f
    || m.shininess < 0.f)
    {
        return SL_MATERIAL_STATUS_VALUE_UNDERFLOW;
    }

    if (m.ambient[0] > 1.f
    || m.ambient[1] > 1.f
    || m.ambient[2] > 1.f
    || m.ambient[3] > 1.f
    || m.diffuse[0] > 1.f
    || m.diffuse[1] > 1.f
    || m.diffuse[2] > 1.f
    || m.diffuse[3] > 1.f
    || m.specular[0] > 1.f
    || m.specular[1] > 1.f
    || m.specular[2] > 1.f
    || m.specular[3] > 1.f
    || m.shininess > 1.f)
    {
        return SL_MATERIAL_STATUS_VALUE_OVERFLOW;
    }

    return SL_MATERIAL_STATUS_VALID;
}
