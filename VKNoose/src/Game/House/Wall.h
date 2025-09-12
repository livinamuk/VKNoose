#pragma once
#include "API/Vulkan/vk_mesh.h"
#include "../../Common.h"
#include "../../Util.h"
#include "../../Renderer/Material.hpp"

struct Wall {
	// methods
	Wall() {}
	Wall(glm::vec3 begin, glm::vec3 end, std::string materialName);
	// fields
	int _meshIndex = 0;
	Material* _material = nullptr;
	glm::vec3 _begin;
	glm::vec3 _end;
};