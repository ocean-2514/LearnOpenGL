#pragma once
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <glad/glad.h>
#include <iostream>

class Framebuffer {
public:
    unsigned int fbo = 0;
    unsigned int colorTexture = 0;
    unsigned int rbo = 0;

    unsigned int screenWidth = 800;
    unsigned int screenHeight = 600;

    Framebuffer(unsigned int width = 800, unsigned int height = 600) : screenWidth(width), screenHeight(height) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Create color texture
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        // Create depth buffer
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Framebuffer&& other) noexcept {
        fbo = other.fbo;
        colorTexture = other.colorTexture;
        rbo = other.rbo;
        screenWidth = other.screenWidth;
        screenHeight = other.screenHeight;

        other.fbo = 0;
        other.colorTexture = 0;
        other.rbo = 0;
    }
    Framebuffer& operator=(Framebuffer&& other) noexcept {
        if (this != &other) {
            glDeleteFramebuffers(1, &fbo);
            glDeleteTextures(1, &colorTexture);
            glDeleteRenderbuffers(1, &rbo);

            fbo = other.fbo;
            colorTexture = other.colorTexture;
            rbo = other.rbo;
            screenWidth = other.screenWidth;
            screenHeight = other.screenHeight;

            other.fbo = 0;
            other.colorTexture = 0;
            other.rbo = 0;
        }
        return *this;
    }

    ~Framebuffer() {
        std::cout << "Framebuffer destructor called" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &colorTexture);
        glDeleteRenderbuffers(1, &rbo);
    }

};

#endif // FRAMEBUFFER_H