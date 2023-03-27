#include "TextBlitter.h"

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
	float _textSpeed = 124.75f;


	void TextBlitter::Type(std::string text) {
		_textTime = 0;
		_textToBilt = text;
	}

	void TextBlitter::AddDebugText(std::string text) {
		_debugTextToBilt += text + "\n";
	}

	void TextBlitter::Reset() {
		_debugTextToBilt = "";
	}

	void TextBlitter::Update(float deltaTime) {
		_objectData.clear();
		_textTime += (_textSpeed * deltaTime);
		_charCursorIndex = (int)_textTime;

		float xcursor = _xMargin;
		float ycursor = _yMargin;

		for (int i = 0; i < _textToBilt.length() && i < _charCursorIndex; i++)
		{
			char character = _textToBilt[i];
			if (_textToBilt[i] == '[' &&
				_textToBilt[(size_t)i + 2] == ']') {
				i += 2;
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
			_objectData.push_back(data);

			xcursor += texWidth + _charSpacing;
		}

		// Debug text
		xcursor = _xDebugMargin;
		ycursor = _yDebugMargin;
		for (int i = 0; i < _debugTextToBilt.length(); i++)
		{
			char character = _debugTextToBilt[i];
			if (_debugTextToBilt[i] == '[' &&
				_debugTextToBilt[(size_t)i + 2] == ']') {
				i += 2;
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
			_objectData.push_back(data);

			xcursor += texWidth + _charSpacing;
		}
	}
}
