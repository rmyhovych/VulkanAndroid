#include "Camera.h"

Camera::Camera(
	float width,
	float height,
	float fovy,
	const glm::vec3& eye,
	const glm::vec3& center
	) :
	m_projection(glm::perspective(fovy, width / height, 0.01f, 1000.0f))
{
	setCoordinates(eye, center);
}

void Camera::setCoordinates(const glm::vec3& eye, const glm::vec3& center)
{
	m_view = glm::lookAt(eye, center, { 0, 1, 0 });
}
