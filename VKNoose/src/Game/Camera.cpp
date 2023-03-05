#include "Camera.h"
#include <math.h>

void Camera::Update(bool moving, bool crouching)
{
	static float totalTime;

	if (crouching)
		totalTime += 0.01f * 0.6f;
	else
		totalTime += 0.0075f;

	float amplitude = 0.00505f;
	float frequency = 25.0f;

	//float time = 10.1f;
	static Transform headBob;
	static Transform breatheBob;

	if (moving) {
		headBob.position.x = cos(totalTime * frequency) * amplitude * 1;
		headBob.position.y = sin(totalTime * frequency) * amplitude * 4;
	} else
	{
		static float breatheTime;
		totalTime += 0.0075f;
		float breatheAmplitude = 0.000515f;
		float breatheFrequency = 8;
		breatheBob.position.x = cos(totalTime * breatheFrequency) * breatheAmplitude * 1;
		breatheBob.position.y = sin(totalTime * breatheFrequency) * breatheAmplitude * 2;
	}

	m_viewMatrix = glm::inverse(headBob.to_mat4() * breatheBob.to_mat4() * m_transform.to_mat4());
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