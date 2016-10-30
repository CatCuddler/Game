#pragma once

#include <Kore/TextureImpl.h>
#include <Kore/VertexBufferImpl.h>
#include <Kore/IndexBufferImpl.h>
#include <Kore/Graphics/Texture.h>
#include <Kore/Graphics/Shader.h>
#include "ObjLoader.h"

class MeshObject {
    
private:
    Kore::VertexBuffer* vertexBuffer;
    Kore::IndexBuffer* indexBuffer;
    
    Mesh* mesh;
    Kore::Texture* image;
    
    Kore::uint occlusionQuery;
    
    float min_x;
    float max_x;
    float min_y;
    float max_y;
    float min_z;
    float max_z;
    
public:
    
    bool renderObj;
    
    MeshObject(const char* meshFile, const char* textureFile, const Kore::VertexStructure& structure, float scale = 1.0f);
    
    void checkRender();
    void render(Kore::TextureUnit tex);
    
    Kore::mat4 M; // Model matrix
    
};
