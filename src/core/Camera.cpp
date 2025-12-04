#include "Camera.h"

Camera::Camera(float fov, float nearClipPlane) :
	m_position(0.0f), m_forward(0.0f), m_right(0.0f), m_up(0.0f), m_viewParams(0.0f),
	m_fov(radians(fov)),
	m_nearClipPlane(nearClipPlane),
	m_aspectRatio((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT),
	m_pitch(0.0f),
	m_yaw(-90.0f)
{
	// Calculate dimensions of the projection plane at the near clip plane
	float projectionPlaneY = 2.0f * tan(m_fov * 0.5f) * m_nearClipPlane;
	float projectionPlaneX = projectionPlaneY * m_aspectRatio;

	// Store view parameters to send to the shader
	m_viewParams = vec3(projectionPlaneX, projectionPlaneY, m_nearClipPlane);

	// Initialize camera vectors
	UpdateCameraVectors();
}

void Camera::SetPitchYaw(const float pitch, const float yaw)
{
	m_pitch = pitch;
	m_yaw = yaw;

	m_pitch = clamp(m_pitch, -89.0f, 89.0f);

	UpdateCameraVectors();
}

void Camera::AdjustPitchYaw(const float deltaPitch, const float deltaYaw)
{
	m_pitch += deltaPitch;
	m_yaw += deltaYaw;

	m_pitch = clamp(m_pitch, -89.0f, 89.0f);

	UpdateCameraVectors();
}

void Camera::LookAt(const vec3& target)
{
	// Set forwards vector to point at target
	m_forward = normalize(target - m_position);

	// Recalculate right and up vectors
	m_right = normalize(cross(m_forward, vec3(0.0f, 1.0f, 0.0f)));
	m_up = normalize(cross(m_right, m_forward));

	// Update pitch and yaw based on new forward vector
	m_pitch = degrees(asin(m_forward.y));
	m_yaw = degrees(atan2(m_forward.z, m_forward.x));
}

void Camera::UpdateCameraVectors()
{
	// Calculate the new forward vector
	vec3 forward;
	forward.x = cos(radians(m_yaw)) * cos(radians(m_pitch));
	forward.y = sin(radians(m_pitch));
	forward.z = sin(radians(m_yaw)) * cos(radians(m_pitch));

	m_forward = normalize(forward);

	// Recalculate right and up vectors
	m_right = normalize(cross(m_forward, vec3(0.0f, 1.0f, 0.0f)));
	m_up = normalize(cross(m_right, m_forward));
}