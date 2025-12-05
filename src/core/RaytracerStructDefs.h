#ifndef RTDEFS_H
#define RTDEFS_H

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

struct RTSphere
{
	vec4 center_radius; // xyz: center, w: radius
	RTMaterial material;
};

struct RTTriangle
{
	vec4 a, b, c;
	vec4 normalA, normalB, normalC;
};

struct RTMesh
{
	uint triangleOffset;
	uint triangleCount;
	vec4 bboxMin, bboxMax;
	RTMaterial material;
};

#endif