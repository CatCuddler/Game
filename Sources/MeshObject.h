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
    
    Kore::uint occlusionQuery;
    
    Kore::ConstantLocation pLocation;
    
    Mesh* mesh;
    Kore::Texture* image;
    float scale;
    
public:
    float min_x;
    float max_x;
    float min_y;
    float max_y;
    float min_z;
    float max_z;
    
    bool renderObj;
    
    MeshObject(const char* meshFile, const char* textureFile, Kore::VertexStructure structure, Kore::ConstantLocation pLocation, float scale = 1.0f);
    
    
    void checkRender();
    void render(Kore::TextureUnit tex, int instances);
    
};
