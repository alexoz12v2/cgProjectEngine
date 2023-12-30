#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"
#include "Resource/Rendering/Buffer.h"

#include <cmath>
#include <memory>
#include <utility>

namespace cge
{

enum class ETexture_t : U8_t
{
    eInvalid = 0,
    e1D,
    e2D,
    e3D,
    e2DArray,
    eCube,
    eCount
};

struct TextureSpec_t
{
    ETexture_t type;
    U32_t      width;
    U32_t      height;
    U32_t      depth;
    U32_t      internalFormat;
    B8_t       genMips;
};

struct TextureData_s
{
    std::shared_ptr<Byte_t> data;

    U32_t width;
    U32_t height;
    U32_t depth; // if 1 then 2D

    // OpenGL related
    U32_t format;
    U32_t type;
};

struct TexTransferSpec_t
{
    void const* data;

    U32_t level;
    U32_t xoff;
    U32_t yoff;
    U32_t zoff;
    U32_t width;
    U32_t height;
    U32_t depth;
    U32_t layer;

    U32_t format;
    U32_t type;

    B8_t usePixelUnpack;
};

// TODO: presets functions
struct SamplerSpec_t
{
    U32_t minFilter;
    U32_t magFilter;
    F32_t minLod;
    F32_t maxLod;
    U32_t wrap; // s t and r
    F32_t borderColor[3];
    // U32_t compareMode; maybe later, these
    // U32_t compareFunc; are for shadow sampler
};

class Texture_s
{
  public:
    static void useTextureUnit(U32_t unit);
    Texture_s();
    ~Texture_s();

    U32_t getId() const;

    void bind(ETexture_t type) const;

    EErr_t allocate(TextureSpec_t const& spec);
    EErr_t allocPixelUnpack(U32_t size, void const* data);

    /// @warning if pixel unpack is to be used allocate it first
    EErr_t transferData(TexTransferSpec_t const& spec);

    EErr_t defaultSamplerParams(SamplerSpec_t const& spec);

    static U32_t bindFromType(ETexture_t type);

  private:
    Buffer_s m_pixelUnpack;
    U32_t    m_id = 0;

    U32_t m_width  = 0;
    U32_t m_height = 0;
    U32_t m_depth  = 0;

    U8_t       m_nlayers      = 0;
    U8_t       m_mips         = 0;
    U8_t       m_ndims        = 0;
    ETexture_t m_type         = ETexture_t::eInvalid;
    B8_t       m_genMips      = false;
    B8_t       m_bPixelUnpack = false;

    // TODO 3D
    U32_t static numLevels(U32_t width, U32_t height);
};

class Sampler_s
{
  public:
    Sampler_s();
    ~Sampler_s();
    EErr_t defineParams(SamplerSpec_t const& spec) const;

  private:
    U32_t m_id = 0;
};

} // namespace cge
