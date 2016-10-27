#include "pch.h"

//#include <Kore/Application.h>
#include <Kore/IO/FileReader.h>
#include <Kore/Math/Core.h>
#include <Kore/System.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/Audio/Mixer.h>
#include <Kore/Graphics/Image.h>
#include <Kore/Graphics/Graphics.h>
#include <Kore/Log.h>
#include "ObjLoader.h"
#include "MeshObject.h"

using namespace Kore;

#define CAMERA_ROTATION_SPEED_X 0.001
#define CAMERA_ROTATION_SPEED_Y 0.001

namespace {
    const int width = 1024;
    const int height = 768;
    Shader* vertexShader;
    Shader* fragmentShader;
    Program* program;
    MeshObject* sphere;
    TextureUnit tex;
    
    Kore::ConstantLocation cl_modelViewMatrix;
    Kore::ConstantLocation cl_normalMatrix;
    
    Kore::ConstantLocation cl_lightPos;
    
    double startTime;
    float lastT = 0;
    
    const float movementSpeed = 10;
    
    vec3 cameraPosition;
    vec3 lookAt;
    
    float cameraRotX = 0;
    float cameraRotY = 0;
    float cameraRotZ = 0;
    
    float lightPosX;
    float lightPosY;
    float lightPosZ;
    
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
        
        cameraRotX = 0;
        cameraRotY = Kore::pi;
        cameraRotZ = 0;
    }
    
    void rotate3d(float &x, float &y, float &z, float rx, float ry, float rz) {
        float d1x = Kore::cos(ry) * x + Kore::sin(ry) * z;
        float d1y = y;
        float d1z = Kore::cos(ry) * z - Kore::sin(ry) * x;
        float d2x = d1x;
        float d2y = Kore::cos(rx) * d1y - Kore::sin(rx) * d1z;
        float d2z = Kore::cos(rx) * d1z + Kore::sin(rx) * d1y;
        float d3x = Kore::cos(rz) * d2x - Kore::sin(rz) * d2y;
        float d3y = Kore::cos(rz) * d2y + Kore::sin(rz) * d2x;
        float d3z = d2z;
        
        x = d3x;
        y = d3y;
        z = d3z;
    }
    
    void update() {
        float t = (float)(System::time() - startTime);
        
        float deltaT = t - lastT;
        lastT = t;
        
        //Kore::Audio::update();
        
        Graphics::begin();
        //Graphics::clear(Graphics::ClearColorFlag | Graphics::ClearDepthFlag, 0xff000000);
        Graphics::clear(Graphics::ClearColorFlag | Graphics::ClearDepthFlag | Graphics::ClearStencilFlag, 0xFF000000, 1.0f, 0);
        
        program->set();
        Graphics::setBlendingMode(SourceAlpha, Kore::BlendingOperation::InverseSourceAlpha);
        Graphics::setRenderState(BlendingState, true);
        Graphics::setRenderState(DepthTest, true);
        
        //update camera:
        float cameraMovementX = 0;
        float cameraMovementY = 0;
        float cameraMovementZ = 0;
        
        if (moveUp)
            cameraMovementY += deltaT * movementSpeed;
        if (moveDown)
            cameraMovementY -= deltaT * movementSpeed;
        if (moveLeft)
            cameraMovementX -= deltaT * movementSpeed;
        if (moveRight)
            cameraMovementX += deltaT * movementSpeed;
        if (moveForward)
            cameraMovementZ += deltaT * movementSpeed;
        if (moveBackward)
            cameraMovementZ -= deltaT * movementSpeed;
        
        // rotate direction according to current rotation
        rotate3d(cameraMovementX, cameraMovementY, cameraMovementZ, -cameraRotX, 0, -cameraRotZ);
        rotate3d(cameraMovementX, cameraMovementY, cameraMovementZ, 0, -cameraRotY, -cameraRotZ);
        
        cameraPosition.x() += cameraMovementX;
        cameraPosition.y() += cameraMovementY;
        cameraPosition.z() += cameraMovementZ;
        
        // prepare model view matrix and pass it to shaders
        Kore::mat4 modelView = Kore::mat4::RotationZ(cameraRotZ)
        * Kore::mat4::RotationX(cameraRotX)
        * Kore::mat4::RotationY(cameraRotY)
        * Kore::mat4::Translation(-cameraPosition.x(), -cameraPosition.y(), -cameraPosition.z());
        
        Graphics::setMatrix(cl_modelViewMatrix, modelView);
        
        // prepare normal matrix and pass it to shaders
        Kore::mat4 normalMatrix = modelView;
        normalMatrix.Invert();
        normalMatrix = normalMatrix.Transpose();
        Graphics::setMatrix(cl_normalMatrix, normalMatrix);
        
        // update light pos
        lightPosX = 20;// * Kore::sin(2 * t);
        lightPosY = 10;
        lightPosZ = 20;// * Kore::cos(2 * t);
        Graphics::setFloat3(cl_lightPos, lightPosX, lightPosY, lightPosZ);
        
        // check if objects should be rendered
        //sphere->checkRender();
        //log(Kore::Info, "Render sphere %i", sphere->renderObj);
        
        // render
        //if (sphere->renderObj) {
            sphere->render(tex, 0);
        //}
        
        
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
                Kore::log(Kore::LogLevel::Info, "Position: (%.2f, %.2f, %.2f) - Rotation: (%.2f, %.2f, %.2f)\n", cameraPosition.x(), cameraPosition.y(), cameraPosition.z(), cameraRotX, cameraRotY, cameraRotZ);
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
            cameraRotX += (float)((mousePressY - y) * CAMERA_ROTATION_SPEED_X);
            cameraRotY += (float)((mousePressX - x) * CAMERA_ROTATION_SPEED_Y);
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
        
        cl_modelViewMatrix = program->getConstantLocation("modelViewMatrix");
        cl_normalMatrix = program->getConstantLocation("normalMatrix");
        cl_lightPos = program->getConstantLocation("lightPos");
        
        sphere = new MeshObject("sphere.obj", "sand.png", structure, cl_modelViewMatrix, 5.0f);
        
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
    Kore::Mixer::init();
    Kore::Audio::init();
    //Kore::Mixer::play(new SoundStream("back.ogg", true));
    
    Keyboard::the()->KeyDown = keyDown;
    Keyboard::the()->KeyUp = keyUp;
    Mouse::the()->Move = mouseMove;
    Mouse::the()->Press = mousePress;
    Mouse::the()->Release = mouseRelease;
    
    initCamera();
    
    Kore::System::start();
    
    return 0;
}
