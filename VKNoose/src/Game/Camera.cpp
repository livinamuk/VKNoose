#include "Camera.h"
#include <math.h>

void Camera::Update(bool moving, bool crouching, float deltaTime)
{
	static float moveBobTime = 0;
	static float breatheBobTime = 0;
	
	if (_disableHeadBob) {
		moveBobTime = 0;
		breatheBobTime = 0;
	}

	float amplitude = 0.00505f;
	float frequency = 10.0f;
	float crouchingBobSpeeding = 0.1f;
	float walkingBobSpeeding = 1.0f;

	//float time = 10.1f;
	static Transform walkBob;
	static Transform breatheBob;

	if (moving) {
		moveBobTime += deltaTime;
		if (crouching)
			frequency *= 0.75;

		walkBob.position.x = cos(moveBobTime * frequency) * amplitude * 1;
		walkBob.position.y = sin(moveBobTime * frequency) * amplitude * 4;
	} else
	{
		breatheBobTime += deltaTime;
		float breatheAmplitude = 0.000515f;
		float breatheFrequency = 3;
		breatheBob.position.x = cos(breatheBobTime * breatheFrequency) * breatheAmplitude * 1;
		breatheBob.position.y = sin(breatheBobTime * breatheFrequency) * breatheAmplitude * 2;
	}
	//std::cout << moveBobTime << "   " << walkBob.position.y << "\n";

	m_viewMatrix = glm::inverse(walkBob.to_mat4() * breatheBob.to_mat4() * m_transform.to_mat4());
	m_inverseViewMatrix = glm::inverse(m_viewMatrix);
	m_right = glm::vec3(m_inverseViewMatrix[0]);
	m_up = glm::vec3(m_inverseViewMatrix[1]);
	m_front = glm::vec3(m_inverseViewMatrix[2]);
	m_viewPos = m_inverseViewMatrix[3];
}

glm::vec3 Camera::GetRotation()
{
	return m_transform.rotation;
}

glm::mat4 Camera::GetViewMatrix()
{
	return m_viewMatrix;
}