#ifndef RTSCENE_H
#define RTSCENE_H

#include <string>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <glm/glm.hpp>
using namespace glm;

struct RTSceneData
{
	uint numModels;
	uint maxBounceCount;
	uint samplePerPixel;
};

struct RTMaterial
{
	vec4 colour;
	vec4 emission; // xyz: emission colour, w : emission strength
};

struct RTTriangle
{
	vec4 a, b, c;
	vec4 normalA, normalB, normalC;
};

struct RTMesh
{
	vec4 bboxMin, bboxMax;
	RTMaterial material;
	uint triangleOffset;
	uint triangleCount;
	uint padding0, padding1;
};

class RTScene
{
public:
	RTScene();
	~RTScene() = default;

	void LoadObj(const std::string& fileName, const RTMaterial& material);

	void UploadSceneToGPU(SDL_GPUDevice* sdlDevice, SDL_GPUBuffer* triangleBuffer, SDL_GPUBuffer* meshBuffer) const;

	RTSceneData* SceneData() { return &m_sceneData; }

private:
	std::vector<RTMesh> m_meshes;
	std::vector<RTTriangle> m_triangles;

	RTSceneData m_sceneData;
};

#endif