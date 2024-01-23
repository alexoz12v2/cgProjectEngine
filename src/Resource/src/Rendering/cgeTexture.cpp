#include "Rendering/cgeTexture.h"

#include "RenderUtils/GLutils.h"
#include "glad/gl.h"

namespace cge
{
void Texture_s::useTextureUnit(U32_t unit) { GL_CHECK(glActiveTexture(unit)); }
Texture_s::Texture_s() { GL_CHECK(glGenTextures(1, &m_id)); }

Texture_s::~Texture_s() { GL_CHECK(glDeleteTextures(1, &m_id)); }

U32_t Texture_s::getId() const { return m_id; }

void Texture_s::bind(ETexture_t type) const
{
    glBindTexture(bindFromType(type), m_id);
}

EErr_t Texture_s::allocate(TextureSpec_t const &spec)
{
    U32_t levels = 1;
    switch (spec.type)
    {
        using enum cge::ETexture_t;
    case e1D:
        m_ndims = 1;
        break;
    case e2D:
        levels = spec.genMips ? numLevels(spec.width, spec.height) : 1u;
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_id));
        GL_CHECK(glTexStorage2D(
          GL_TEXTURE_2D, levels, spec.internalFormat, spec.width, spec.height));
        m_ndims = 2;
        break;
    case e3D:
        m_ndims = 3;
        break;
    }

    m_mips    = (U8_t)levels;
    m_width   = spec.width;
    m_height  = spec.height;
    m_depth   = spec.depth;
    m_nlayers = 1; // TODO
    m_genMips = spec.genMips;
    m_type    = spec.type;
    return EErr_t::eSuccess;
}

EErr_t Texture_s::allocPixelUnpack(U32_t size, void const *data)
{
    // is immutable the right choice for a host visible buffer?
    m_pixelUnpack.allocateImmutable(
      GL_PIXEL_UNPACK_BUFFER, size, EAccess::eReadWrite);
    m_bPixelUnpack = true;

    m_pixelUnpack.transferDataImm(GL_PIXEL_UNPACK_BUFFER, 0, size, data);

    return EErr_t::eSuccess;
}

/// @warning No input validation
EErr_t Texture_s::transferData(TexTransferSpec_t const &spec)
{
    if (spec.usePixelUnpack) { m_pixelUnpack.bind(GL_PIXEL_UNPACK_BUFFER); }
    else { m_pixelUnpack.unbind(GL_PIXEL_UNPACK_BUFFER); }

    switch (m_type)
    {
        using enum cge::ETexture_t;
    case e1D:
        break;
    case e2D:
        GL_CHECK(glTexSubImage2D(
          GL_TEXTURE_2D,
          spec.level,
          spec.xoff,
          spec.yoff,
          spec.width,
          spec.height,
          spec.format,
          spec.type,
          spec.data));
        if (m_genMips) { GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D)); }
        break;
    case e3D:
        GL_CHECK(glTexSubImage3D(
          GL_TEXTURE_3D,
          spec.level,
          spec.xoff,
          spec.yoff,
          spec.zoff,
          spec.width,
          spec.height,
          spec.depth,
          spec.format,
          spec.type,
          spec.data));
        break;
    case e2DArray:
        GL_CHECK(glTexSubImage3D(
          GL_TEXTURE_2D_ARRAY,
          spec.level,
          spec.xoff,
          spec.yoff,
          spec.zoff,
          spec.width,
          spec.height,
          spec.depth,
          spec.format,
          spec.type,
          spec.data));
        break;
    case eCube:
        for (int face = 0; face < 6; face++)
        {
            GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
            GL_CHECK(glTexSubImage3D(
              target,
              spec.level,
              spec.xoff,
              spec.yoff,
              spec.zoff,
              spec.width,
              spec.height,
              1,
              spec.format,
              spec.type,
              spec.data));
        }
        break;
    case eCount:
        [[fallthrough]];
    case eInvalid:
        [[fallthrough]];
    default:
        break;
    }

    return EErr_t::eSuccess;
}

EErr_t Texture_s::defaultSamplerParams(SamplerSpec_t const &spec)
{
    U32_t texBind = bindFromType(m_type);
    glTexParameteri(texBind, GL_TEXTURE_MIN_FILTER, spec.minFilter);
    glTexParameteri(texBind, GL_TEXTURE_MAG_FILTER, spec.magFilter);
    glTexParameterf(texBind, GL_TEXTURE_MIN_LOD, spec.minLod);
    glTexParameterf(texBind, GL_TEXTURE_MAX_LOD, spec.maxLod);
    glTexParameteri(texBind, GL_TEXTURE_WRAP_S, spec.wrap);
    if (m_ndims >= 2)
    {
        glTexParameteri(texBind, GL_TEXTURE_WRAP_T, spec.wrap);
        if (m_ndims == 3)
        {
            glTexParameteri(texBind, GL_TEXTURE_WRAP_R, spec.wrap);
        }
    }
    glTexParameterfv(texBind, GL_TEXTURE_BORDER_COLOR, spec.borderColor);
    return EErr_t::eSuccess;
}

U32_t Texture_s::bindFromType(ETexture_t type)
{
    switch (type)
    {
        using enum cge::ETexture_t;
    case e1D:
        return GL_TEXTURE_1D;
    case e2D:
        return GL_TEXTURE_2D;
    case e3D:
        return GL_TEXTURE_3D;
    case e2DArray:
        return GL_TEXTURE_2D_ARRAY;
    case eCube:
        return GL_TEXTURE_CUBE_MAP;
    case eCount:
        [[fallthrough]];
    case eInvalid:
        return 0;
    }
}

// the following formula \floor(\log2{width, height}) + 1 = numLevels
U32_t Texture_s::numLevels(U32_t width, U32_t height)
{
#define max(a, b) ((a) > (b) ? (a) : (b))
    U32_t biggest = max(width, height);
#undef max

    // log2
<<<<<<< Updated upstream
=======
#if defined(_MSC_VER)
>>>>>>> Stashed changes
    biggest = 31 - __lzcnt(biggest);

    ++biggest;

    return biggest;
}

Sampler_s::Sampler_s() { GL_CHECK(glGenSamplers(1, &m_id)); }

Sampler_s::~Sampler_s() { GL_CHECK(glDeleteSamplers(1, &m_id)); }

EErr_t Sampler_s::defineParams(SamplerSpec_t const &spec) const
{
    glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, spec.minFilter);
    glSamplerParameteri(m_id, GL_TEXTURE_MAG_FILTER, spec.magFilter);
    glSamplerParameterf(m_id, GL_TEXTURE_MIN_LOD, spec.minLod);
    glSamplerParameterf(m_id, GL_TEXTURE_MAX_LOD, spec.maxLod);

    // Set wrap mode for each coordinate
    glSamplerParameteri(m_id, GL_TEXTURE_WRAP_S, spec.wrap);
    glSamplerParameteri(m_id, GL_TEXTURE_WRAP_T, spec.wrap);
    glSamplerParameteri(m_id, GL_TEXTURE_WRAP_R, spec.wrap);

    // Set border color
    glSamplerParameterfv(m_id, GL_TEXTURE_BORDER_COLOR, spec.borderColor);

    // glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_MODE, spec.compareMode);
    // glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_FUNC, spec.compareFunc);
    return EErr_t::eSuccess;
}

} // namespace cge
