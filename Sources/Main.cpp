#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Math/Core.h>
#include <Kore/System.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/Graphics/Image.h>
#include <Kore/Graphics/Graphics.h>
#include <Kore/Graphics/Color.h>
#include <Kore/Log.h>
#include "MeshObject.h"

#ifdef VR_RIFT
#include <Kore/Math/Quaternion.h>
#include <Kore/Vr/VrInterface.h>
#include <Kore/Vr/SensorState.h>
#endif

#include <Kore/Graphics/Graphics.h>

using namespace Kore;

#define CAMERA_ROTATION_SPEED 0.001

namespace {
    const int width = 1024;
    const int height = 768;
    double startTime;
    Shader* vertexShader;
    Shader* fragmentShader;
    Program* program;
    
    // null terminated array of MeshObject pointers
    MeshObject* objects[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    int renderObjectNum = 0;

	MeshObject* tiger;
    
    // uniform locations - add more as you see fit
    TextureUnit tex;
    ConstantLocation pLocation;
    ConstantLocation vLocation;
    ConstantLocation mLocation;

    vec3 playerPosition;
    vec3 globe;
    
    bool left, right, up, down, forward, backward;
	
	bool rotate = false;
	int mousePressX, mousePressY;
    
	void initCamera() {
		playerPosition = vec3(0, 0, 0);
        globe = vec3(0, Kore::pi, 0);
	}

#ifdef VR_RIFT
	mat4 getViewMatrix(SensorState* state) {

		Quaternion orientation = state->predicted->vrPose->orientation;
		vec3 position = state->predicted->vrPose->position;

		// Get view matrices
		mat4 rollPitchYaw = orientation.matrix();
		vec3 up = vec3(0, 1, 0);
		vec3 eyePos = rollPitchYaw * vec4(position.x(), position.y(), position.z(), 0);
		eyePos += playerPosition;
		vec3 lookAt = eyePos + vec3(0, 0, -1);
		mat4 view = mat4::lookAt(eyePos, lookAt, up);

		return view;
	}

	mat4 getOrthogonalProjection(float left, float right, float up, float down, float zn, float zf) {
		float projXScale = 2.0f / (left + right);
		float projXOffset = (left - right) * projXScale * 0.5f;
		float projYScale = 2.0f / (up + down);
		float projYOffset = (up - down) * projYScale * 0.5f;

		bool flipZ = false;

		mat4 m = mat4::Identity();
		m.Set(0, 0, 2 / (left + right));
		m.Set(0, 1, 0);
		m.Set(0, 2, projXOffset);
		m.Set(0, 3, 0);
		m.Set(1, 0, 0);
		m.Set(1, 1, projYScale);
		m.Set(1, 2, -projYOffset);
		m.Set(1, 3, 0);
		m.Set(2, 0, 0);
		m.Set(2, 1, 0);
		m.Set(2, 2, -1.0f * (zn + zf) / (zn - zf));
		m.Set(2, 3, 2.0f * (zf * zn) / (zn - zf));
		m.Set(3, 0, 0.0f);
		m.Set(3, 1, 0.0f);
		m.Set(3, 2, 1.0f);
		m.Set(3, 3, 0.0f);
		return m;
	}

	mat4 getProjectionMatrix(SensorState* state) {
		float left = state->predicted->vrPose->left;
		float right = state->predicted->vrPose->right;
		float bottom = state->predicted->vrPose->bottom;
		float top = state->predicted->vrPose->top;

		// Get projection matrices
		mat4 proj = mat4::Perspective(45, (float)width / (float)height, 0.1f, 100.0f);
		//mat4 proj = mat4::orthogonalProjection(left, right, top, bottom, 0.1f, 100.0f); // top and bottom are same
		//mat4 proj = getOrthogonalProjection(left, right, top, bottom, 0.1f, 100.0f);
		return proj;
	}

#endif

    void update() {
        float t = (float)(System::time() - startTime);

		const float speed = 0.05f;
		if (left) {
			playerPosition.x() -= speed;
		}
		if (right) {
			playerPosition.x() += speed;
		}
		if (forward) {
			playerPosition.z() += speed;
		}
		if (backward) {
			playerPosition.z() -= speed;
		}
		if (up) {
			playerPosition.y() += speed;
		}
		if (down) {
			playerPosition.y() -= speed;
		}

		Graphics::begin();
		Graphics::clear(Graphics::ClearColorFlag | Graphics::ClearDepthFlag, Color::Blue, 1.0f, 0);

		program->set();
		
#ifdef VR_RIFT

		for (int eye = 0; eye < 2; ++eye) {
			VrInterface::begin(eye);

			SensorState* state = VrInterface::getSensorState(eye);

			mat4 view = getViewMatrix(state);
			mat4 proj = getProjectionMatrix(state);

			Graphics::setMatrix(vLocation, view);
			Graphics::setMatrix(pLocation, proj);

			// Render world
			Graphics::setMatrix(mLocation, tiger->M);
			tiger->render(tex);

			VrInterface::end(eye);
		}

		VrInterface::warpSwap();

#else
        
        // projection matrix
        mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.1f, 100);
        
        // view matrix
        vec3 lookAt = playerPosition + vec3(0, 0, -1);
        mat4 V = mat4::lookAt(playerPosition, lookAt, vec3(0, 1, 0));
        V *= mat4::Rotation(globe.x(), globe.y(), globe.z());
        
        Graphics::setMatrix(vLocation, V);
        Graphics::setMatrix(pLocation, P);
        
        Graphics::setColorMask(false, false, false, false);
        Graphics::setRenderState(DepthWrite, false);
        
        // draw bounding box for each object
        MeshObject** boundingBox = &objects[0];
        while (*boundingBox != nullptr) {
            // set the model matrix
            Graphics::setMatrix(mLocation, (*boundingBox)->M);
            
            if ((*boundingBox)->useQueries) {
                    (*boundingBox)->renderOcclusionQuery();
            }
            
            ++boundingBox;
        }
        
        Graphics::setColorMask(true, true, true, true);
        Graphics::setRenderState(DepthWrite, true);
        
        Graphics::setBlendingMode(SourceAlpha, Kore::BlendingOperation::InverseSourceAlpha);
        Graphics::setRenderState(BlendingState, true);
        Graphics::setRenderState(DepthTest, true);
        Graphics::setRenderState(DepthTestCompare, ZCompareLess);
        Graphics::setRenderState(DepthWrite, true);
        
        // draw real objects
        int renderCount = 0;
        MeshObject** current = &objects[0];
        while (*current != nullptr) {
            // set the model matrix
            Graphics::setMatrix(mLocation, (*current)->M);
            
            if ((*current)->occluded) {
                (*current)->render(tex);
                ++renderCount;
            }
            
            ++current;
        }
        renderObjectNum = renderCount;
        
#endif
		Graphics::end();
		Graphics::swapBuffers();
    }
	
	void keyDown(KeyCode code, wchar_t character) {
		switch (code)
		{
		case Key_Left:
		case Key_A:
			left = true;
			break;
		case Key_Right:
		case Key_D:
			right = true;
			break;
		case Key_Up:
			up = true;
			break;
		case Key_Down:
			down = true;
			break;
		case Key_W:
			forward = true;
			break;
		case Key_S:
			backward = true;
			break;
		case Key_R:
			initCamera();
			break;
		case Key_L:
			Kore::log(Kore::LogLevel::Info, "Position: (%.2f, %.2f, %.2f) - Rotation: (%.2f, %.2f, %.2f)", playerPosition.x(), playerPosition.y(), playerPosition.z(), globe.x(), globe.y(), globe.z());
			log(Info, "Render Object Count: %i", renderObjectNum);
			log(Info, "pixel %u %u\n", objects[0]->pixelCount, objects[1]->pixelCount);
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
			left = false;
			break;
		case Key_Right:
		case Key_D:
			right = false;
			break;
		case Key_Up:
			up = false;
			break;
		case Key_Down:
			down = false;
			break;
		case Key_W:
			forward = false;
			break;
		case Key_S:
			backward = false;
			break;
		default:
			break;
		}
	}

	void mouseMove(int windowId, int x, int y, int movementX, int movementY) {
		if (rotate) {
			globe.x() += (float)((mousePressX - x) * CAMERA_ROTATION_SPEED);
			globe.z() += (float)((mousePressY - y) * CAMERA_ROTATION_SPEED);
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
        
		pLocation = program->getConstantLocation("P");
        vLocation = program->getConstantLocation("V");
        mLocation = program->getConstantLocation("M");
        
		objects[0] = new MeshObject("earth.obj", "earth.png", structure, 1.0f);
		objects[0]->M = mat4::Translation(10.0f, 0.0f, 0.0f);
		objects[1] = new MeshObject("earth.obj", "earth.png", structure, 3.0f);
		objects[1]->M = mat4::Translation(-10.0f, 0.0f, 0.0f);

		tiger = new MeshObject("tiger.obj", "tigeratlas.jpg", structure);
		tiger->M = mat4::Translation(0.0, 0.0, -5.0);
        
        Graphics::setRenderState(DepthTest, true);
        Graphics::setRenderState(DepthTestCompare, ZCompareLess);
        
        Graphics::setTextureAddressing(tex, Kore::U, Repeat);
        Graphics::setTextureAddressing(tex, Kore::V, Repeat);
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
