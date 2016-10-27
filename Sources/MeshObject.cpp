#include "pch.h"
#include "MeshObject.h"

#include <Kore/Graphics/Graphics.h>

using namespace Kore;

MeshObject::MeshObject(const char* meshFile, const char* textureFile, Kore::VertexStructure structure, Kore::ConstantLocation pLocation, float scale) : occlusionQuery(0), renderObj(false), scale(scale) {
    mesh = loadObj(meshFile);
    image = new Kore::Texture(textureFile, true);
    
    vertexBuffer = new VertexBuffer(mesh->numVertices, structure, 0);
    float* vertices = vertexBuffer->lock();
    
    min_x = max_x = mesh->vertices[0];
    min_y = max_y = mesh->vertices[1];
    min_z = max_z = mesh->vertices[2];
    
    for (int i = 0; i < mesh->numVertices; ++i) {
        vertices[i * 8 + 0] = mesh->vertices[i * 8 + 0] * scale;
        vertices[i * 8 + 1] = mesh->vertices[i * 8 + 1] * scale;
        vertices[i * 8 + 2] = mesh->vertices[i * 8 + 2] * scale;
        vertices[i * 8 + 3] = mesh->vertices[i * 8 + 3];
        vertices[i * 8 + 4] = mesh->vertices[i * 8 + 4];
        vertices[i * 8 + 5] = mesh->vertices[i * 8 + 5];
        vertices[i * 8 + 6] = mesh->vertices[i * 8 + 6];
        vertices[i * 8 + 7] = mesh->vertices[i * 8 + 7];
        
        if (mesh->vertices[i * 8 + 0] < min_x) min_x = mesh->vertices[i * 8 + 0];
        if (mesh->vertices[i * 8 + 0] > max_x) max_x = mesh->vertices[i * 8 + 0];
        if (mesh->vertices[i * 8 + 1] < min_y) min_y = mesh->vertices[i * 8 + 1];
        if (mesh->vertices[i * 8 + 1] > max_y) max_y = mesh->vertices[i * 8 + 1];
        if (mesh->vertices[i * 8 + 2] < min_z) min_z = mesh->vertices[i * 8 + 2];
        if (mesh->vertices[i * 8 + 2] > max_z) max_z = mesh->vertices[i * 8 + 2];
    }
    vertexBuffer->unlock();
    
    indexBuffer = new IndexBuffer(mesh->numFaces * 3);
    int* indices = indexBuffer->lock();
    for (int i = 0; i < mesh->numFaces * 3; ++i) {
        indices[i] = mesh->indices[i];
    }
    indexBuffer->unlock();
    
    Graphics::initOcclusionQuery(&occlusionQuery);
    
}

void MeshObject::checkRender() {
    // Dont write to color and depth buffer. Only test if the object would be rendered.
    // TODO: put this in main if we have more than one object???
    //Graphics::setColorMask(false, false, false, false);
    //Graphics::setRenderState(DepthWrite, false);
    
    Graphics::setRenderState(DepthTest, true);
    Graphics::renderOcclusionQuery(occlusionQuery, pLocation, min_x, max_x, min_y, max_y, min_z, max_z);
     
    int pixelCount = 0;
    Graphics::getOcclusionResults(occlusionQuery, pixelCount);
    if (pixelCount > 0) {
        renderObj = true;
    }
     
    // Re-enable writing to color buffer and depth buffer
    //Graphics::setColorMask(true, true, true, true);
    //Graphics::setRenderState(DepthWrite, true);
}

void MeshObject::render(TextureUnit tex, int instances) {
    //Graphics::setTexture(tex, image);
    Graphics::setVertexBuffer(*vertexBuffer);
    Graphics::setIndexBuffer(*indexBuffer);
    Graphics::drawIndexedVertices();
}





