#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Math/Core.h>
#include <Kore/System.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/Graphics/Image.h>
#include <Kore/Graphics/Graphics.h>
#include <Kore/Log.h>
#include "MeshObject.h"

using namespace Kore;

#define CAMERA_ROTATION_SPEED 0.001

namespace {
    const int width = 1024;
    const int height = 768;
    Shader* vertexShader;
    Shader* fragmentShader;
    Program* program;
    
    const int numObjects = 2;
    MeshObject* objects[numObjects] = { nullptr, nullptr };
    TextureUnit tex;
    
    Kore::ConstantLocation mLocation;
    Kore::ConstantLocation vLocation;
    Kore::ConstantLocation pLocation;
    
    double startTime;
    float lastT = 0;
    
    const float movementSpeed = 10;
    
    vec3 cameraPosition;
    vec3 cameraRotation;
    vec3 lookAt;
    
    bool moveUp = false;
    bool moveDown = false;
    bool moveRight = false;
    bool moveLeft = false;
    bool moveForward = false;
    bool moveBackward = false;
    
    bool rotate = false;
    int mousePressX, mousePressY;
    
    void initCamera() {
        cameraPosition = vec3(0, 0, 20);
        cameraRotation = vec3(0, Kore::pi, 0);
    }
    
    void update() {
        float t = (float)(System::time() - startTime);
        
        float deltaT = t - lastT;
        lastT = t;
        
        Graphics::begin();
        //Graphics::clear(Graphics::ClearColorFlag | Graphics::ClearDepthFlag, 0xff000000);
        Graphics::clear(Graphics::ClearColorFlag | Graphics::ClearDepthFlag | Graphics::ClearStencilFlag, 0xFF000000, 1.0f, 0);
        
        program->set();
        Graphics::setBlendingMode(SourceAlpha, Kore::BlendingOperation::InverseSourceAlpha);
        Graphics::setRenderState(BlendingState, true);
        Graphics::setRenderState(DepthTest, true);
        
        if (moveUp)
            cameraPosition.y() += deltaT * movementSpeed;
        if (moveDown)
            cameraPosition.y() -= deltaT * movementSpeed;
        if (moveLeft)
            cameraPosition.x() += deltaT * movementSpeed;
        if (moveRight)
            cameraPosition.x() -= deltaT * movementSpeed;
        if (moveForward)
            cameraPosition.z() += deltaT * movementSpeed;
        if (moveBackward)
            cameraPosition.z() -= deltaT * movementSpeed;
        
        // Projection matrix
        mat4 P = mat4::Perspective(45.0f, (float)width / (float)height, 0.1f, 100);
        
        // Camera matrix
        vec3 lookAt = cameraPosition + vec3(0, 0, -1);
        mat4 V = mat4::lookAt(cameraPosition, lookAt, vec3(0, 1, 0));
        V *= mat4::Rotation(cameraRotation.x(), cameraRotation.y(), cameraRotation.z());
        
        
        Graphics::setMatrix(vLocation, V);
        Graphics::setMatrix(pLocation, P);
        
        // check if object should be rendered
        for(int i = 0; i < numObjects; i++) {
            MeshObject** current = &objects[i];
            // set the model matrix
            Graphics::setMatrix(mLocation, (*current)->M);
            
            (*current)->checkRender();
            //log(Kore::Info, "Render sphere %i %i", i, (*current)->renderObj);
            
            //if (sphere->renderObj) {
            (*current)->render(tex);
            //}
            //++current;
        }
        
        Graphics::end();
        Graphics::swapBuffers();
    }
    
    void keyDown(KeyCode code, wchar_t character) {
        switch (code)
        {
            case Key_Left:
            case Key_A:
                moveLeft = true;
                break;
            case Key_Right:
            case Key_D:
                moveRight = true;
                break;
            case Key_Up:
                moveUp = true;
                break;
            case Key_Down:
                moveDown = true;
                break;
            case Key_W:
                moveForward = true;
                break;
            case Key_S:
                moveBackward = true;
                break;
            case Key_R:
                initCamera();
                break;
            case Key_L:
                Kore::log(Kore::LogLevel::Info, "Position: (%.2f, %.2f, %.2f) - Rotation: (%.2f, %.2f, %.2f)\n", cameraPosition.x(), cameraPosition.y(), cameraPosition.z(), cameraRotation.x(), cameraRotation.y(), cameraRotation.z());
                break;
            default:
                break;
        }
    }
    
    void keyUp(KeyCode code, wchar_t character) {
        switch (code)
        {
            case Key_Left:
            case Key_A:
                moveLeft = false;
                break;
            case Key_Right:
            case Key_D:
                moveRight = false;
                break;
            case Key_Up:
                moveUp = false;
                break;
            case Key_Down:
                moveDown = false;
                break;
            case Key_W:
                moveForward = false;
                break;
            case Key_S:
                moveBackward = false;
                break;
            default:
                break;
        }
    }
    
    void mouseMove(int windowId, int x, int y, int movementX, int movementY) {
        if (rotate) {
            cameraRotation.x() += (float)((mousePressX - x) * CAMERA_ROTATION_SPEED);
            cameraRotation.z() += (float)((mousePressY - y) * CAMERA_ROTATION_SPEED);
            mousePressX = x;
            mousePressY = y;
        }
    }
    
    void mousePress(int windowId, int button, int x, int y) {
        rotate = true;
        mousePressX = x;
        mousePressY = y;
    }
    
    void mouseRelease(int windowId, int button, int x, int y) {
        rotate = false;
    }
    
    void init() {
        FileReader vs("shader.vert");
        FileReader fs("shader.frag");
        vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
        fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);
        
        // This defines the structure of your Vertex Buffer
        VertexStructure structure;
        structure.add("pos", Float3VertexData);
        structure.add("tex", Float2VertexData);
        structure.add("nor", Float3VertexData);
        
        program = new Program;
        program->setVertexShader(vertexShader);
        program->setFragmentShader(fragmentShader);
        program->link(structure);
        
        tex = program->getTextureUnit("tex");
        
        mLocation = program->getConstantLocation("M");
        vLocation = program->getConstantLocation("V");
        pLocation = program->getConstantLocation("P");
        
        objects[0] = new MeshObject("earth.obj", "earth.png", structure, 1.0f);
        objects[0]->M = mat4::Translation(10.0f, 0.0f, 0.0f);
        objects[1] = new MeshObject("earth.obj", "earth.png", structure, 3.0f);
        objects[1]->M = mat4::Translation(-10.0f, 0.0f, 0.0f);
        
        Graphics::setRenderState(DepthTest, true);
        Graphics::setRenderState(DepthTestCompare, ZCompareLess);
        
        Graphics::setTextureAddressing(tex, U, Repeat);
        Graphics::setTextureAddressing(tex, V, Repeat);
    }
}

int kore(int argc, char** argv) {
    System::init("Game", width, height);
    
    init();
    
    Kore::System::setCallback(update);
    
    startTime = System::time();
    
    Keyboard::the()->KeyDown = keyDown;
    Keyboard::the()->KeyUp = keyUp;
    Mouse::the()->Move = mouseMove;
    Mouse::the()->Press = mousePress;
    Mouse::the()->Release = mouseRelease;
    
    initCamera();
    
    Kore::System::start();
    
    return 0;
}
