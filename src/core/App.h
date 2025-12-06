#ifndef APP_H
#define APP_H

#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_mouse.h>

#include "Global.h"
#include "VertexLayout.h"
#include "RaytracerScene.h"
#include "Camera.h"
#include "ObjLoader.h"

class RayTracerApp
{
public:
	RayTracerApp();
	~RayTracerApp();

	void FrameUpdate();

private:
	SDL_Window* m_pWindow;
	SDL_GPUDevice* m_pDevice;
	SDL_GPUGraphicsPipeline* m_pGraphicsPipeline;

	SDL_GPUSampler* m_pTextureSampler;

	SDL_GPUComputePipeline* m_pComputePipeline;
	SDL_GPUTexture* m_pComputeRenderTarget;
	SDL_GPUBuffer* m_pComputeTriangleBuffer;
	SDL_GPUBuffer* m_pComputeMeshBuffer;

	Uint32 m_computeSizeX;
	Uint32 m_computeSizeY;
	Uint32 m_dispatchSizeX;
	Uint32 m_dispatchSizeY;

	SDL_GPUBuffer* m_pScreenQuadVertexBuffer;
	int m_screenQuadVertexCount;

	RTScene m_scene;

	void UpdateFPSCounter();
	Uint64 m_lastCount;
	Uint64 m_countDelta;
	Uint64 m_frequency;

	Camera m_camera;
	vec2 m_lastMousePosition;

	void HandleMovement(float deltaTime);
};

#endif