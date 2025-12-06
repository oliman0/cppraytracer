struct Ray 
{
    float3 origin;
    float3 dir;
};

// std140 aligned
struct Material
{
    float4 color;
    float4 emission;
};

// std140 aligned
struct Triangle
{
    float4 a, b, c;
    float4 normalA, normalB, normalC;
};

// std140 aligned
struct Mesh
{
    float4 bboxMin, bboxMax;
    Material material;
    uint triangleOffset;
    uint triangleCount;
    uint padding0, padding1;
};

struct Intersection 
{
    bool hit;
    float dst;
    float3 hitPoint;
    float3 normal;
    
    Material material;
};

[[vk::image_format("rgba8")]]
RWTexture2D<float4> uOutputTexture : register(u0, space1);

cbuffer uCameraData : register(b0, space2)
{
    float4 cameraPosition;
    float4 cameraForward;
    float4 cameraRight;
    float4 cameraUp;
    float4 viewParams;
};

StructuredBuffer<Triangle> uTriangles : register(t0, space0);
StructuredBuffer<Mesh> uMeshes : register(t1, space0);
cbuffer uSceneData : register(b1, space2)
{
    uint numMeshes;
    uint maxBounces;
    uint samplesPerPixel;
};

float RandomValue(inout uint seed)
{
    seed = seed * 747796405 + 2891336453;
    uint result = ((seed >> ((seed >> 28) + 4)) ^ seed) * 277803737;
    result = (result >> 22) ^ result;
    return result / 4294967296.0;
}

float RandomValueNorm(inout uint seed)
{
    float theta = 2 * 3.14159265 * RandomValue(seed);
    float rho = sqrt(-2.0 * log(RandomValue(seed)));
    return rho * cos(theta);
}

float3 RandomDirection(inout uint seed)
{
    // Using normally distributed values to ensure uniform distribution on the sphere
    float x = RandomValueNorm(seed);
    float y = RandomValueNorm(seed);
    float z = RandomValueNorm(seed);
    return normalize(float3(x, y, z));
}

float3 RandomDirectionHemisphere(float3 normal, inout uint seed)
{
    float3 dir = RandomDirection(seed);
    
    // Dot product will be negative if below the hemisphere
    // multipling by its sign flips the direction to be above the hemisphere
    return dir * sign(dot(dir, normal));
}

bool RayAABB(Ray ray, float3 bboxMin, float3 bboxMax)
{
    float3 invDir = 1.0 / ray.dir;
    float3 t0s = (bboxMin - ray.origin) * invDir;
    float3 t1s = (bboxMax - ray.origin) * invDir;
    
    float3 tsmaller = min(t0s, t1s);
    float3 tbigger = max(t0s, t1s);
    
    float tmin = max(max(tsmaller.x, tsmaller.y), tsmaller.z);
    float tmax = min(min(tbigger.x, tbigger.y), tbigger.z);
    
    return tmax >= max(tmin, 0.0);
}

Intersection RayTriangle(Ray ray, Triangle tri)
{
    Intersection intersection = (Intersection)0;
    
    float3 edgeAB = tri.b.xyz - tri.a.xyz;
    float3 edgeAC = tri.c.xyz - tri.a.xyz;
    
    float3 pVec = cross(ray.dir, edgeAC);
    float det = dot(edgeAB, pVec);

    // Cull back-facing triangles
    if (det < 1e-6)
    {
        intersection.hit = false;
        return intersection;
    }
    // Ray is parallel to triangle plane
    else if (abs(det) < 1e-6)
    {
        intersection.hit = false;
        return intersection;
    }
    
    float invDet = 1 / det;

    float3 tVec = ray.origin - tri.a.xyz;
    float3 qVec = cross(tVec, edgeAB);
    
    float u = dot(tVec, pVec) * invDet;
    float v = dot(ray.dir, qVec) * invDet;
    
    if (u < 0 || u > 1 || v < 0 || (u + v) > 1)
    {
        intersection.hit = false;
        return intersection;
    }

    intersection.dst = dot(edgeAC, qVec) * invDet;
    intersection.hit = intersection.dst > 1e-6;
    intersection.hitPoint = ray.origin + ray.dir * intersection.dst;
    intersection.normal = normalize((1 - u - v) * tri.normalA.xyz + u * tri.normalB.xyz + v * tri.normalC.xyz);
    
    return intersection;
}

Intersection CalculateIntersection(Ray ray)
{
    Intersection closestIntersection = (Intersection) 0;
    closestIntersection.dst = 1.#INF;

    for (uint i = 0; i < numMeshes; i++)
    {
        Mesh meshInfo = uMeshes[i];
        
        if (!RayAABB(ray, meshInfo.bboxMin.xyz, meshInfo.bboxMax.xyz))
        {
            continue;
        }
        
        for (uint j = 0; j < meshInfo.triangleCount; j++)
        {
            Triangle tri = uTriangles[meshInfo.triangleOffset + j];
            Intersection intersection = RayTriangle(ray, tri);

            if (intersection.hit && intersection.dst < closestIntersection.dst)
            {
                closestIntersection = intersection;
                closestIntersection.material = meshInfo.material;
            }
        }
    }

    return closestIntersection;
}

float3 TraceRay(Ray ray, inout uint rndSeed)
{
    float3 resultLight = float3(0, 0, 0);
    float3 rayColor = float3(1, 1, 1);
    
    for (int i = 0; i < maxBounces; i++)
    {
        Intersection intersection = CalculateIntersection(ray);
        if (intersection.hit)
        {   
            ray.origin = intersection.hitPoint;
            ray.dir = RandomDirectionHemisphere(intersection.normal, rndSeed);
            
            resultLight += intersection.material.emission.xyz * intersection.material.emission.w * rayColor;
            rayColor *= intersection.material.color.rgb * dot(intersection.normal, ray.dir);
        }
        else
        {
            break;
        }
    }
    
    return resultLight;
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int width, height;
    uOutputTexture.GetDimensions(width, height);
    
    uint rndSeed = DTid.x + DTid.y * width;
    
    // Screen coordinates in [0, 1] range 
    float2 ndc = float2(DTid.x, DTid.y) / float2(width, height);
    
    // Convert to view space
    // Transform to [-0.5, 0.5] range and scale by view params (size of the projection plane)
    float3 viewPointLocal = float3(ndc.xy - 0.5, 1.0) * viewParams.xyz;
    // Transform to world space
    float3 viewPoint = cameraPosition.xyz + cameraRight.xyz * viewPointLocal.x + cameraUp.xyz * -viewPointLocal.y + cameraForward.xyz * viewPointLocal.z;
    
    Ray ray;
    ray.origin = cameraPosition.xyz;
    ray.dir = normalize(viewPoint - ray.origin);
    
    float3 totalLight = float3(0, 0, 0);
    for (int i = 0; i < samplesPerPixel; i++)
    {
        totalLight += TraceRay(ray, rndSeed);
    }

    uOutputTexture[DTid.xy] = float4(totalLight / samplesPerPixel, 1);
    
    if (samplesPerPixel == 0)
    {    
        Intersection intersection = CalculateIntersection(ray);
        if (intersection.hit)
        {
            uOutputTexture[DTid.xy] = float4(intersection.normal * 0.5 + 0.5, 1);
        }
        else
        {
            uOutputTexture[DTid.xy] = float4(0, 0, 0, 1);
        }
    }
}