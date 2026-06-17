#pragma once

#include "Render/Pipeline/Image.h"
#include "Resources/Model.h";

#include <string>

Render::Image* LoadBmp(const char* filename);
Render::Image* LoadTga(const char* filename);
Render::Image* LoadPng(const char* filename);

inline void LoadObj(const std::string& filename, Scene& scene, float scale = 1.0f)
{
    ObjLoader loader(scene, scale);
    loader.load(filename);
}