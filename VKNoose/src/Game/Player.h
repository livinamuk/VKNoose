#pragma once
#include "../Common.h"
#include "Camera.h"

class Player
{
public:
	void UpdateMovement(float deltaTime);
	void UpdateMouselook();
	void UpdateCamera();
	bool IsCrouching();

public:
	glm::vec3 m_position = glm::vec3(0);
	Camera m_camera;
	bool m_moving = false;
	float m_radius = 0.15f;
	bool m_collisionFound = false;
	bool m_mouselookDisabled = false;
	bool m_movementDisabled = false;;

public:
	//bool EvaluateCollision(glm::vec3 lineStart, glm::vec3 lineEnd);

private: 
	bool _crouching = false;
};