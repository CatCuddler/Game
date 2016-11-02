#pragma once

#include <Kore/TextureImpl.h>
#include <Kore/VertexBufferImpl.h>
#include <Kore/IndexBufferImpl.h>
#include <Kore/Graphics/Texture.h>
#include <Kore/Graphics/Shader.h>
#include "ObjLoader.h"


namespace Kore {
    enum EntityMeshOcclusionState {
        Hidden, Visible, Waiting
    };
}
class MeshObject {
    
private:
    Kore::VertexBuffer* vertexBuffer;               // Mesh Vertex Buffer
    Kore::VertexBuffer* vertexBoundingBoxBuffer;    // Bounding Box Vertex Buffer
    Kore::IndexBuffer* indexBuffer;
    
    int trianglesCount;
    float* boundingBoxVertices;
    
    Mesh* mesh;
    Kore::Texture* image;
    
    Kore::uint occlusionQuery;
    Kore::uint pixelCount;
    
public:
    
    bool occluded;
    Kore::EntityMeshOcclusionState occlusionState;
    
    MeshObject(const char* meshFile, const char* textureFile, const Kore::VertexStructure& structure, float scale = 1.0f);
    ~MeshObject();
    
    void renderOcclusionQuery();
    void render(Kore::TextureUnit tex);
    
    Kore::mat4 M; // Model matrix
    
};
