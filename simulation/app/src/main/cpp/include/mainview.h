//
// Created by martin on 08-05-2024.
//


#ifndef GL_SHADER_MANAGER_H
#define GL_SHADER_MANAGER_H

#include <string>
#include <chrono>
#include <GLES3/gl32.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/asset_manager.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <atomic>

#include "android_logging.h"
#include "consts.h"
#include "transforms.h"
#include "shaderManager.h"



class Mainview {
public:
    Mainview(AAssetManager* assetManager);
    ~Mainview();

    void setFrame();
    void setupGraphics();

    void loadVectorFieldData(std::vector<float>& vertices);
    void drawVectorField(int size);
    void createVectorFieldBuffer(std::vector<float>& vertices);

    void createParticlesBuffer(std::vector<float>& particlesPos);
    void loadParticlesData(std::vector<float>& particlesPos);
    void drawParticles(int size);

    void createComputeBuffer(std::vector<float>& vector_field0, std::vector<float>& vector_field1, std::vector<float>& vector_field2);
    void loadConstUniforms(float dt, int width, int height, int depth);
    void preloadComputeBuffer(std::vector<float>& vector_field, std::atomic<GLsync>& globalFence);
    void loadComputeBuffer();
    void dispatchComputeShader();

    Transforms& getTransforms() { return *transforms; }

    std::chrono::steady_clock::time_point startTime;

private:
    // Shaders
    ShaderManager *shaderManager;

    // Uniforms
    void loadUniforms();

    GLint isPointLocationLines;
    GLint isPointLocationPoints;
    GLint pointSize;
    GLint modelLocationLines;
    GLint viewLocationLines;
    GLint projectionLocationLines;
    GLint modelLocationPoints;
    GLint viewLocationPoints;
    GLint projectionLocationPoints;
    GLint globalTimeInStepLocation;

    // Buffers
    GLuint particleVBO;
    GLuint particleVAO;

    GLuint vectorFieldVBO;
    GLuint vectorFieldVAO;

    GLuint computeVectorField0SSBO;
    GLuint computeVectorField1SSBO;
    GLuint computeVectorField2SSBO;

    // Transforms
    Transforms *transforms;
};

#endif // GL_SHADER_MANAGER_H
