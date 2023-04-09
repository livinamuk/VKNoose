#include "TextBlitter.h"
#include "../IO/Input.h"
#include "../Audio/Audio.h"
#include "../Game/Scene.h"

namespace TextBlitter {

	int _xMargin = 16;
	int _yMargin = 16;
	int _xDebugMargin = 6;
	int _yDebugMargin = 288 - 20;
	int _lineHeight = 16;
	int _charSpacing = 0;
	int _spaceWidth = 6;
	std::string _charSheet = "•!\"#$%&\'••*+,-./0123456789:;<=>?_ABCDEFGHIJKLMNOPQRSTUVWXYZ\\^_`abcdefghijklmnopqrstuvwxyz";
	std::string _textToBilt = "";// "Fuck the world, is full of idiots.";
	std::string _debugTextToBilt = "";
	int _charCursorIndex = 0;
	float _textTime = 0;
	float _textSpeed = 200.0f;
	float _countdownTimer = 0;

	enum QuestionState { CLOSED, TYPING_QUESTION, SELECTED_YES, SELECTED_NO } _questionState;
	std::string _questionText = "";
	std::function<void(void)> _questionCallback;
	void* _userPtr = nullptr;

	void TextBlitter::Type(std::string text) {
		ResetBlitter();
		_textToBilt = text;
		_countdownTimer = 2;
	}

	void TextBlitter::AddDebugText(std::string text) {
		_debugTextToBilt += text + "\n";
	}

	void TextBlitter::ResetDebugText() {
		_debugTextToBilt = "";
	}

	void TextBlitter::ResetBlitter() {
		_textTime = 0;
		_textToBilt = "";
		_questionText = "";
		_questionCallback = nullptr;
		_questionState = QuestionState::CLOSED;
		_userPtr = nullptr;
		_charCursorIndex = 0;
		_countdownTimer = 0;
	}

	void TextBlitter::Update(float deltaTime) {
		_objectData.clear();
		_textTime += (_textSpeed * deltaTime);
		_charCursorIndex = (int)_textTime;

		float xcursor = _xMargin;
		float ycursor = _yMargin;
		int color = 0; // 0 for white, 1 for green

		// Decrement timer
		if (_countdownTimer > 0)
			_countdownTimer -= deltaTime; 
		// Wipe the text if timer equals zero  
		if (_countdownTimer < 0) {
			ResetBlitter();
		}

		// If a quesiton is active, shift the Y margin, and set _textToBlit to the question
		if (_questionState != QuestionState::CLOSED) {
			_textToBilt = _questionText;
			ycursor += _lineHeight;

			// If the question has finished typing then select yes by default
			if (_questionState == QuestionState::TYPING_QUESTION && _charCursorIndex >= _textToBilt.length()) {
				_questionState = QuestionState::SELECTED_YES;
			}
			// Show the YES/NO arrow
			if (_questionState == QuestionState::SELECTED_YES) {
				_textToBilt += "\n" + std::string(">YES  NO");
			}
			else if (_questionState == QuestionState::SELECTED_NO) {
				_textToBilt += "\n" + std::string(" YES >NO");
			}
			// Move the arrow left
			if (Input::KeyPressed(HELL_KEY_A) && _questionState == QuestionState::SELECTED_NO) {
				_questionState = QuestionState::SELECTED_YES;
				Audio::PlayAudio("RE_bleep.wav", 0.5f);
			}
			// Move the arrow right
			else if (Input::KeyPressed(HELL_KEY_D) && _questionState == QuestionState::SELECTED_YES) {
				_questionState = QuestionState::SELECTED_NO;
				Audio::PlayAudio("RE_bleep.wav", 0.5f);
			}
			// Confirm YES
			else if (Input::KeyPressed(HELL_KEY_E) && _questionState == QuestionState::SELECTED_YES) {
				Audio::PlayAudio("RE_type.wav", 1.0f);
				// Execute any callback
				if (_questionCallback)
					_questionCallback();
				// If user pointer points to a pickup item then pick it up
				for (int i = 0; i < Scene::GetGameObjectCount(); i++) {
					if (Scene::GetGameObjectByIndex(i) == _userPtr && Scene::GetGameObjectByIndex(i)->GetInteractType() == InteractType::PICKUP) {
						Scene::GetGameObjectByIndex(i)->PickUp();
					}
				}
				ResetBlitter();
				return;
			}
			// Confirm NO
			else if (Input::KeyPressed(HELL_KEY_E) && _questionState == QuestionState::SELECTED_NO) {
				Audio::PlayAudio("RE_type.wav", 1.0f);
				ResetBlitter();
				return;
			}
		}

		// Type blitting
		for (int i = 0; i < _textToBilt.length() && i < _charCursorIndex; i++)
		{
			char character = _textToBilt[i];
			if (_textToBilt[i] == '[' &&
				_textToBilt[(size_t)i + 1] == 'w' &&
				_textToBilt[(size_t)i + 2] == ']') {
				i += 2;
				color = 0;
				continue;
			}
			if (_textToBilt[i] == '[' &&
				_textToBilt[(size_t)i + 1] == 'g' &&
				_textToBilt[(size_t)i + 2] == ']') {
				i += 2;
				color = 1;
				continue;
			}
			if (character == ' ') {
				xcursor += _spaceWidth;
				continue;
			}
			if (character == '\n') {
				xcursor = _xMargin;
				ycursor -= _lineHeight;
				continue;
			}

			//std::cout << i << ": " << color << "\n";

			size_t charPos = _charSheet.find(character);

			float texWidth = _charExtents[charPos].width;
			float texWHeight = _charExtents[charPos].height;
			float renderTargetWidth = 512;
			float renderTargetHeight = 288;

			float width = (1.0f / renderTargetWidth) * texWidth;
			float height = (1.0f / renderTargetHeight) * texWHeight;
			float cursor_X = ((xcursor + (texWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
			float cursor_Y = ((ycursor + (texWHeight / 2.0f)) / renderTargetHeight) * 2 - 1;

			Transform transform;
			transform.position.x = cursor_X;
			transform.position.y = cursor_Y * -1;
			transform.scale = glm::vec3(width, height * -1, 1);

			GPUObjectData data;
			data.modelMatrix = transform.to_mat4();
			data.index_basecolor = charPos;
			data.index_normals = color; // color stored in normals
			_objectData.push_back(data);

			xcursor += texWidth + _charSpacing;
		}

		// Debug text
		color = 0;
		xcursor = _xDebugMargin;
		ycursor = _yDebugMargin;
		for (int i = 0; i < _debugTextToBilt.length(); i++)
		{
			char character = _debugTextToBilt[i];
			if (_debugTextToBilt[i] == '[' &&
				_debugTextToBilt[(size_t)i + 1] == 'w' &&
				_debugTextToBilt[(size_t)i + 2] == ']') {
				i += 2;
				color = 0;
				continue;
			}
			if (_debugTextToBilt[i] == '[' &&
				_debugTextToBilt[(size_t)i + 1] == 'g' &&
				_debugTextToBilt[(size_t)i + 2] == ']') {
				i += 2;
				color = 1;
				continue;
			}
			if (character == ' ') {
				xcursor += _spaceWidth;
				continue;
			}
			if (character == '\n') {
				xcursor = _xDebugMargin;
				ycursor -= _lineHeight;
				continue;
			}
			size_t charPos = _charSheet.find(character);

			float texWidth = _charExtents[charPos].width;
			float texWHeight = _charExtents[charPos].height;
			float renderTargetWidth = 512;
			float renderTargetHeight = 288;

			float width = (1.0f / renderTargetWidth) * texWidth;
			float height = (1.0f / renderTargetHeight) * texWHeight;
			float cursor_X = ((xcursor + (texWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
			float cursor_Y = ((ycursor + (texWHeight / 2.0f)) / renderTargetHeight) * 2 - 1;

			Transform transform;
			transform.position.x = cursor_X;
			transform.position.y = cursor_Y * -1;
			transform.scale = glm::vec3(width, height * -1, 1);

			GPUObjectData data;
			data.modelMatrix = transform.to_mat4();
			data.index_basecolor = charPos;
			data.index_normals = color; // color stored in normals
			_objectData.push_back(data);

			xcursor += texWidth + _charSpacing;
		}
	}

	void TextBlitter::AskQuestion(std::string question, std::function<void(void)> callback, void* userPtr) {
		Audio::PlayAudio("RE_type.wav", 1.0f);
		ResetBlitter();
		_questionState = QuestionState::TYPING_QUESTION;	
		_questionText = question;
		_questionCallback = callback;
		if (userPtr) {
			_userPtr = userPtr;
		}
	}

	bool TextBlitter::QuestionIsOpen() {
		return (_questionState != QuestionState::CLOSED);
	}
}
