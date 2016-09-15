#include "framebuffer.h"

#include "gl/texture.h"
#include "gl/renderState.h"
#include "gl/error.h"

namespace Tangram {

FrameBuffer::FrameBuffer(bool _colorRenderBuffer) :
    m_glFrameBufferHandle(0),
    m_generation(-1),
    m_valid(false),
    m_colorRenderBuffer(_colorRenderBuffer) {

}

void FrameBuffer::applyAsRenderTarget(RenderState& _rs,
                                      unsigned int _rtWidth, unsigned int _rtHeight,
                                      unsigned int _vpWidth, unsigned int _vpHeight) {

    if (!m_glFrameBufferHandle) {
        init(_rs, _rtWidth, _rtHeight);
    }

    if (!m_valid) {
        return;
    }

    _rs.depthMask(GL_TRUE);

    // TOOD: get viewport size from render state, re-enable when unbinding
    GL::viewport(0, 0, _vpWidth, _vpHeight);
    GL::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO: use render state
    GL::bindFramebuffer(GL_FRAMEBUFFER, m_glFrameBufferHandle);
}

void FrameBuffer::init(RenderState& _rs, unsigned int _rtWidth, unsigned int _rtHeight) {

    // get previous bound fbo

    GL::genFramebuffers(1, &m_glFrameBufferHandle);

    // TOOD: use render state for binding
    GL::bindFramebuffer(GL_FRAMEBUFFER, m_glFrameBufferHandle);

    // Setup color render target
    if (m_colorRenderBuffer) {
        GL::genRenderbuffers(1, &m_glColorRenderBufferHandle);
        GL::bindRenderbuffer(GL_RENDERBUFFER, m_glColorRenderBufferHandle);
        GL::renderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _rtWidth, _rtHeight);

        GL::framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_RENDERBUFFER, m_glColorRenderBufferHandle);
    } else {
        TextureOptions options =
            {GL_RGBA, GL_RGBA,
            {GL_NEAREST, GL_NEAREST},
            {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}
        };

        m_texture = std::make_unique<Texture>(_rtWidth, _rtHeight, options);
        m_texture->update(_rs, 0);

        GL::framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, m_texture->getGlHandle(), 0);
    }

    {
        // Create depth render buffer
        GL::genRenderbuffers(1, &m_glDepthRenderBufferHandle);
        GL::bindRenderbuffer(GL_RENDERBUFFER, m_glDepthRenderBufferHandle);
        GL::renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _rtWidth, _rtHeight);

        GL::framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                    m_glDepthRenderBufferHandle);
    }

    GLenum status = GL::checkFramebufferStatus(GL_FRAMEBUFFER);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer status is incomplete");
    } else {
        m_valid = true;
    }

    m_generation = _rs.generation();

    m_disposer = Disposer(_rs);
}

FrameBuffer::~FrameBuffer() {

    int generation = m_generation;
    GLuint glHandle = m_glFrameBufferHandle;

    m_disposer([=](RenderState& rs) {
        if (rs.isValidGeneration(generation)) {
            // TODO: unset render state for framebuffer binding
            // rs.framebufferUnset(glHandle);

            GL::deleteFramebuffers(1, &glHandle);
        }
    });
}

}
