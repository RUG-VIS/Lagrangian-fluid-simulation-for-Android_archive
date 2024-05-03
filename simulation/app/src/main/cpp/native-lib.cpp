#include <jni.h>
#include <string>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <chrono>
#include <netcdf>
#include <assert.h>

#include "android_logging.h"
#include "netcdf_reader.h"
#include "mainview.h"
#include "triple.h"
#include "particle.h"



std::vector<float> vertices;
std::vector<std::vector<float>> allVertices;
std::vector<std::vector<float>> displayVertices;
int currentFrame = 0;
int numVertices = 0;
int width = 0;
int height = 0;
int depth = 0;
int fineness = 15;
bool started = false;
float dt = 0.05f;

float b = 0.8f;  // Drag coefficient
std::vector<Particle> particles;
std::vector<float> particlesPos;

GLShaderManager* shaderManager;

struct TouchPoint {
    float startX;
    float startY;
    float currentX;
    float currentY;
};

TouchPoint tp;

int frameCount = 0;
float timeCount = 0.0f;

void updateParticlePosArr() {
    particlesPos.clear();
    for (auto& particle : particles) {
        Vec3 particlePos = particle.getPosition();
        particlesPos.push_back(particlePos.x);
        particlesPos.push_back(particlePos.y);
        particlesPos.push_back(particlePos.z);
    }
}

void initParticles(int num) {
    particles.clear();
    particlesPos.clear();
    for (int i = 0; i < num; i++) {

        // Randomly generate initial velocity
//        float aspectRatio = 19.3f / 9.0f;
//        float angle = 2.0f * M_PI * rand() / (float)RAND_MAX;
//        float magnitude = 0.3f * rand() / (float)RAND_MAX;
//        float xVel = magnitude * cos(angle) * aspectRatio;
//        float yVel = magnitude * sin(angle);
//        float zVel = 0.0f;
//        Vec3 initialVel(xVel, yVel, zVel);
//        Vec3 initialPos(-0.25f, 0.25f, 0.0f);

        // Zero initial velocity, diagonal initial position
//        Vec3 initialVel(0.0f, 0.0f, 0.0f);
//        float xPos = 2 * (i / (float) num) - 1;
//        float yPos = 2 * (i / (float) num) - 1;
//        float zPos = 0.0f;
//        Vec3 initialPos(xPos, yPos, zPos);

        // Zero initial velocity, half-diagonal position
        Vec3 initialVel(0.0f, 0.0f, 0.0f);
        float xPos = 2 * (i / (float) num) - 1;
        float yPos = i % 2 ? (i / (float) num) - 1 : 1 - (i / (float) num);
        float zPos = 0.0f;
        Vec3 initialPos(xPos, yPos, zPos);

        particles.push_back(Particle(initialPos, initialVel));
    }

    updateParticlePosArr();
}

void velocityField(Point position, Vec3& velocity) {
    int fineness = 1;  // TODO: Remove once definitely not needed
    int adjWidth = width / fineness;
    int adjHeight = height / fineness;

    // Transform position [-1, 1] range to [0, adjWidth/adjHeight] grid indices
    int gridX = (int)((position.x + 1.0) / 2 * adjWidth);
    int gridY = (int)((position.y + 1.0) / 2 * adjHeight);
    int gridZ = abs((int)(position.z * depth));

    // Ensure indices are within bounds
    gridX = std::max(0, std::min(gridX, adjWidth - 1));
    gridY = std::max(0, std::min(gridY, adjHeight - 1));
    gridZ = std::max(0, std::min(gridZ, adjHeight - 1));

    int idx = gridZ* adjWidth * adjHeight + gridY * adjWidth + gridX;

    // Calculate velocity as differences
    velocity = Vec3(allVertices[currentFrame][idx * 6 + 3] - allVertices[currentFrame][idx * 6],
                    allVertices[currentFrame][idx * 6 + 4] - allVertices[currentFrame][idx * 6 + 1],
                    allVertices[currentFrame][idx * 6 + 5] - allVertices[currentFrame][idx * 6 + 2]
                    );
}

void updateParticles() {
    if (!started) {
        shaderManager->startTime = std::chrono::steady_clock::now();
        started = true;
    }
    auto currentTime = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - shaderManager->startTime).count();
    shaderManager->startTime = currentTime;

    timeCount += deltaTime;
    if (timeCount >= 1.0f) {
        LOGI("Fps: %d\n", frameCount);
        frameCount = 0;
        timeCount = 0.0f;
    }

    for (auto& particle : particles) {
        particle.rk4Step(dt, velocityField, b);
        particle.bindPosition();
    }
}

void prepareVertexData(const std::vector<float>& uData, const std::vector<float>& vData, int width, int height) {

    vertices.clear();
    std::vector<float> tempVertices;

    float maxU = *std::max_element(uData.begin(), uData.end());
    float minU = *std::min_element(uData.begin(), uData.end());
    float maxV = *std::max_element(vData.begin(), vData.end());
    float minV = *std::min_element(vData.begin(), vData.end());

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;

            float normalizedX = (x / (float)(width)) * 2 - 1;
            float normalizedY = (y / (float)(height)) * 2 - 1;

            float scaleFactor = 0.1f;
            float normalizedU = 2 * ((uData[index] - minU) / (maxU - minU)) - 1;
            normalizedU *= scaleFactor;
            float normalizedV = 2 * ((vData[index] - minV) / (maxV - minV)) - 1;
            normalizedV *= scaleFactor;

            float endX = normalizedX + normalizedU;
            float endY = normalizedY + normalizedV;

            // Start point
            vertices.push_back(normalizedX);
            vertices.push_back(normalizedY);
            vertices.push_back(0.0f);

            // End point
            vertices.push_back(endX);
            vertices.push_back(endY);
            vertices.push_back(0.0f);

            if (y % fineness != 0 || x % fineness != 0) continue;
            tempVertices.push_back(normalizedX);
            tempVertices.push_back(normalizedY);
            tempVertices.push_back(0.0f);

            tempVertices.push_back(endX);
            tempVertices.push_back(endY);
            tempVertices.push_back(0.0f);
        }
    }
    numVertices = vertices.size();
    allVertices.push_back(vertices);
    displayVertices.push_back(tempVertices);
}

void prepareVertexData(const std::vector<float>& uData, const std::vector<float>& vData, const std::vector<float>& wData, int width, int height, int depth) {

    vertices.clear();
    std::vector<float> tempVertices;

    float maxU = *std::max_element(uData.begin(), uData.end());
    float minU = *std::min_element(uData.begin(), uData.end());
    float maxV = *std::max_element(vData.begin(), vData.end());
    float minV = *std::min_element(vData.begin(), vData.end());
    float maxW = *std::max_element(wData.begin(), wData.end());
    float minW = *std::min_element(wData.begin(), wData.end());

//    LOGI("Max W: %f, Min W: %f", maxW, minW);
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {

                int index = z * width * height + y * width + x;

                float normalizedX = (x / (float)(width)) * 2 - 1;
                float normalizedY = (y / (float)(height)) * 2 - 1;
                float normalizedZ = 0.0f;

                float scaleFactor = 0.1f;
                float normalizedU = 2 * ((uData[index] - minU) / (maxU - minU)) - 1;
                normalizedU *= scaleFactor;
                float normalizedV = 2 * ((vData[index] - minV) / (maxV - minV)) - 1;
                normalizedV *= scaleFactor;
    //            float normalizedW = 2 * ((wData[index] - minW) / (maxW - minW)) - 1;
                float normalizedW = wData[index];
//                normalizedW *= scaleFactor;

                float endX = normalizedX + normalizedU;
                float endY = normalizedY + normalizedV;
                float endZ = normalizedZ + normalizedW;

                // Start point
                vertices.push_back(normalizedX);
                vertices.push_back(normalizedY);
                vertices.push_back(normalizedZ);

                // End point
                vertices.push_back(endX);
                vertices.push_back(endY);
                vertices.push_back(endZ);

                if (y % fineness != 0 || x % fineness != 0) continue;
                tempVertices.push_back(normalizedX);
                tempVertices.push_back(normalizedY);
                tempVertices.push_back(0.0f);

                tempVertices.push_back(endX);
                tempVertices.push_back(endY);
                tempVertices.push_back(0.0f);
            }
        }
    }
    numVertices = vertices.size();
    allVertices.push_back(vertices);
    displayVertices.push_back(tempVertices);
}

void loadAllTimeSteps(const std::string& fileUPath, const std::string& fileVPath) {
    netCDF::NcFile dataFileU(fileUPath, netCDF::NcFile::read);
    netCDF::NcFile dataFileV(fileVPath, netCDF::NcFile::read);

    LOGI("NetCDF files opened");

    size_t numTimeSteps = dataFileU.getDim("time_counter").getSize();

    for (size_t i = 0; i < 1; i++) {
        std::vector<size_t> startp = {i, 0, 0, 0};  // Start index for time, depth, y, x
        std::vector<size_t> countp = {1, 1, dataFileU.getDim("y").getSize(), dataFileU.getDim("x").getSize()};  // Read one time step, all y, all x
        std::vector<float> uData(countp[2] * countp[3]), vData(countp[2] * countp[3]);

        dataFileU.getVar("vozocrtx").getVar(startp, countp, uData.data());
        dataFileV.getVar("vomecrty").getVar(startp, countp, vData.data());

        // Prepare vertex data for OpenGL from uData and vData, and store in allVertices[i]
        width = countp[3];
        height = countp[2];
        depth = 1;
        prepareVertexData(uData, vData, countp[3], countp[2]);
    }
}

void loadAllTimeSteps(const std::string& fileUPath, const std::string& fileVPath, const std::string& fileWPath) {
    netCDF::NcFile dataFileU(fileUPath, netCDF::NcFile::read);
    netCDF::NcFile dataFileV(fileVPath, netCDF::NcFile::read);
    netCDF::NcFile dataFileW(fileWPath, netCDF::NcFile::read);

    LOGI("NetCDF files opened");

    size_t numTimeSteps = dataFileU.getDim("time_counter").getSize();

    for (size_t i = 0; i < 1; i++) {
        std::vector<size_t> startp = {i, 1, 0, 0};  // Start index for time, depth, y, x
        std::vector<size_t> countp = {1, dataFileU.getDim("depthu").getSize()-1, dataFileU.getDim("y").getSize(), dataFileU.getDim("x").getSize()};  // Read one time step, all depths, all y, all x
        std::vector<float> uData( countp[1] * countp[2] * countp[3]), vData(countp[1] * countp[2] * countp[3]), wData(countp[1] * countp[2] * countp[3]);

        // Prepare vertex data for OpenGL from uData and vData, and store in allVertices[i]
        width = countp[3];
        height = countp[2];
        depth = countp[1];

        dataFileU.getVar("vozocrtx").getVar(startp, countp, uData.data());
        dataFileV.getVar("vomecrty").getVar(startp, countp, vData.data());
        dataFileW.getVar("W").getVar(startp, countp, wData.data());

        prepareVertexData(uData, vData, wData, width, height, depth);
    }
}

void updateFrame() {
    static auto lastUpdate = std::chrono::steady_clock::now(); // Last update time
    static const std::chrono::seconds updateInterval(1);       // Update every 1 second

    auto now = std::chrono::steady_clock::now();
    if (now - lastUpdate >= updateInterval) {                  // Check if 1 second has passed
        currentFrame = (currentFrame + 1) % allVertices.size(); // Update the frame index
        lastUpdate = now;                                      // Reset the last update time
    }
}


extern "C" {
    JNIEXPORT void JNICALL Java_com_example_lagrangianfluidsimulation_MainActivity_drawFrame(JNIEnv* env, jobject /* this */) {
        shaderManager->setFrame();

        shaderManager->loadVectorFieldData(displayVertices[currentFrame]);
        shaderManager->drawVectorField(displayVertices[currentFrame].size());

        updateParticles();
        updateParticlePosArr();
        shaderManager->loadParticlesData(particlesPos);
        shaderManager->drawParticles(particlesPos.size());

        //        updateFrame();

        frameCount++;
    }

    JNIEXPORT void JNICALL Java_com_example_lagrangianfluidsimulation_MainActivity_setupGraphics(JNIEnv* env, jobject obj, jobject assetManager) {
        shaderManager = new GLShaderManager(AAssetManager_fromJava(env, assetManager));
        shaderManager->setupGraphics();
        LOGI("Graphics setup complete");
    }

    JNIEXPORT void JNICALL
    Java_com_example_lagrangianfluidsimulation_FileAccessHelper_initializeNetCDFVisualization(
            JNIEnv* env, jobject /* this */, jint fdU, jint fdV) {

        NetCDFReader reader;
        std::string tempFileU = reader.writeTempFileFromFD(fdU, "tempU.nc");
        std::string tempFileV = reader.writeTempFileFromFD(fdV, "tempV.nc");

        if (tempFileU.empty() || tempFileV.empty()) {
            LOGE("Failed to create temporary files.");
            return;
        }

        loadAllTimeSteps(tempFileU, tempFileV);
        LOGI("NetCDF files loaded of width: %d, height: %d", width, height);

        initParticles(1000);
        LOGI("Particles initialized");
    }

    JNIEXPORT void JNICALL
    Java_com_example_lagrangianfluidsimulation_FileAccessHelper_initializeNetCDFVisualization3D(
            JNIEnv* env, jobject /* this */, jint fdU, jint fdV, jint fdW) {

        NetCDFReader reader;
        std::string tempFileU = reader.writeTempFileFromFD(fdU, "tempU.nc");
        std::string tempFileV = reader.writeTempFileFromFD(fdV, "tempV.nc");
        std::string tempFileW = reader.writeTempFileFromFD(fdW, "tempW.nc");

        if (tempFileU.empty() || tempFileV.empty() || tempFileW.empty()) {
            LOGE("Failed to create temporary files.");
            return;
        }

        loadAllTimeSteps(tempFileU, tempFileV, tempFileW);
        LOGI("NetCDF files loaded");

        initParticles(100);
        LOGI("Particles initialized");
    }

    JNIEXPORT void JNICALL
    Java_com_example_lagrangianfluidsimulation_MainActivity_createBuffers(JNIEnv *env, jobject thiz) {
        shaderManager->createVectorFieldBuffer(allVertices[currentFrame]);
        shaderManager->createParticlesBuffer(particlesPos);
        LOGI("Buffers created");
    }

    JNIEXPORT void JNICALL
    Java_com_example_lagrangianfluidsimulation_MainActivity_nativeSendTouchEvent(JNIEnv *env, jobject obj, jint pointerCount, jfloatArray xArray, jfloatArray yArray, jint action) {
        jfloat* x = env->GetFloatArrayElements(xArray, nullptr);
        jfloat* y = env->GetFloatArrayElements(yArray, nullptr);


        // 0 -> click
        // 1 -> release
        // 2 -> move

        LOGI("Touch event: %d", action);
        if (action == 0) {
            tp.startX = x[0];
            tp.startY = y[0];
            tp.currentX = x[0];
            tp.currentY = y[0];

        } else if (action == 1) {
            tp.startX = 0.0f;
            tp.startY = 0.0f;
            tp.currentX = 0.0f;
            tp.currentY = 0.0f;

        } else if (action == 2) {
            tp.currentX = x[0];
            tp.currentY = y[0];

            float rotSensitivity = 0.001f;
            float dx = tp.currentX - tp.startX;
            float dy = tp.currentY - tp.startY;
            shaderManager->setRotation(rotSensitivity*dy, rotSensitivity*dx, 0.0f);
        }

        if (pointerCount == 1) {
            LOGI("Touch event: %f, %f", x[0], y[0]);
        } else if (pointerCount == 2) {
            LOGI("Touch event: %f, %f, %f, %f", x[0], y[0], x[1], y[1]);
        }

        env->ReleaseFloatArrayElements(xArray, x, 0);
        env->ReleaseFloatArrayElements(yArray, y, 0);
    }
} // extern "C"
