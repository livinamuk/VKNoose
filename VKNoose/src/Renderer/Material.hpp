#pragma once
#include "../vk_textures.h"
#include <string>

struct Material
{
	Material() {}
	std::string _name = "undefined";
	int _basecolor = 0;
	int _normal = 0;
	int _rma = 0;
};
