#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/System.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/Graphics4/Graphics.h>
#include <Kore/Graphics1/Color.h>
#include <Kore/Log.h>

#include "MeshObject.h"

#ifdef VR_RIFT
#include <Kore/Math/Quaternion.h>
#include <Kore/Vr/VrInterface.h>
#include <Kore/Vr/SensorState.h>
#endif

using namespace Kore;
using namespace Kore::Graphics4;

#define CAMERA_ROTATION_SPEED 0.001

namespace {
    const int width = 1024;
    const int height = 768;
    double startTime;
    Shader* vertexShader;
    Shader* fragmentShader;
    PipelineState* pipeline;
    
    // null terminated array of MeshObject pointers
    MeshObject* objects[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    int renderObjectNum = 0;

	MeshObject* tiger;

	bool sit = false;
    
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

	bool debug = false;
    
	void initCamera() {
#ifdef VR_RIFT
		playerPosition = vec3(0, 0, 0);
#else
		playerPosition = vec3(0, 0, 20); 
#endif
		globe = vec3(0, Kore::pi, 0);
	}

#ifdef VR_RIFT
	mat4 getViewMatrix(SensorState* state) {

		Quaternion orientation = state->pose->vrPose->orientation;
		vec3 position = state->pose->vrPose->position;

		if (debug) {
			log(Info, "Pos %f %f %f", position.x(), position.y(), position.z());
			log(Info, "Player Pos %f %f %f", playerPosition.x(), playerPosition.y(), playerPosition.z());
		}

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
		float left = state->pose->vrPose->left;
		float right = state->pose->vrPose->right;
		float bottom = state->pose->vrPose->bottom;
		float top = state->pose->vrPose->top;

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

		Graphics4::begin();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);

		Graphics4::setPipeline(pipeline);
		
#ifdef VR_RIFT

		VrInterface::begin();
		for (int eye = 0; eye < 2; ++eye) {
			VrInterface::beginRender(eye);

			SensorState* state = VrInterface::getSensorState(eye);
			mat4 view = getViewMatrix(state);
			mat4 proj = getProjectionMatrix(state);

			Graphics4::setMatrix(vLocation, view);
			Graphics4::setMatrix(pLocation, proj);

			// Render world
			Graphics4::setMatrix(mLocation, tiger->M);
			tiger->render(tex);

			VrInterface::endRender(eye);
		}

		VrInterface::warpSwap();

#else
        
        // projection matrix
        mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.1f, 100);
        
        // view matrix
        vec3 lookAt = playerPosition + vec3(0, 0, -1);
        mat4 V = mat4::lookAt(playerPosition, lookAt, vec3(0, 1, 0));
        V *= mat4::Rotation(globe.x(), globe.y(), globe.z());
        
        Graphics4::setMatrix(vLocation, V);
        Graphics4::setMatrix(pLocation, P);
		
		Graphics4::setMatrix(mLocation, tiger->M);
		tiger->render(tex);
        
/*        Graphics::setColorMask(false, false, false, false);
        Graphics::setRenderState(DepthWrite, false);
        
        // draw bounding box for each object
        MeshObject** boundingBox = &objects[0];
        while (*boundingBox != nullptr) {
            // set the model matrix
            Graphics4::setMatrix(mLocation, (*boundingBox)->M);
            
            if ((*boundingBox)->useQueries) {
                    (*boundingBox)->renderOcclusionQuery();
            }
            
            ++boundingBox;
        }
        
        Graphics4::setColorMask(true, true, true, true);
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
            Graphics4::setMatrix(mLocation, (*current)->M);
            
            if ((*current)->occluded) {
                (*current)->render(tex);
                ++renderCount;
            }
            
            ++current;
        }
        renderObjectNum = renderCount;*/
        
#endif
		Graphics4::setPipeline(pipeline);
		
		Graphics4::end();
		Graphics4::swapBuffers();
    }
	
	void keyDown(KeyCode code) {
		switch (code)
		{
		case Kore::KeyLeft:
		case Kore::KeyA:
			left = true;
			break;
		case Kore::KeyRight:
		case Kore::KeyD:
			right = true;
			break;
		case Kore::KeyUp:
			up = true;
			break;
		case Kore::KeyDown:
			down = true;
			break;
		case Kore::KeyW:
			forward = true;
			break;
		case Kore::KeyS:
			backward = true;
			break;
		case Kore::KeyR:
			initCamera();
#ifdef VR_RIFT
			VrInterface::resetHmdPose();
#endif
			break;
		case KeyU:
#ifdef VR_RIFT
			sit = !sit;
			if (sit) VrInterface::updateTrackingOrigin(TrackingOrigin::Sit);
			else VrInterface::updateTrackingOrigin(TrackingOrigin::Stand);
			log(Info, sit ? "Sit" : "Stand up");
#endif
			break;
		case KeyL:
			Kore::log(Kore::LogLevel::Info, "Position: (%.2f, %.2f, %.2f) - Rotation: (%.2f, %.2f, %.2f)", playerPosition.x(), playerPosition.y(), playerPosition.z(), globe.x(), globe.y(), globe.z());
			log(Info, "Render Object Count: %i", renderObjectNum);
			log(Info, "pixel %u %u\n", objects[0]->pixelCount, objects[1]->pixelCount);
			break;
		default:
			break;
		}
	}

	void keyUp(KeyCode code) {
		switch (code)
		{
		case Kore::KeyLeft:
		case Kore::KeyA:
			left = false;
			break;
		case Kore::KeyRight:
		case Kore::KeyD:
			right = false;
			break;
		case Kore::KeyUp:
			up = false;
			break;
		case Kore::KeyDown:
			down = false;
			break;
		case Kore::KeyW:
			forward = false;
			break;
		case Kore::KeyS:
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
        
		pipeline = new PipelineState;
		pipeline->inputLayout[0] = &structure;
		pipeline->inputLayout[1] = nullptr;
		pipeline->vertexShader = vertexShader;
		pipeline->fragmentShader = fragmentShader;
		pipeline->depthMode = ZCompareLess;
		pipeline->depthWrite = true;
		pipeline->blendSource = Graphics4::SourceAlpha;
		pipeline->blendDestination = Graphics4::InverseSourceAlpha;
		pipeline->alphaBlendSource = Graphics4::SourceAlpha;
		pipeline->alphaBlendDestination = Graphics4::InverseSourceAlpha;
		pipeline->compile();
		
        tex = pipeline->getTextureUnit("tex");
        
		pLocation = pipeline->getConstantLocation("P");
        vLocation = pipeline->getConstantLocation("V");
        mLocation = pipeline->getConstantLocation("M");
        
		objects[0] = new MeshObject("earth.obj", "earth.png", structure, 1.0f);
		objects[0]->M = mat4::Translation(10.0f, 0.0f, 0.0f);
		objects[1] = new MeshObject("earth.obj", "earth.png", structure, 3.0f);
		objects[1]->M = mat4::Translation(-10.0f, 0.0f, 0.0f);

		tiger = new MeshObject("tiger.obj", "tigeratlas.jpg", structure);
		tiger->M = mat4::Translation(0.0, 0.0, -5.0);
        
		
        Graphics4::setTextureAddressing(tex, Graphics4::U, Repeat);
        Graphics4::setTextureAddressing(tex, Graphics4::V, Repeat);
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
