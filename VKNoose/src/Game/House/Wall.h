#pragma once
#include "../../Common.h"
#include "../../Util.h"
#include "../../vk_mesh.h"
#include "../../Renderer/Material.hpp"

struct Wall {
	// methods
	Wall() {}
	Wall(glm::vec3 begin, glm::vec3 end, float height, std::string materialName);
	// fields
	int _meshIndex = 0;
	Material* _material = nullptr;
	glm::vec3 _begin;
	glm::vec3 _end;
	float _height;
};