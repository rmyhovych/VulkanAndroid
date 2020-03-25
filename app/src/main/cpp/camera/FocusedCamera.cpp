
#include "FocusedCamera.h"
#include <math.h>

FocusedCamera::FocusedCamera(
	float width,
	float height,
	float fovy,
	glm::vec3 center,
	float theta,
	float alpha,
	float radius
) :
	m_radius(radius),
	m_theta(theta),
	m_alpha(alpha),
	m_center(center),
	m_fovy(fovy),

	Camera(width, height, fovy, getEye(center, theta, alpha, radius), center)
{
}

void FocusedCamera::setCenter(const glm::vec3& center)
{
	m_center = center;
	setCoordinates(getEye(center, m_theta, m_alpha, m_radius), center);
}

void FocusedCamera::rotate(float x, float y)
{
	m_theta -= y;
	m_alpha += x;

	setCenter(m_center);
}

const glm::vec3 FocusedCamera::getEye(const glm::vec3& center, float theta, float alpha, float radius)
{
	glm::vec3 distance = glm::vec3(radius * sinf(theta) * cosf(alpha), radius * cosf(theta), radius * sinf(theta) * sinf(alpha));
	return center + distance;
}

void FocusedCamera::setSize(float width, float height)
{
	m_projection = glm::perspective(m_fovy, width / height, 0.01f, 1000.0f);
}
