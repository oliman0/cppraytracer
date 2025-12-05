#include "ObjLoader.h"

// Simple string split implementation for using in OBJ loading
static std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string> tokens;
    // Start at 0 and find first delim
    size_t offset = 0, delimPosition = s.find(delim);

    // Find and split by delim until none found
    while (delimPosition != std::string::npos)
    {
        tokens.push_back(s.substr(offset, delimPosition - offset));
        offset = delimPosition + 1;
        delimPosition = s.find(delim, offset);
    }

    // Push the last segment of string
    tokens.push_back(s.substr(offset, delimPosition - offset));
    return tokens;
}

std::vector<ObjVertex> LoadObjFile(const std::string& filename)
{
    std::vector<ObjVertex> triangleVertices;

    std::vector<vec3> vertexPositions;
    std::vector<vec3> vertexNormals;

    std::fstream file(filename, std::ios::in);
    std::string line;
    while (std::getline(file, line))
    {
        if (line.find("v ") != std::string::npos)
        {
            std::vector<std::string> vertexPositionLine = split(line, ' ');
            vertexPositions.push_back(vec3(std::stof(vertexPositionLine[1]), std::stof(vertexPositionLine[2]), std::stof(vertexPositionLine[3])));
        }
        else if (line.find("vn ") != std::string::npos)
        {
            std::vector<std::string> vertexNormalLine = split(line, ' ');
            vertexNormals.push_back(vec3(std::stof(vertexNormalLine[1]), std::stof(vertexNormalLine[2]), std::stof(vertexNormalLine[3])));
        }
        else if (line.find("f ") != std::string::npos)
        {
            std::vector<std::string> faceIndexGroups = split(line, ' ');
            // Remove "f " at index 0
            faceIndexGroups.erase(faceIndexGroups.begin());

            for (int i = 0; i < faceIndexGroups.size(); i++)
            {
                std::vector<std::string> indexGroupStr = split(faceIndexGroups[i], '/');
                int indexGroup[3] = { std::stoi(indexGroupStr[0]) - 1, std::stoi(indexGroupStr[1]) - 1, std::stoi(indexGroupStr[2]) - 1};

                if (i >= 3)
                {
                    triangleVertices.push_back(triangleVertices[triangleVertices.size() - (3 * i - 6)]);
                    triangleVertices.push_back(triangleVertices[triangleVertices.size() - 2]);
                }

                triangleVertices.push_back(ObjVertex{vertexPositions[indexGroup[0]], vertexNormals[indexGroup[1]]});
            }
        }
    }

    return triangleVertices;
}

// TODO: single interpolated normal per tri
std::vector<RTTriangle> LoadObjFileTriangles(const std::string& filename)
{
    std::vector<RTTriangle> triangles;
    const std::vector<ObjVertex> vertices = LoadObjFile(filename);

    for (int i = 0; i < vertices.size(); i+=3)
    {
        triangles.push_back(RTTriangle{
            vec4(vertices[i].position, 0), vec4(vertices[i+1].position, 0), vec4(vertices[i+2].position, 0),
            vec4(vertices[i].normal, 0), vec4(vertices[i+1].normal, 0), vec4(vertices[i+2].normal, 0) });
    }

    return triangles;
}