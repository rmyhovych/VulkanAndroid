#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera(
		float width, 
		float height, 
		float fovy = glm::pi<float>() / 3, 
		const glm::vec3& eye = { 1, 1, 1 }, 
		const glm::vec3& center = { 0, 0, 0 }
	);

	glm::mat4& getProjection()
	{
		return m_projection;
	}

	glm::mat4& getView()
	{
		return m_view;
	}

	void setCoordinates(const glm::vec3& eye, const glm::vec3& center);

protected:
	glm::mat4 m_projection;
	glm::mat4 m_view;
};
