#pragma once
//#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <algorithm>
#include "Common.h"
#include <sstream>
#include <iomanip> // setprecision

namespace Util {

	#define SMALL_NUMBER		(float)9.99999993922529e-9
	#define KINDA_SMALL_NUMBER	(float)0.00001

	inline float FInterpTo(float Current, float Target, float DeltaTime, float InterpSpeed)
	{
		// If no interp speed, jump to target value
		if (InterpSpeed <= 0.f)
			return Target;

		// Distance to reach
		const float Dist = Target - Current;

		// If distance is too small, just set the desired location
		if (Dist * Dist < SMALL_NUMBER)
			return Target;

		// Delta Move, Clamp so we do not over shoot.
		const float DeltaMove = Dist * std::clamp(DeltaTime * InterpSpeed, 0.0f, 1.0f);
		return Current + DeltaMove;
	}

	inline float RandomFloat(float LO, float HI)
	{
		return LO + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (HI - LO)));
	}

	/*


	// Forward declarations
	inline glm::vec3 NormalFromTriangle(glm::vec3 pos0, glm::vec3 pos1, glm::vec3 pos2);
	//inline void SetNormalsAndTangentsFromVertices(Vertex* vert0, Vertex* vert1, Vertex* vert2); 
	inline void DrawUpFacingPlane();
	inline float RandomFloat(float min, float max);

	// Implementations

	inline std::string UpperCase(std::string& input)
	{
		std::string str = input;
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}



	glm::vec3 NormalFromTriangle(glm::vec3 pos0, glm::vec3 pos1, glm::vec3 pos2)
	{
		return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
	}

	void SetNormalsAndTangentsFromVertices(Vertex* vert0, Vertex* vert1, Vertex* vert2)
	{
		// Shortcuts for UVs
		glm::vec3& v0 = vert0->Position;
		glm::vec3& v1 = vert1->Position;
		glm::vec3& v2 = vert2->Position;
		glm::vec2& uv0 = vert0->TexCoords;
		glm::vec2& uv1 = vert1->TexCoords;
		glm::vec2& uv2 = vert2->TexCoords;

		// Edges of the triangle : postion delta. UV delta
		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

		glm::vec3 normal = Util::NormalFromTriangle(vert0->Position, vert1->Position, vert2->Position);

		vert0->Normal = normal;
		vert1->Normal = normal;
		vert2->Normal = normal;

		vert0->Tangent = tangent;
		vert1->Tangent = tangent;
		vert2->Tangent = tangent;

		vert0->Bitangent = bitangent;
		vert1->Bitangent = bitangent;
		vert2->Bitangent = bitangent;
	}

	void DrawUpFacingPlane()
	{
		static unsigned int upFacingPlaneVAO = 0;

		// Setup if you haven't already
		if (upFacingPlaneVAO == 0) {
			Vertex vert0, vert1, vert2, vert3;
			vert0.Position = glm::vec3(-0.5, 0, 0.5);
			vert1.Position = glm::vec3(0.5, 0, 0.5f);
			vert2.Position = glm::vec3(0.5, 0, -0.5);
			vert3.Position = glm::vec3(-0.5, 0, -0.5);
			vert0.TexCoords = glm::vec2(0, 1);
			vert1.TexCoords = glm::vec2(1, 1);
			vert2.TexCoords = glm::vec2(1, 0);
			vert3.TexCoords = glm::vec2(0, 0);
			Util::SetNormalsAndTangentsFromVertices(&vert0, &vert1, &vert2);
			Util::SetNormalsAndTangentsFromVertices(&vert3, &vert0, &vert1);
			std::vector<Vertex> vertices;
			std::vector<unsigned int> indices;
			unsigned int i = (unsigned int)vertices.size();
			indices.push_back(i);
			indices.push_back(i + 1);
			indices.push_back(i + 2);
			indices.push_back(i + 2);
			indices.push_back(i + 3);
			indices.push_back(i);
			vertices.push_back(vert0);
			vertices.push_back(vert1);
			vertices.push_back(vert2);
			vertices.push_back(vert3);
			unsigned int VBO;
			unsigned int EBO;
			glGenVertexArrays(1, &upFacingPlaneVAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);
			glBindVertexArray(upFacingPlaneVAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
			glEnableVertexAttribArray(5);
			glVertexAttribIPointer(5, 1, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, MaterialID));
		}
		// Draw
		glBindVertexArray(upFacingPlaneVAO);
		glDrawElements(GL_TRIANGLES, 8, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	inline std::string FileNameFromPath(std::string filepath)
	{
		// Remove directory if present.
		// Do this before extension removal incase directory has a period character.
		const size_t last_slash_idx = filepath.find_last_of("\\/");
		if (std::string::npos != last_slash_idx) {
			filepath.erase(0, last_slash_idx + 1);
		}
		// Remove extension if present.
		const size_t period_idx = filepath.rfind('.');
		if (std::string::npos != period_idx)
		{
			filepath.erase(period_idx);
		}
		return filepath;
	}

	inline bool FileExists(const std::string& name) {
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
	}

	inline glm::vec3 ClosestPointOnLine(glm::vec3 point, glm::vec3 start, glm::vec3 end)
	{
		glm::vec2 p(point.x, point.z);
		glm::vec2 v(start.x, start.z);
		glm::vec2 w(end.x, end.z);

		const float l2 = ((v.x - w.x) * (v.x - w.x)) + ((v.y - w.y) * (v.y - w.y));
		if (l2 == 0.0)
			return glm::vec3(0);

		const float t = std::max(0.0f, std::min(1.0f, dot(p - v, w - v) / l2));
		const glm::vec2 projection = v + t * (w - v);

		return glm::vec3(projection.x, 0, projection.y);
	}
	
	inline float YRotationBetweenTwoPoints(glm::vec3 a, glm::vec3 b)
	{
		float delta_x = b.x - a.x;
		float delta_y = b.z - a.z;
		float theta_radians = atan2(delta_y, delta_x);
		return -theta_radians;
	}


	
	inline std::string FloatToString(float value, int precision = 2) {
		std::stringstream stream;
		stream << std::fixed << std::setprecision(precision) << value;
		return stream.str();
	}

	inline std::string Vec3ToString(glm::vec3 value, int precision = 2) {
		std::stringstream stream;
		stream << std::fixed << std::setprecision(precision) << value.x << ", " << value.y << ", " << value.z;
		return stream.str();
	}
	*/
}