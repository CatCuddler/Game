#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/System.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/Graphics3/Graphics.h>
#include <Kore/Graphics4/Graphics.h>
#include <Kore/Graphics1/Color.h>
#include <Kore/Log.h>

#include "MeshObject.h"

#ifdef KORE_OCULUS
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
	PipelineState* pipelineOcclusion;
	
	// null terminated array of MeshObject pointers
	MeshObject* objects[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	int renderObjectNum = 0;
	
	MeshObject* tiger;
	
	bool sit = false;
	
	VertexStructure structure;
	VertexStructure structureBB;
	
	// uniform locations - add more as you see fit
	TextureUnit tex;
	ConstantLocation pLocation;
	ConstantLocation vLocation;
	ConstantLocation mLocation;
	ConstantLocation pLocationBoundingBox;
	ConstantLocation vLocationBoundingBox;
	ConstantLocation mLocationBoundingBox;
	ConstantLocation colorBoundingBox;
	
	vec3 playerPosition;
	vec3 globe;
	
	bool left, right, up, down, forward, backward;
	
	bool rotate = false;
	int mousePressX, mousePressY;
	
	bool debug = false;
	
	void initCamera() {
#ifdef KORE_OCULUS
		playerPosition = vec3(0, 0, 0);
#else
		playerPosition = vec3(0, 0, 20);
#endif
		globe = vec3(0, Kore::pi, 0);
	}
	
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
		
#ifdef KORE_OCULUS
		
		VrInterface::begin();
		for (int eye = 0; eye < 2; ++eye) {
			VrInterface::beginRender(eye);
			
			SensorState state = VrInterface::getSensorState(eye);
			mat4 view = state.pose.vrPose.eye;
			mat4 proj = state.pose.vrPose.projection;
			
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
		
		// draw bounding box for each object
		Graphics4::setPipeline(pipelineOcclusion);
		
		Graphics4::setMatrix(vLocationBoundingBox, V);
		Graphics4::setMatrix(pLocationBoundingBox, P);
		Graphics4::setFloat4(colorBoundingBox, vec4(1, 0, 0, 1));
		
		MeshObject** boundingBox = &objects[0];
		while (*boundingBox != nullptr) {
			// set the model matrix
			Graphics4::setMatrix(mLocationBoundingBox, (*boundingBox)->M);
			if ((*boundingBox)->useQueries) {
				(*boundingBox)->renderOcclusionQuery();
			}
			++boundingBox;
		}
		
		// draw real objects
		Graphics4::setPipeline(pipeline);
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		
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
		renderObjectNum = renderCount;
		
#endif
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
#ifdef KORE_OCULUS
				VrInterface::resetHmdPose();
#endif
				break;
			case KeyU:
#ifdef KORE_OCULUS
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
	
	void initShader() {
		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);
		
		// This defines the structure of your Vertex Buffer
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
	}
	
	void initOccluderShader() {
		FileReader vs("occluders.vert");
		FileReader fs("occluders.frag");
		vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);
		
		// This defines the structure of your Vertex Buffer
		structureBB.add("pos", Float3VertexData);
		
		pipelineOcclusion = new PipelineState;
		pipelineOcclusion->inputLayout[0] = &structureBB;
		pipelineOcclusion->inputLayout[1] = nullptr;
		pipelineOcclusion->vertexShader = vertexShader;
		pipelineOcclusion->fragmentShader = fragmentShader;
		pipelineOcclusion->depthMode = ZCompareLess;
		pipelineOcclusion->depthWrite = false;
		pipelineOcclusion->blendSource = Graphics4::SourceAlpha;
		pipelineOcclusion->blendDestination = Graphics4::InverseSourceAlpha;
		pipelineOcclusion->alphaBlendSource = Graphics4::SourceAlpha;
		pipelineOcclusion->alphaBlendDestination = Graphics4::InverseSourceAlpha;
		pipelineOcclusion->colorWriteMaskRed = false;
		pipelineOcclusion->colorWriteMaskGreen = false;
		pipelineOcclusion->colorWriteMaskBlue = false;
		pipelineOcclusion->colorWriteMaskAlpha = false;
		pipelineOcclusion->compile();
		
		pLocationBoundingBox = pipelineOcclusion->getConstantLocation("P");
		vLocationBoundingBox = pipelineOcclusion->getConstantLocation("V");
		mLocationBoundingBox = pipelineOcclusion->getConstantLocation("M");
		colorBoundingBox = pipelineOcclusion->getConstantLocation("vColor");
	}
	
	void init() {
		initShader();
		initOccluderShader();
		
#ifdef KORE_OCULUS
		tiger = new MeshObject("tiger.obj", "tigeratlas.jpg", structure);
		tiger->M = mat4::Translation(0.0, 0.0, -5.0);
#else
		objects[0] = new MeshObject("earth.obj", "earth.png", structure, 1.0f);
		objects[0]->M = mat4::Translation(10.0f, 0.0f, 0.0f);
		objects[1] = new MeshObject("earth.obj", "earth.png", structure, 3.0f);
		objects[1]->M = mat4::Translation(-10.0f, 0.0f, 0.0f);
#endif
		
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
