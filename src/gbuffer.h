#pragma once
#include <glad/glad.h>
#include <iostream>

class GBuffer {
public:
    unsigned int fbo = 0;
    unsigned int gPosition = 0;
    unsigned int gNormal = 0;
    unsigned int gAlbedoSpec = 0;
    unsigned int rboDepth = 0;

    GBuffer() = default;

    ~GBuffer() {
        CleanUp();
    }

    bool Init(unsigned int width, unsigned int height) {
        CleanUp();

        // 1. Create Framebuffer Object
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // 2. Position Color Buffer
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

        // 3. Normal Color Buffer
        glGenTextures(1, &gNormal);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

        // 4. Albedo + Specular Color Buffer
        glGenTextures(1, &gAlbedoSpec);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

        // 5. Tell OpenGL which color buffers to draw into
        const unsigned int attachments[3] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2
        };
        glDrawBuffers(3, attachments);

        // 6. Depth Renderbuffer
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

        // 7. Verify Framebuffer Completeness
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "GBuffer Framebuffer is not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    // Binds the G-Buffer FBO for the geometry rendering pass
    void BindForWriting() const {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    // Unbinds the G-Buffer back to the default system framebuffer
    static void Unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Binds output textures to specified texture units for the lighting pass
    void BindTexturesForReading(unsigned int startUnit = 0) const {
        glActiveTexture(GL_TEXTURE0 + startUnit);
        glBindTexture(GL_TEXTURE_2D, gPosition);

        glActiveTexture(GL_TEXTURE0 + startUnit + 1);
        glBindTexture(GL_TEXTURE_2D, gNormal);

        glActiveTexture(GL_TEXTURE0 + startUnit + 2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    }

    void CleanUp() {
        if (fbo)          { glDeleteFramebuffers(1, &fbo); fbo = 0; }
        if (gPosition)    { glDeleteTextures(1, &gPosition); gPosition = 0; }
        if (gNormal)      { glDeleteTextures(1, &gNormal); gNormal = 0; }
        if (gAlbedoSpec)  { glDeleteTextures(1, &gAlbedoSpec); gAlbedoSpec = 0; }
        if (rboDepth)     { glDeleteRenderbuffers(1, &rboDepth); rboDepth = 0; }
    }
};
