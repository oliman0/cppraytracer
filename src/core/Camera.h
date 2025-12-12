#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "Global.h"

struct CameraData
{
	vec4 cameraPosition;
	vec4 cameraForward;
	vec4 cameraRight;
	vec4 cameraUp;
	vec4 viewParams; // x : projection plane width, y : projection plane height, z : near plane, w : padding
};

class Camera
{
public:
	Camera(float fov, float nearClipPlane);
	~Camera() = default;

	CameraData GetCameraData() const
	{
		return CameraData{ 
			vec4(m_position, 1.0f),
			vec4(m_forward, 0.0f),
			vec4(m_right, 0.0f),
			vec4(m_up, 0.0f),
			vec4(m_viewParams, 0.0f)};
	}

	void SetPosition(const vec3& position) { m_position = position; }
	void Move(const vec3& delta) { m_position += delta; }

	void SetPitchYaw(float pitch, float yaw);
	void AdjustPitchYaw(float deltaPitch, float deltaYaw);
	void LookAt(const vec3& target);

	vec3 GetPosition() const { return m_position; }
	vec3 GetForward() const { return m_forward; }
	vec3 GetRight() const { return m_right; }
	vec3 GetUp() const { return m_up; }

private:
	void UpdateCameraVectors();

	float m_fov;
	float m_aspectRatio;
	float m_nearClipPlane;

	float m_pitch;
	float m_yaw;

	vec3 m_position;
	vec3 m_forward;
	vec3 m_right;
	vec3 m_up;

	vec3 m_viewParams;
};

#endif