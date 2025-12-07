#include "RaytracerScene.h"

#include "ObjLoader.h"

RTScene::RTScene() : m_sceneData(RTSceneData{ 0, 30, 10, 3 })
{
}

void RTScene::LoadObj(const std::string &fileName, const RTMaterial& material)
{
    std::vector<RTTriangle> objTriangles = LoadObjFileTriangles(fileName);

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& tri : objTriangles)
    {
        minX = min(minX, tri.a.x);
        minX = min(minX, tri.b.x);
        minX = min(minX, tri.c.x);

        minY = min(minY, tri.a.y);
        minY = min(minY, tri.b.y);
        minY = min(minY, tri.c.y);

        minZ = min(minZ, tri.a.z);
        minZ = min(minZ, tri.b.z);
        minZ = min(minZ, tri.c.z);

        maxX = max(maxX, tri.a.x);
        maxX = max(maxX, tri.b.x);
        maxX = max(maxX, tri.c.x);

        maxY = max(maxY, tri.a.y);
        maxY = max(maxY, tri.b.y);
        maxY = max(maxY, tri.c.y);

        maxZ = max(maxZ, tri.a.z);
        maxZ = max(maxZ, tri.b.z);
        maxZ = max(maxZ, tri.c.z);
    }

    const size_t offset = m_triangles.size();
    m_triangles.insert(m_triangles.end(), objTriangles.begin(), objTriangles.end());

    m_meshes.push_back( RTMesh {
        vec4(minX, minY, minZ, 0), vec4(maxX, maxY, maxZ, 0),
        material,
        static_cast<uint>(offset),
        static_cast<uint>(objTriangles.size())
    });

    m_sceneData.numModels = m_meshes.size();
}

void RTScene::UploadSceneToGPU(SDL_GPUDevice* sdlDevice, SDL_GPUBuffer* triangleBuffer, SDL_GPUBuffer* meshBuffer) const
{
    // Upload triangles to GPU buffer

    SDL_GPUTransferBufferCreateInfo triangleTransferBufferInfo{};
    triangleTransferBufferInfo.size = sizeof(RTTriangle) * m_triangles.size();
    triangleTransferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* triangleTransferBuffer = SDL_CreateGPUTransferBuffer(sdlDevice, &triangleTransferBufferInfo);
    if (triangleTransferBuffer == nullptr)
    {
        std::cerr << "Failed to create GPU transfer buffer: Triangle data" << std::endl;
    }

    auto pTriangleTransferData = static_cast<RTTriangle *>(SDL_MapGPUTransferBuffer(sdlDevice, triangleTransferBuffer, false));
    SDL_memcpy(pTriangleTransferData, m_triangles.data(), sizeof(RTTriangle) * m_triangles.size());
    SDL_UnmapGPUTransferBuffer(sdlDevice, triangleTransferBuffer);

    SDL_GPUCommandBuffer* pCommandBuffer = SDL_AcquireGPUCommandBuffer(sdlDevice);
    SDL_GPUCopyPass* triangleCopyPass = SDL_BeginGPUCopyPass(pCommandBuffer);

    SDL_GPUTransferBufferLocation triangleBufferLocation{};
    triangleBufferLocation.transfer_buffer = triangleTransferBuffer;
    triangleBufferLocation.offset = 0;

    SDL_GPUBufferRegion triangleBufferRegion{};
    triangleBufferRegion.buffer = triangleBuffer;
    triangleBufferRegion.size = sizeof(RTTriangle) * m_triangles.size();
    triangleBufferRegion.offset = 0;

    SDL_UploadToGPUBuffer(triangleCopyPass, &triangleBufferLocation, &triangleBufferRegion, true);
    SDL_ReleaseGPUTransferBuffer(sdlDevice, triangleTransferBuffer);

    SDL_EndGPUCopyPass(triangleCopyPass);

    // Upload mesh data to GPU

    SDL_GPUTransferBufferCreateInfo meshTransferBufferInfo{};
    meshTransferBufferInfo.size = sizeof(RTMesh) * m_meshes.size();
    meshTransferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* meshTransferBuffer = SDL_CreateGPUTransferBuffer(sdlDevice, &meshTransferBufferInfo);
    if (meshTransferBuffer == nullptr)
    {
        std::cerr << "Failed to create GPU transfer buffer: Mesh data" << std::endl;
    }

    auto pMeshTransferData = static_cast<RTMesh *>(SDL_MapGPUTransferBuffer(sdlDevice, meshTransferBuffer, false));
    SDL_memcpy(pMeshTransferData, m_meshes.data(), sizeof(RTMesh) * m_meshes.size());
    SDL_UnmapGPUTransferBuffer(sdlDevice, meshTransferBuffer);

    SDL_GPUCopyPass* meshCopyPass = SDL_BeginGPUCopyPass(pCommandBuffer);

    SDL_GPUTransferBufferLocation meshBufferLocation{};
    meshBufferLocation.transfer_buffer = meshTransferBuffer;
    meshBufferLocation.offset = 0;

    SDL_GPUBufferRegion meshBufferRegion{};
    meshBufferRegion.buffer = meshBuffer;
    meshBufferRegion.size = sizeof(RTMesh) * m_meshes.size();
    meshBufferRegion.offset = 0;

    SDL_UploadToGPUBuffer(meshCopyPass, &meshBufferLocation, &meshBufferRegion, true);
    SDL_ReleaseGPUTransferBuffer(sdlDevice, meshTransferBuffer);

    SDL_EndGPUCopyPass(meshCopyPass);

    if (!SDL_SubmitGPUCommandBuffer(pCommandBuffer))
    {
        std::cerr << "Failed to submit command buffer for scene upload." << std::endl;
    }
}