#pragma once

#include <Kore/TextureImpl.h>
#include <Kore/VertexBufferImpl.h>
#include <Kore/IndexBufferImpl.h>
#include <Kore/Graphics4/Texture.h>
#include <Kore/Math/Matrix.h>

#include "ObjLoader.h"


namespace Kore {
    enum EntityMeshOcclusionState {
        Hidden, Visible, Waiting
    };
}
class MeshObject {
    
private:
    Kore::Graphics4::VertexBuffer* vertexBuffer;               // Mesh Vertex Buffer
//    Kore::Graphics4::VertexBuffer* vertexBoundingBoxBuffer;    // Bounding Box Vertex Buffer
    Kore::Graphics4::IndexBuffer* indexBuffer;
    
//    int trianglesCount;
//    float* boundingBoxVertices;
    
    Mesh* mesh;
    Kore::Graphics4::Texture* image;
    
//    Kore::uint occlusionQuery;

public:
//	bool useQueries;
//    bool occluded;
//    Kore::EntityMeshOcclusionState occlusionState;
    
    MeshObject(const char* meshFile, const char* textureFile, const Kore::Graphics4::VertexStructure& structure, float scale = 1.0f);
    ~MeshObject();
    
//    void renderOcclusionQuery();
    void render(Kore::Graphics4::TextureUnit tex);
	
    Kore::mat4 M; // Model matrix
    
    Kore::uint pixelCount; // should be private
    
};
