#include "Player.h"

#include "../Audio/Audio.h"
#include "../IO/Input.h"
#include "../Util.h"

float _walkingSpeed = 3.0f * 0.01;
float _crouchingSpeed = 2.125f * 0.01;
float _viewHeightCrouching = 1.15f;
float _viewHeightStanding = 1.65f;;
float _crouchDownSpeed = 17.5f;
float _viewHeight = _viewHeightStanding;

void Player::UpdateMovement(float deltaTime)
{
	m_moving = false;

	//if (m_movementDisabled || GameData::inventoryOpen)
	//	return;	

	glm::vec3 displacement = glm::vec3(0);
	glm::vec3 forward = glm::normalize(glm::vec3(m_camera.m_front.x, 0, m_camera.m_front.z));

	float amt = 0.025f;
	if (Input::KeyDown(HELL_KEY_Q)) {
		_viewHeightCrouching += amt;
		_viewHeightStanding += amt;
	}
	if (Input::KeyDown(HELL_KEY_E)) {
		_viewHeightCrouching -= amt;
		_viewHeightStanding -= amt;
	}

	if (Input::KeyDown(HELL_KEY_W)) {
		displacement -= forward;
		m_moving = true;
	}	
	if (Input::KeyDown(HELL_KEY_S)) {
		displacement += forward;
		m_moving = true;
	}
	if (Input::KeyDown(HELL_KEY_A)) {
		displacement -= m_camera.m_right;
		m_moving = true;
	}
	if (Input::KeyDown(HELL_KEY_D)) {
		displacement += m_camera.m_right;
		m_moving = true;
	}

	// Are they crouching
	_crouching = Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW);
	
	// Set speeds and view height
	float target = IsCrouching() ? _viewHeightCrouching : _viewHeightStanding;
	float speed = IsCrouching() ? _crouchingSpeed : _walkingSpeed;

	//if (TextBlitter::GetState() == QuestionState::NOT_ASKING)
		_viewHeight = Util::FInterpTo(_viewHeight, target, deltaTime, _crouchDownSpeed);

	// Move player
	m_position += (displacement * glm::vec3(speed));

	static float m_footstepAudioTimer = 0;
	static float footstepAudioLoopLength = 0.5;

	if (!m_moving)
		m_footstepAudioTimer = 0;
	else
	{
		if (m_moving && m_footstepAudioTimer == 0) {
			int random_number = std::rand() % 4 + 1;
			//std::string file = "player_step_" + std::to_string(random_number) + ".wav"; 
			std::string file = "pl_dirt" + std::to_string(random_number) + ".wav"; 
			Audio::PlayAudio(file.c_str(), 0.5f);
		}
		float timerIncrement = IsCrouching() ? deltaTime * 0.75f : deltaTime;
		m_footstepAudioTimer += timerIncrement;

		if (m_footstepAudioTimer > footstepAudioLoopLength)
			m_footstepAudioTimer = 0;
	}
}

void Player::UpdateMouselook()
{
	//if (m_mouselookDisabled || GameData::inventoryOpen)
	//	return;

	float mouseSensitivity = 0.005f;
	float xoffset = Input::GetMouseOffsetY() * mouseSensitivity * Input::_sensitivity * 0.01f;
	float yoffset = Input::GetMouseOffsetX() * mouseSensitivity * Input::_sensitivity * 0.01f;
	float yLimit = 1.5f;	
	m_camera.m_transform.rotation += glm::vec3(-xoffset, -yoffset, 0.0);
	m_camera.m_transform.rotation.x = std::min(m_camera.m_transform.rotation.x, yLimit);
	m_camera.m_transform.rotation.x = std::max(m_camera.m_transform.rotation.x, -yLimit);
}

void Player::UpdateCamera()
{
	m_camera.m_transform.position = glm::vec3(m_position.x, m_position.y + _viewHeight, m_position.z);
	m_camera.Update(m_moving, IsCrouching());
}

bool Player::IsCrouching()
{
	return _crouching;
}

glm::vec3 ClosestPointOnLine(glm::vec3 point, glm::vec3 start, glm::vec3 end);
bool CircleIntersectsLine(glm::vec3 center, glm::vec3 point1, glm::vec3 point2, float radius);

/*bool Player::EvaluateCollision(glm::vec3 lineStart, glm::vec3 lineEnd)
{
	float radius = m_radius;
	glm::vec3 playerPos = m_position;
	playerPos.y = 0;

	glm::vec3 closestPointOnLine = Util::ClosestPointOnLine(playerPos, lineStart, lineEnd);

	glm::vec3 dir = glm::normalize(closestPointOnLine - playerPos);
	float distToLine = glm::length(closestPointOnLine - playerPos);
	float correctionFactor = radius - distToLine;

	if (glm::length(closestPointOnLine - playerPos) < radius) {
		m_position -= dir * correctionFactor;
		m_position.y = 0;
		m_collisionFound = true;
		return true;
	}
	return false;
}*/