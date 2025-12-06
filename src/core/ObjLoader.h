#ifndef RAYTRACER_OBJLOADER_H
#define RAYTRACER_OBJLOADER_H

#include <vector>
#include <fstream>

#include "RaytracerScene.h"

struct ObjVertex
{
    vec3 position;
    vec3 normal;
};

std::vector<ObjVertex> LoadObjFile(const std::string& filename);
std::vector<RTTriangle> LoadObjFileTriangles(const std::string& filename);

#endif //RAYTRACER_OBJLOADER_H