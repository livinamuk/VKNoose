#pragma once
#include "../Common.h"
#include "Camera.h"

class Player
{
public:
	void UpdateMovement(float deltaTime);
	void UpdateMouselook(float deltaTime);
	void UpdateCamera(float deltaTime, bool inventoryOpen);
	void EvaluateCollsions(std::vector<Vertex>& lines);
	bool IsCrouching();

public:
	glm::vec3 m_position = glm::vec3(0);
	Camera m_camera;
	bool m_moving = false;
	float m_radius = 0.15f;
	bool m_collisionFound = false;
	bool m_mouselookDisabled = false;
	bool m_movementDisabled = false;;
	bool m_interactDisabled = false;

	float _walkingSpeed = 2.25f;
	float _crouchingSpeed = _walkingSpeed * 0.75f;
	float _viewHeightCrouching = 1.15f;
	float _viewHeightStanding = 1.65f;;
	float _crouchDownSpeed = 17.5f;
	float _viewHeight = _viewHeightStanding;



public:
	//bool EvaluateCollision(glm::vec3 lineStart, glm::vec3 lineEnd);

private: 
	bool _crouching = false;
};