#include "App.h"

RayTracerApp::RayTracerApp() :
	m_computeSizeX(8), m_computeSizeY(8),
	m_dispatchSizeX((SCREEN_WIDTH + m_computeSizeX - 1) / m_computeSizeX),
	m_dispatchSizeY((SCREEN_HEIGHT + m_computeSizeY - 1) / m_computeSizeY),
	m_sceneData(0),
	m_lastCount(0), m_countDelta(0),
	m_camera(45.0f, 0.1f),
	m_lastMousePosition(0.0f)
{
	m_frequency = SDL_GetPerformanceFrequency();

	m_pWindow = SDL_CreateWindow("Ray Tracer", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	m_pDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
    if (m_pDevice == nullptr)
    {
        std::cerr << "Failed to create GPU device." << std::endl;
    }

	SDL_ClaimWindowForGPUDevice(m_pDevice, m_pWindow);

    Vertex vertices[] = {
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
        { {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
        { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } },
        { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } },
        { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }
	};

    m_screenQuadVertexCount = 6;

    SDL_GPUBufferCreateInfo bufferInfo{};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    m_pScreenQuadVertexBuffer = SDL_CreateGPUBuffer(m_pDevice, &bufferInfo);

    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.size = sizeof(vertices);
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* vertexTransferBuffer = SDL_CreateGPUTransferBuffer(m_pDevice, &transferInfo);

    auto data = static_cast<Vertex *>(SDL_MapGPUTransferBuffer(m_pDevice, vertexTransferBuffer, false));
	SDL_memcpy(data, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(m_pDevice, vertexTransferBuffer);

    SDL_GPUCommandBuffer* pCommandBuffer = SDL_AcquireGPUCommandBuffer(m_pDevice);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(pCommandBuffer);

    SDL_GPUTransferBufferLocation location{};
    location.transfer_buffer = vertexTransferBuffer;
    location.offset = 0;

    SDL_GPUBufferRegion region{};
    region.buffer = m_pScreenQuadVertexBuffer;
    region.size = sizeof(vertices);
    region.offset = 0;

    SDL_UploadToGPUBuffer(copyPass, &location, &region, true);
    SDL_ReleaseGPUTransferBuffer(m_pDevice, vertexTransferBuffer);

    SDL_EndGPUCopyPass(copyPass);
    if (!SDL_SubmitGPUCommandBuffer(pCommandBuffer))
    {
		std::cerr << "Failed to submit command buffer for vertex buffer upload." << std::endl;
    }

    SDL_GPUSamplerCreateInfo samplerInfo{};

    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.mip_lod_bias = 0.0f;
    samplerInfo.max_anisotropy = 1.0f;
    samplerInfo.compare_op = SDL_GPU_COMPAREOP_NEVER;
    samplerInfo.min_lod = 0.0f;
    samplerInfo.max_lod = FLT_MAX;
    samplerInfo.enable_anisotropy = false;
    samplerInfo.enable_compare = false;

    m_pTextureSampler = SDL_CreateGPUSampler(m_pDevice, &samplerInfo);

    size_t vertexCodeSize;
    void* pVertexCode = SDL_LoadFile("./screenShader.vert.spv", &vertexCodeSize);

    if (vertexCodeSize == 0 || pVertexCode == nullptr)
    {
        std::cerr << "Failed to load vertex shader." << std::endl;
	}

    SDL_GPUShaderCreateInfo vertexInfo{};
    vertexInfo.code = static_cast<Uint8 *>(pVertexCode);
    vertexInfo.code_size = vertexCodeSize;
    vertexInfo.entrypoint = "main";
    vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexInfo.num_samplers = 0;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_uniform_buffers = 0;

    SDL_GPUShader* pVertexShader = SDL_CreateGPUShader(m_pDevice, &vertexInfo);
    SDL_free(pVertexCode);

    size_t fragmentCodeSize;
    void* pFragmentCode = SDL_LoadFile("./screenShader.frag.spv", &fragmentCodeSize);

    if (fragmentCodeSize == 0 || pFragmentCode == nullptr)
    {
        std::cerr << "Failed to load fragment shader." << std::endl;
    }

    SDL_GPUShaderCreateInfo fragmentInfo{};
    fragmentInfo.code = static_cast<Uint8 *>(pFragmentCode);
    fragmentInfo.code_size = fragmentCodeSize;
    fragmentInfo.entrypoint = "main";
    fragmentInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragmentInfo.num_samplers = 1;
    fragmentInfo.num_storage_buffers = 0;
    fragmentInfo.num_storage_textures = 0;
    fragmentInfo.num_uniform_buffers = 0;

    SDL_GPUShader* pFragmentShader = SDL_CreateGPUShader(m_pDevice, &fragmentInfo);
    SDL_free(pFragmentCode);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};

    pipelineInfo.vertex_shader = pVertexShader;
    pipelineInfo.fragment_shader = pFragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    SDL_GPUVertexBufferDescription vertexBufferDescriptions[1];
    vertexBufferDescriptions[0].slot = 0;
    vertexBufferDescriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescriptions[0].instance_step_rate = 0;
    vertexBufferDescriptions[0].pitch = sizeof(Vertex);

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDescriptions;

    SDL_GPUVertexAttribute vertexAttributes[2];

    // position
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].location = 0; // layout (location = 0) in shader
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].offset = 0; // start from the first byte from current buffer position

    // uv
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].location = 1; // layout (location = 1) in shader
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[1].offset = sizeof(float) * 3; // 4th float from current buffer position

    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {};
    colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(m_pDevice, m_pWindow);

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

    m_pGraphicsPipeline = SDL_CreateGPUGraphicsPipeline(m_pDevice, &pipelineInfo);
    if (m_pGraphicsPipeline == nullptr)
    {
        std::cerr << "Failed to create graphics pipeline." << std::endl;
	}

    SDL_ReleaseGPUShader(m_pDevice, pVertexShader);
    SDL_ReleaseGPUShader(m_pDevice, pFragmentShader);

	SDL_GPUTextureCreateInfo textureInfo{};

	textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    textureInfo.width = SCREEN_WIDTH;
    textureInfo.height = SCREEN_HEIGHT;
	textureInfo.layer_count_or_depth = 1;
	textureInfo.num_levels = 1;
	textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

	m_pComputeRenderTarget = SDL_CreateGPUTexture(m_pDevice, &textureInfo);

    size_t computeCodeSize;
	void* pComputeCode = SDL_LoadFile("raytracing.comp.spv", &computeCodeSize);
    if (computeCodeSize == 0 || pComputeCode == nullptr)
    {
        std::cerr << "Failed to load compute shader." << std::endl;
    }

	SDL_GPUComputePipelineCreateInfo computePipelineInfo{};

    computePipelineInfo.code_size = computeCodeSize;
	computePipelineInfo.code = static_cast<Uint8 *>(pComputeCode);
	computePipelineInfo.entrypoint = "main";
	computePipelineInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    computePipelineInfo.num_samplers = 0;
    computePipelineInfo.num_readonly_storage_textures = 0;
    computePipelineInfo.num_readonly_storage_buffers = 1;
    computePipelineInfo.num_readwrite_storage_textures = 1;
    computePipelineInfo.num_readwrite_storage_buffers = 0;
    computePipelineInfo.num_uniform_buffers = 2;
    computePipelineInfo.threadcount_x = m_computeSizeX;
    computePipelineInfo.threadcount_y = m_computeSizeY;
    computePipelineInfo.threadcount_z = 1;

    m_pComputePipeline = SDL_CreateGPUComputePipeline(m_pDevice, &computePipelineInfo);
    if (m_pComputePipeline == nullptr)
    {
        std::cerr << "Failed to create compute pipeline." << std::endl;
    }

	SDL_free(pComputeCode);

	SDL_GPUBufferCreateInfo modelBufferInfo{};
	modelBufferInfo.size = sizeof(RTSphere) * MAX_MODELS;
	modelBufferInfo.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
    m_pComputeModelBuffer = SDL_CreateGPUBuffer(m_pDevice, &modelBufferInfo);

    RTSphere spheres[3];

    spheres[0].center_radius = vec4(0.0f, 0.0f, 0.0f, 5.0f);
	spheres[0].material.colour = vec4(0.0f, 0.0f, 0.0, 0.0f);
	spheres[0].material.emission = vec4(1.0f, 1.0f, 1.0f, 5.0f);

    spheres[1].center_radius = vec4(10.0f, 25.0f, 3.0f, 20.0f);
    spheres[1].material.colour = vec4(1.0f);
	spheres[1].material.emission = vec4(0.0f);

    spheres[2].center_radius = vec4(10.0f, 3.0f, 3.0f, 2.0f);
    spheres[2].material.colour = vec4(0.929f, 0.282f, 0.216f, 1.0f);
    spheres[2].material.emission = vec4(0.0f);

	SDL_GPUTransferBufferCreateInfo modelTransferInfo{};
	modelTransferInfo.size = sizeof(spheres);
	modelTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	SDL_GPUTransferBuffer* modelTransferBuffer = SDL_CreateGPUTransferBuffer(m_pDevice, &modelTransferInfo);

	auto pSphereData = static_cast<RTSphere *>(SDL_MapGPUTransferBuffer(m_pDevice, modelTransferBuffer, false));
	SDL_memcpy(pSphereData, spheres, sizeof(spheres));
	SDL_UnmapGPUTransferBuffer(m_pDevice, modelTransferBuffer);

	pCommandBuffer = SDL_AcquireGPUCommandBuffer(m_pDevice);
	SDL_GPUCopyPass* modelCopyPass = SDL_BeginGPUCopyPass(pCommandBuffer);

	SDL_GPUTransferBufferLocation modelBufferLocation{};
	modelBufferLocation.transfer_buffer = modelTransferBuffer;
	modelBufferLocation.offset = 0;

	SDL_GPUBufferRegion modelBufferRegion{};
    modelBufferRegion.buffer = m_pComputeModelBuffer;
	modelBufferRegion.size = sizeof(spheres);
	modelBufferRegion.offset = 0;

	m_sceneData.numModels = sizeof(spheres) / sizeof(RTSphere);
	m_sceneData.maxBounceCount = 30;
	m_sceneData.samplePerPixel = 50;

	SDL_UploadToGPUBuffer(modelCopyPass, &modelBufferLocation, &modelBufferRegion, true);
	SDL_ReleaseGPUTransferBuffer(m_pDevice, modelTransferBuffer);

	SDL_EndGPUCopyPass(modelCopyPass);

    if (!SDL_SubmitGPUCommandBuffer(pCommandBuffer))
    {
        std::cerr << "Failed to submit command buffer for model buffer upload." << std::endl;
	}
}

RayTracerApp::~RayTracerApp()
{
	SDL_ReleaseGPUBuffer(m_pDevice, m_pScreenQuadVertexBuffer);
    SDL_ReleaseGPUGraphicsPipeline(m_pDevice, m_pGraphicsPipeline);
	SDL_ReleaseGPUSampler(m_pDevice, m_pTextureSampler);

	SDL_ReleaseGPUComputePipeline(m_pDevice, m_pComputePipeline);
	SDL_ReleaseGPUTexture(m_pDevice, m_pComputeRenderTarget);
	SDL_ReleaseGPUBuffer(m_pDevice, m_pComputeModelBuffer);

	SDL_DestroyGPUDevice(m_pDevice);
	SDL_DestroyWindow(m_pWindow);
}

void RayTracerApp::FrameUpdate()
{
	UpdateFPSCounter();
    HandleMovement(static_cast<float>(m_countDelta) / static_cast<float>(m_frequency));

    SDL_GPUCommandBuffer* pCommandBuffer = SDL_AcquireGPUCommandBuffer(m_pDevice);

	// Upload the Camera view data and SceneData uniforms to the shader
    CameraData cameraData = m_camera.GetCameraData();
    SDL_PushGPUComputeUniformData(pCommandBuffer, 0, &cameraData, sizeof(CameraData));
    SDL_PushGPUComputeUniformData(pCommandBuffer, 1, &m_sceneData, sizeof(RTSceneData));

	// Bind the read/write texture to use as a render target
	SDL_GPUStorageTextureReadWriteBinding textureBinding{};
	textureBinding.texture = m_pComputeRenderTarget;
	textureBinding.mip_level = 0;
	textureBinding.layer = 0;
	textureBinding.cycle = false;

    SDL_GPUComputePass* pComputePass = SDL_BeginGPUComputePass(
        pCommandBuffer,
        &textureBinding,
        1,
        nullptr,
        0
    );

	// Bind the compute pipeline with our path tracing shader
	SDL_BindGPUComputePipeline(pComputePass, m_pComputePipeline);

	// Geometry Buffer is readonly so we bind in separately
    SDL_BindGPUComputeStorageBuffers(pComputePass, 0, &m_pComputeModelBuffer, 1);

	// Dispatch with precalculated size to cover screen
	SDL_DispatchGPUCompute(pComputePass, m_dispatchSizeX, m_dispatchSizeY, 1);
	SDL_EndGPUComputePass(pComputePass);

	// Display compute output
	// Render the texture to a screen quad

    SDL_GPUTexture* pSwapchainTexture;
    Uint32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(
        pCommandBuffer,
        m_pWindow,
        &pSwapchainTexture,
        &width,
        &height
    );

    if (pSwapchainTexture == nullptr)
    {
        // Must always submit command buffer
        SDL_SubmitGPUCommandBuffer(pCommandBuffer);
		std::cerr << "Failed to acquire swapchain texture." << std::endl;
        return;
    }

    SDL_GPUColorTargetInfo colorTargetInfo{};
    colorTargetInfo.clear_color = { 0.2f, 0.2f, 0.2f };
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = pSwapchainTexture;

    SDL_GPURenderPass* pRenderPass = SDL_BeginGPURenderPass(pCommandBuffer, &colorTargetInfo, 1, nullptr);

    SDL_BindGPUGraphicsPipeline(pRenderPass, m_pGraphicsPipeline);

    SDL_GPUBufferBinding bufferBinding{};
    bufferBinding.buffer = m_pScreenQuadVertexBuffer;
    bufferBinding.offset = 0;
    
    SDL_BindGPUVertexBuffers(pRenderPass, 0, &bufferBinding, 1);

	SDL_GPUTextureSamplerBinding samplerBinding{};
	samplerBinding.sampler = m_pTextureSampler;
	samplerBinding.texture = m_pComputeRenderTarget;
    
    SDL_BindGPUFragmentSamplers(pRenderPass, 0, &samplerBinding, 1);
    
    SDL_DrawGPUPrimitives(pRenderPass, m_screenQuadVertexCount, 1, 0, 0);

    SDL_EndGPURenderPass(pRenderPass);
    SDL_SubmitGPUCommandBuffer(pCommandBuffer);

	LoadObjFileTriangles("../cube.obj");
}

void RayTracerApp::UpdateFPSCounter() 
{
    const Uint64 currentCount = SDL_GetPerformanceCounter();
    m_countDelta = currentCount - m_lastCount;

    if (m_countDelta > 0)
    {
        const double fps = static_cast<double>(m_frequency) / static_cast<double>(m_countDelta);
        std::cout << "FPS: " << fps << "\r" << std::endl;
    }

    m_lastCount = currentCount;
}

void RayTracerApp::HandleMovement(const float deltaTime)
{
    const bool* pKeyStates = SDL_GetKeyboardState(nullptr);

    vec3 movement(0.0f);
    constexpr float movementSpeed = 5.0f;

    if (pKeyStates[SDL_SCANCODE_W])
    {
        movement += m_camera.GetForward() * movementSpeed * deltaTime;
    }
    if (pKeyStates[SDL_SCANCODE_S])
    {
        movement -= m_camera.GetForward() * movementSpeed * deltaTime;
    }
    if (pKeyStates[SDL_SCANCODE_A])
    {
        movement -= m_camera.GetRight() * movementSpeed * deltaTime;
    }
    if (pKeyStates[SDL_SCANCODE_D])
    {
        movement += m_camera.GetRight() * movementSpeed * deltaTime;
    }

	if (pKeyStates[SDL_SCANCODE_Q])
    {
        movement.y -= movementSpeed * deltaTime;
    }
    if (pKeyStates[SDL_SCANCODE_E])
    {
        movement.y += movementSpeed * deltaTime;
	}

    m_camera.Move(movement);

    float mouseX, mouseY;
    if (const SDL_MouseButtonFlags mouseState = SDL_GetMouseState(&mouseX, &mouseY); mouseState & SDL_BUTTON_LEFT)
    {
        m_camera.AdjustPitchYaw((mouseY - m_lastMousePosition.y) * 0.1f, (mouseX - m_lastMousePosition.x) * 0.1f);
    }
    m_lastMousePosition = vec2(mouseX, mouseY);
}