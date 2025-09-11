#include "Camera.h"
#include <math.h>
#include "../Util.h"

#include <float.h>
#include "../Input/Input.h"

static float moveBobTime = 0;
static float breatheBobTime = 0;
static Transform walkBob;
static Transform breatheBob;

void Camera::ResetHeadBob() {
	moveBobTime = 0;
	breatheBobTime = 0;
	walkBob.position = glm::vec3(0);
	breatheBob.position = glm::vec3(0);
}
#include <fstream>
void Camera::Update(bool moving, bool crouching, float deltaTime)
{
	
	/*

	if (Input::KeyPressed(HELL_KEY_K)) {
		test = !test;
	}

	if (test) {
		int i = 0;
		std::ifstream file("Config.txt");
		std::string str;
		std::string file_contents;
		while (std::getline(file, str))
		{
			std::cout << str << "\n";

			if (i == 0) {
				m_transform.position.x = std::stof(str);
			}
			if (i == 1) {
				m_transform.position.y = std::stof(str);
			}
			if (i == 2) {
				m_transform.position.z = std::stof(str);
			}
			if (i == 3) {
				m_transform.rotation.x = std::stof(str);
			}
			if (i == 4) {
				m_transform.rotation.y = std::stof(str);
			}
			if (i == 5) {
				m_transform.rotation.z = std::stof(str);
			}
			if (i == 6) {
				zoom = std::stof(str);
			}
			i++;(
		}
	}
		*/
	
	if (_state == State::USING_LAPTOP) {

		//static glm::vec3 targetPosition = { -2.28, 1.09, 1.14 };
		//static glm::vec3 targetRotation = { -0.55, 1.73, 0.00 };
		static glm::vec3 targetPosition = { -2.04, 1.214845, 1.106 };
		static glm::vec3 targetRotation = { -0.5059, 1.72159265359, 0.00 };

		float lerpSpeed = 20;
		m_transform.rotation.x = Util::FInterpTo(m_transform.rotation.x, targetRotation.x, deltaTime, lerpSpeed);
		m_transform.rotation.y = Util::FInterpTo(m_transform.rotation.y, targetRotation.y, deltaTime, lerpSpeed);
		m_transform.rotation.z = Util::FInterpTo(m_transform.rotation.z, targetRotation.z, deltaTime, lerpSpeed);
		m_transform.position.x = Util::FInterpTo(m_transform.position.x, targetPosition.x, deltaTime, lerpSpeed);
		m_transform.position.y = Util::FInterpTo(m_transform.position.y, targetPosition.y, deltaTime, lerpSpeed);
		m_transform.position.z = Util::FInterpTo(m_transform.position.z, targetPosition.z, deltaTime, lerpSpeed);
		
		moveBobTime = 0;
		breatheBobTime = 0;

		//m_viewMatrix = glm::inverse(m_transform.to_mat4());

	}
	//m_transform.rotation.y = fwrap_alternate(m_transform.rotation.y, 0, NOOSE_PI * 2);

	//std::cout << "pos: " << Util::Vec3ToString(m_transform.position) << "\n";
	//std::cout << "rot: " << Util::Vec3ToString(m_transform.rotation) << "\n";
	
	if (true) {

		if (_disableHeadBob) {
			moveBobTime = 0;
			breatheBobTime = 0;
		}

		float amplitude = 0.00505f;
		float frequency = 10.0f;
		float crouchingBobSpeeding = 0.1f;
		float walkingBobSpeeding = 1.0f;

		//float time = 10.1f;

		if (moving) {
			moveBobTime += deltaTime;
			if (crouching)
				frequency *= 0.75;

			walkBob.position.x = cos(moveBobTime * frequency) * amplitude * 1;
			walkBob.position.y = sin(moveBobTime * frequency) * amplitude * 4;
		}
		else
		{
			breatheBobTime += deltaTime;
			float breatheAmplitude = 0.000515f;
			float breatheFrequency = 3;
			breatheBob.position.x = cos(breatheBobTime * breatheFrequency) * breatheAmplitude * 1;
			breatheBob.position.y = sin(breatheBobTime * breatheFrequency) * breatheAmplitude * 2;
		}
		m_viewMatrix = glm::inverse(walkBob.to_mat4() * breatheBob.to_mat4() * m_transform.to_mat4());
	}
	

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