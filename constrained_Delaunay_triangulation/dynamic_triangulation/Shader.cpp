#include "Shader.h"
#include<iostream>
#include<fstream>
#include<sstream>

using namespace std;

void Shader::checkCompileErrors(GLuint ID, string type)
{
	int success;
	char infolog[512];
	
	if (type != "PROGRAM") {
		glGetShaderiv(ID, GL_COMPILE_STATUS,&success);
		if (!success) {
			glGetShaderInfoLog(ID, 512, nullptr, infolog);
			cout << "shader compile error:" << infolog << endl;
		}
	}
	else {
		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(ID, 512, nullptr, infolog);
			cout << "program compile error:" << infolog << endl;
		}
	}
}

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
	ifstream vertexFile;
	ifstream fragmentFile;

	stringstream vertexSStream;
	stringstream fragmentSStream;

	vertexFile.open(vertexPath);
	fragmentFile.open(fragmentPath);

	vertexFile.exceptions(ifstream::failbit || ifstream::badbit);
	fragmentFile.exceptions(ifstream::failbit || ifstream::badbit);

	try {
		//设置顶点和片段着色器
		if (!vertexFile.is_open() || !fragmentFile.is_open())
			throw exception("Open File Error");
		vertexSStream << vertexFile.rdbuf();
		fragmentSStream << fragmentFile.rdbuf();

		vertexString = vertexSStream.str();
		fragmentString = fragmentSStream.str();

		vertexSource = vertexString.c_str();
		fragmentSource = fragmentString.c_str();

		//将顶点和片段着色器于程序相连
		GLuint vertexShader, fragmentShader;

		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexSource, nullptr);
		glCompileShader(vertexShader);
		checkCompileErrors(vertexShader, "VERTEX");

		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
		glCompileShader(fragmentShader);
		checkCompileErrors(fragmentShader, "FRAGMENT");

		ShaderProgram = glCreateProgram();
		glAttachShader(ShaderProgram, vertexShader);
		glAttachShader(ShaderProgram, fragmentShader);

		glLinkProgram(ShaderProgram);
		checkCompileErrors(ShaderProgram, "PROGRAM");

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
	}
	catch (const std::exception& ex)
	{
		printf(ex.what());
	}
}

Shader::Shader(const char* vertexSource, const char* fragmentSource, int a)
{
	//将顶点和片段着色器于程序相连
	GLuint vertexShader, fragmentShader;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, nullptr);
	glCompileShader(vertexShader);
	checkCompileErrors(vertexShader, "VERTEX");

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
	glCompileShader(fragmentShader);
	checkCompileErrors(fragmentShader, "FRAGMENT");

	ShaderProgram = glCreateProgram();
	glAttachShader(ShaderProgram, vertexShader);
	glAttachShader(ShaderProgram, fragmentShader);

	glLinkProgram(ShaderProgram);
	checkCompileErrors(ShaderProgram, "PROGRAM");

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

}

void Shader::use()
{
	glUseProgram(ShaderProgram);

}

ComputeShader::ComputeShader(const char* computePath)
{
	ifstream computeFile;
	stringstream computeSStream;
	computeFile.open(computePath);
	computeFile.exceptions(ifstream::failbit || ifstream::badbit);
	try {
		if (!computeFile.is_open())
			throw exception("Open Compute File Error");
		computeSStream << computeFile.rdbuf();
		computeString = computeSStream.str();
		computeSource = computeString.c_str();

		GLuint computeShader;
		computeShader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(computeShader, 1, &computeSource, nullptr);
		glCompileShader(computeShader);
		checkCompileErrors(computeShader, "COMPUTE");
		ShaderProgram = glCreateProgram();
		glAttachShader(ShaderProgram, computeShader);
		glLinkProgram(ShaderProgram);
		checkCompileErrors(ShaderProgram, "PROGRAM");
		glDeleteShader(computeShader);
	}
	catch (const std::exception& ex)
	{
		printf(ex.what());
	}
}

ComputeShader::ComputeShader(const char* computeSource, int a)
{
	GLuint computeShader;
	computeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(computeShader, 1, &computeSource, nullptr);
	glCompileShader(computeShader);
	checkCompileErrors(computeShader, "COMPUTE");
	ShaderProgram = glCreateProgram();
	glAttachShader(ShaderProgram, computeShader);
	glLinkProgram(ShaderProgram);
	checkCompileErrors(ShaderProgram, "PROGRAM");
	glDeleteShader(computeShader);
}
