#ifndef RTDEFS_H
#define RTDEFS_H

#include <glm/glm.hpp>
using namespace glm;

struct SceneData
{
	uint numModels;
	uint maxBounceCount;
	uint samplePerPixel;
};

struct Material
{
	vec4 colour;
	vec4 emission; // xyz: emission colour, w : emission strength
};

struct Sphere 
{
	vec4 center_radius; // xyz: center, w: radius
	Material material;
};

struct Triangle
{
	vec4 a, b, c;
	vec4 normalA, normalB, normalC;
};

struct Mesh
{
	uint triangleOffset;
	uint triangleCount;
	vec4 bboxMin, bboxMax;
	Material material;
};

#endif