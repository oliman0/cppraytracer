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
struct Sphere 
{
    float4 center_radius;
    Material material;
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

StructuredBuffer<Sphere> uSpheres : register(t0, space0);
cbuffer uSceneData : register(b1, space2)
{
    uint numSpheres;
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

Intersection RaySphere(Ray ray, Sphere sphere)
{
    Intersection res = (Intersection) 0;
    
    // Translate ray relative to sphere center
    float3 oc = ray.origin - sphere.center_radius.xyz;
    
    // Quadratic coefficients
    float a = dot(ray.dir, ray.dir);
    float b = 2 * dot(oc, ray.dir);
    float c = dot(oc, oc) - sphere.center_radius.w * sphere.center_radius.w;
    float discriminant = b * b - 4 * a * c;
    
    // At least one intersection
    if (discriminant >= 0)
    {
        // To find the nearest intersection point
        // solve for t using quadratic formula
        res.dst = (-b - sqrt(discriminant)) / (2.0 * a);
        
        // Discard intersections behind the camera
        if (res.dst >= 0.0)
        {
            res.hit = true;
            
            res.hitPoint = ray.origin + res.dst * ray.dir;
            res.normal = normalize(res.hitPoint - sphere.center_radius.xyz);
            
            res.material = sphere.material;
        }
    }
    
    return res;
}

Intersection CalculateIntersection(Ray ray)
{
    Intersection closestIntersection = (Intersection) 0;
    closestIntersection.dst = 1.#INF;

    for (uint i = 0; i < numSpheres; i++)
    {
        Sphere sphere = uSpheres[i];
        Intersection intersection = RaySphere(ray, sphere);
        
        if (intersection.hit && intersection.dst < closestIntersection.dst)
        {
            closestIntersection = intersection;
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
    float2 ndc = float2(DTid.xy) / float2(width, height);
    
    // Convert to view space
    // Transform to [-0.5, 0.5] range and scale by view params (size of the projection plane)
    float3 viewPointLocal = float3(ndc.xy - 0.5, 1.0) * viewParams.xyz;
    // Transform to world space
    float3 viewPoint = cameraPosition.xyz + cameraRight.xyz * viewPointLocal.x + cameraUp.xyz * viewPointLocal.y + cameraForward.xyz * viewPointLocal.z;
    
    Ray ray;
    ray.origin = cameraPosition.xyz;
    ray.dir = normalize(viewPoint - ray.origin);
    
    float3 totalLight = float3(0, 0, 0);
    for (int i = 0; i < samplesPerPixel; i++)
    {
        totalLight += TraceRay(ray, rndSeed);
    }
    
    uOutputTexture[DTid.xy] = float4(totalLight / samplesPerPixel, 1.0);
}