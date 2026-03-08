#pragma once
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

using namespace glm;

class Camera
{
public:
	Camera(vec3 position, float pitch, float yaw, vec3 worldup);

	vec3 Position;
	vec3 Forward;
	vec3 Right;
	vec3 Up;
	vec3 WorldUp;
	float Pitch;
	float Yaw;
	//控制鼠标灵敏度和移动
	float senseX = 0.01f;
	float senseY = 0.01f;

	mat4 GetViewMatrix();
	void UpdateCameraPos_F(float sense);
	void UpdateCameraPos_R(float sense);
	void UpdateCameraPos_U(float sense);
	void ProcessMouseMovement(double deltaX, double deltaY);
private:
	void UpdateCameraVector();
};

