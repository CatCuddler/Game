#include "pch.h"
#include "MeshObject.h"

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Log.h>

using namespace Kore;
using namespace Kore::Graphics4;

MeshObject::MeshObject(const char* meshFile, const char* textureFile, const VertexStructure& structure, float scale) : occlusionQuery(0), pixelCount(0),occlusionState(Visible), occluded(true), useQueries(true) {
	mesh = loadObj(meshFile);
	image = new Texture(textureFile, true);
	
	// Mesh Vertex Buffer
	vertexBuffer = new VertexBuffer(mesh->numVertices, structure, 0);
	float* vertices = vertexBuffer->lock();
	
	float min_x, max_x, min_y;
	float max_y, min_z, max_z;
	min_x = max_x = mesh->vertices[0];
	min_y = max_y = mesh->vertices[1];
	min_z = max_z = mesh->vertices[2];
	
	for (int i = 0; i < mesh->numVertices; ++i) {
		vertices[i * 8 + 0] = mesh->vertices[i * 8 + 0] * scale;
		vertices[i * 8 + 1] = mesh->vertices[i * 8 + 1] * scale;
		vertices[i * 8 + 2] = mesh->vertices[i * 8 + 2] * scale;
		vertices[i * 8 + 3] = mesh->vertices[i * 8 + 3];
		vertices[i * 8 + 4] = 1.0f - mesh->vertices[i * 8 + 4];
		vertices[i * 8 + 5] = mesh->vertices[i * 8 + 5];
		vertices[i * 8 + 6] = mesh->vertices[i * 8 + 6];
		vertices[i * 8 + 7] = mesh->vertices[i * 8 + 7];
		
		if (vertices[i * 8 + 0] < min_x) min_x = vertices[i * 8 + 0];
		if (vertices[i * 8 + 0] > max_x) max_x = vertices[i * 8 + 0];
		if (vertices[i * 8 + 1] < min_y) min_y = vertices[i * 8 + 1];
		if (vertices[i * 8 + 1] > max_y) max_y = vertices[i * 8 + 1];
		if (vertices[i * 8 + 2] < min_z) min_z = vertices[i * 8 + 2];
		if (vertices[i * 8 + 2] > max_z) max_z = vertices[i * 8 + 2];
	}
	vertexBuffer->unlock();
	
	indexBuffer = new IndexBuffer(mesh->numFaces * 3);
	int* indices = indexBuffer->lock();
	for (int i = 0; i < mesh->numFaces * 3; ++i) {
		indices[i] = mesh->indices[i];
	}
	indexBuffer->unlock();
	
	// Bounding Box Vertex Buffer
	VertexStructure str;
	str.add("pos", Float3VertexData);
	trianglesCount = 12 * 3;
	vertexBoundingBoxBuffer = new VertexBuffer(trianglesCount * 3, str, 0);
	boundingBoxVertices = vertexBoundingBoxBuffer->lock();
	
	float boundingBox[] = {
		min_x, min_y, min_z,    min_x, min_y, max_z,    min_x, max_y, max_z,
		max_x, max_y, min_z,    min_x, min_y, min_z,    min_x, max_y, min_z,
		max_x, min_y, max_z,    min_x, min_y, min_z,    max_x, min_y, min_z,
		max_x, max_y, min_z,    max_x, min_y, min_z,    min_x, min_y, min_z,
		min_x, min_y, min_z,    min_x, max_y, max_z,    min_x, max_y, min_z,
		max_x, min_y, max_z,    min_x, min_y, max_z,    min_x, min_y, min_z,
		min_x, max_y, max_z,    min_x, min_y, max_z,    max_x, min_y, max_z,
		max_x, max_y, max_z,    max_x, min_y, min_z,    max_x, max_y, min_z,
		max_x, min_y, min_z,    max_x, max_y, max_z,    max_x, min_y, max_z,
		max_x, max_y, max_z,    max_x, max_y, min_z,    min_x, max_y, min_z,
		max_x, max_y, max_z,    min_x, max_y, min_z,    min_x, max_y, max_z,
		max_x, max_y, max_z,    min_x, max_y, max_z,    max_x, min_y, max_z };
	
	for (int v = 0; v < trianglesCount; v++) {
		boundingBoxVertices[v * 3 + 0] = boundingBox[v * 3 + 0];
		boundingBoxVertices[v * 3 + 1] = boundingBox[v * 3 + 1];
		boundingBoxVertices[v * 3 + 2] = boundingBox[v * 3 + 2];
	}
	vertexBoundingBoxBuffer->unlock();
	
	indexBoundingBoxBuffer = new IndexBuffer(trianglesCount);
	int* indicesBoundingBox = indexBoundingBoxBuffer->lock();
	for (int i = 0; i < trianglesCount; ++i) {
		indicesBoundingBox[i] = i;
	}
	indexBoundingBoxBuffer->unlock();
	
	useQueries = Graphics4::initOcclusionQuery(&occlusionQuery);
	
	M = mat4::Identity();
}

MeshObject::~MeshObject() {
	Graphics4::deleteOcclusionQuery(occlusionQuery);
}

void MeshObject::renderOcclusionQuery() {
 
 if (!useQueries) {
	 occluded = true;
 } else {
	 if (occlusionState != Waiting) {
		 occlusionState = Waiting;
		 Graphics4::setVertexBuffer(*vertexBoundingBoxBuffer);
		 Graphics4::setIndexBuffer(*indexBoundingBoxBuffer);
		 Graphics4::renderOcclusionQuery(occlusionQuery, trianglesCount);
	 }
	 
	 bool available = Graphics4::isQueryResultsAvailable(occlusionQuery);
	 if (available) {
		 Graphics4::getQueryResults(occlusionQuery, &pixelCount);
		 if (pixelCount > 0) {
			 occluded = true;
			 occlusionState = Visible;
		 } else {
			 occluded = false;
			 occlusionState = Hidden;
		 }
	 } else {
		 // If Query is not done yet; avoid wait by continuing with worst - case scenario.
		 //occluded = true;
		 //occlusionState = Visible;
	 }
 }
 
}

void MeshObject::render(TextureUnit tex) {
	Graphics4::setTexture(tex, image);
	Graphics4::setVertexBuffer(*vertexBuffer);
	Graphics4::setIndexBuffer(*indexBuffer);
	Graphics4::drawIndexedVertices();
}

void MeshObject::render2(TextureUnit tex) {
	Graphics4::setTexture(tex, image);
	Graphics4::setVertexBuffer(*vertexBoundingBoxBuffer);
	Graphics4::setIndexBuffer(*indexBoundingBoxBuffer);
	Graphics4::drawIndexedVertices();
}





