#include "Camera.h"

class FocusedCamera : public Camera
{
public:
	FocusedCamera(
		float width = 1,
		float height = 1,
		float fovy = glm::pi<float>() / 3,
		glm::vec3 center = glm::vec3(0),
		float theta = 2,
		float alpha = glm::pi<float>() / 2.0f,
		float radius = 3
	);

	void setSize(float width, float height);
	void setCenter(const glm::vec3& center);
	void rotate(float x, float y);

private:
	
	const glm::vec3 getEye(const glm::vec3& center, float theta, float alpha, float radius);

private:
	float m_radius;
	float m_fovy;

	float m_theta;
	float m_alpha;

	glm::vec3 m_center;
};
