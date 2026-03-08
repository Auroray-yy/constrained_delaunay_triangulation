#include "Camera.h"

Camera::Camera(vec3 position, float pitch, float yaw, vec3 worldup) {
	Position = position;
	WorldUp = worldup;
	Pitch = pitch;
	Yaw = yaw;
	Forward.x = cos(Pitch) * sin(Yaw);
	Forward.y = sin(Pitch);
	Forward.z = cos(Pitch) * cos(Yaw);
	Right = normalize(cross(Forward, WorldUp));
	Up = normalize(cross(Right, Forward));
}

mat4 Camera::GetViewMatrix() {
	return lookAt(Position, Position + Forward, WorldUp);
}

void Camera::UpdateCameraPos_F(float sense) {
	Position += Forward * sense;
}
void Camera::UpdateCameraPos_R(float sense) {
	Position += Right * sense;
}
void Camera::UpdateCameraPos_U(float sense) {
	Position += Up * sense;
}

void Camera::ProcessMouseMovement(double deltaX, double deltaY) {
	Pitch += -deltaY * senseY;
	Yaw += -deltaX * senseX;
	if (Pitch > 1.56f)
		Pitch = 1.56f;
	if (Pitch < -1.56f)
		Pitch = -1.56f;

	UpdateCameraVector();
}

void Camera::UpdateCameraVector() {
	Forward.x = cos(Pitch) * sin(Yaw);
	Forward.y = sin(Pitch);
	Forward.z = cos(Pitch) * cos(Yaw);
	Right = normalize(cross(Forward, WorldUp));
	Up = normalize(cross(Right, Forward));
}