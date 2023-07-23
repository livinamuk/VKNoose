#pragma once
#include "../Common.h"

class Camera {

public:
	void Update(bool moving, bool crouching, float deltaTime);
	glm::mat4 GetViewMatrix();
	glm::vec3 GetRotation();
	void ResetHeadBob();


public:
	bool _disableHeadBob = false;
	Transform m_transform;
	glm::mat4 m_viewMatrix = glm::mat4(1);
	glm::mat4 m_inverseViewMatrix = glm::mat4(1);
	glm::vec3 m_viewPos = glm::vec3(0);
	glm::vec3 m_up = glm::vec3(0);
	glm::vec3 m_right = glm::vec3(0);
	glm::vec3 m_front = glm::vec3(0);
	enum class State { PLAYER_CONROL, USING_LAPTOP } _state = State::PLAYER_CONROL;
	//bool test = false;
	//float zoom = 1;
};