#pragma once
#include<string>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

using namespace std;
class Shader
{
protected:
	void checkCompileErrors(GLuint ID, string type);
public:
	Shader() = default;
	Shader(const char* vertexPath, const char* fragmentPath);
	Shader(const char* vertexSource, const char* fragmentSource,int a);

	string vertexString;
	string fragmentString;
	
	const char* vertexSource;
	const char* fragmentSource;

	GLuint ShaderProgram;

	void use();
};

class ComputeShader:public Shader {
public:
	ComputeShader() = default;
	ComputeShader(const char* computePath);
	ComputeShader(const char* computeSource, int a);
	string computeString;
	const char* computeSource;
};
