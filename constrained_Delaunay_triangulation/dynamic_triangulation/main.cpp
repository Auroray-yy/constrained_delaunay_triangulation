#define WIDTH 1024
#define HEIGHT 1024
#define WindowWidth 1600
#define WindowHeight 900
int width_2d = 0, height_2d = 0;
#include <set>
#include <array>
#include <string>
#include<iostream>
#include <vector>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<time.h>
#include<algorithm>
#include<math.h>
#include<Windows.h>
#include<fstream>

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include"Shader.h"
#include"Camera.h"

using namespace std;


Camera camera(vec3(500, 500, 2000), 0, radians(180.0f), vec3(0, 1.0f, 0));
float speed = 1;
bool isshow = false;

void processInput(GLFWwindow* window) {
	// 增加速度
	if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS ||
		glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
	{
		speed += 0.5f;
	}

	// 减少速度
	if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS ||
		glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
	{
		speed -= 0.5f;

		if (speed < 0.1f)
			speed = 0.1f;
	}
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera.UpdateCameraPos_F(speed);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera.UpdateCameraPos_F(-speed);
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		camera.UpdateCameraPos_R(speed);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		camera.UpdateCameraPos_R(-speed);
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		isshow = true;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) {
		isshow = false;
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		camera.UpdateCameraPos_U(speed);
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		camera.UpdateCameraPos_U(-speed);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

bool firstMouse = true;
double lastX, lastY;
void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
	if (firstMouse == true) {
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}
	double deltaX, deltaY;
	deltaX = xPos - lastX;
	deltaY = yPos - lastY;
	lastX = xPos;
	lastY = yPos;
	camera.ProcessMouseMovement(deltaX, deltaY);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

//生成Voronoi图
GLuint construct_band_voronoi(vector<glm::vec4> points, GLuint& position_buffer,GLuint& finally_buffer,double max_y) {
	if (points.size() == 0) {
		return 0;
	}
	LARGE_INTEGER nFreq;
	LARGE_INTEGER nBeginTime;
	LARGE_INTEGER nEndTime;
	double time = 0;
	double counts = 0;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&nBeginTime);//开始计时

	GLuint finally_tbo;
	glGenTextures(1, &finally_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, finally_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, finally_buffer);

	GLuint position_tbo;
	glGenTextures(1, &position_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, position_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, position_buffer);

	QueryPerformanceCounter(&nEndTime);//停止计时  
	time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "存放数据运行时间：" << time * 1000 << "ms" << endl;


	QueryPerformanceCounter(&nBeginTime);//开始计时 
	//一步生成一维Voronoi图
	ComputeShader tphase1("tphase1.comp");
	tphase1.use();

	glBindImageTexture(0, finally_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);//绑定一个纹理到计算着色器的图像单元
	glBindImageTexture(1, position_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	
	glUniform1i(glGetUniformLocation(tphase1.ShaderProgram, "width"), width_2d);//设置Uniform变量
	glDispatchCompute(width_2d, height_2d, 1);//为1，表示只在二维平面上处理
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	QueryPerformanceCounter(&nEndTime);//停止计时  
	time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "一维Voronoi图运行时间：" << time * 1000 << "ms" << endl;

	//第二步，竖向选择候选点
	QueryPerformanceCounter(&nBeginTime);//开始计时 
	GLuint middle_buffer;	//此缓冲区用于存储栈中的数据
	glGenBuffers(1, &middle_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, middle_buffer);
	glBufferData(GL_ARRAY_BUFFER, width_2d * height_2d * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);

	GLuint middle_tbo;
	glGenTextures(1, &middle_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, middle_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, middle_buffer);

	//此缓冲用来存储middle_buffer每一列中间栈的大小
	GLuint stack_buffer;
	glGenBuffers(1, &stack_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, stack_buffer);
	glBufferData(GL_ARRAY_BUFFER, width_2d * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);

	GLuint stack_tbo;
	glGenTextures(1, &stack_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, stack_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, stack_buffer);

	ComputeShader phase2a("phase2a.comp");
	//ComputeShader phase2a(phase2aSource,1);
	phase2a.use();

	glBindImageTexture(0, finally_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, middle_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(2, position_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(3, stack_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	int m2 = 1;		//m2代表分了多少个水平带
	glUniform1i(glGetUniformLocation(phase2a.ShaderProgram, "height"), height_2d);
	glUniform1i(glGetUniformLocation(phase2a.ShaderProgram, "width"), width_2d);
	glDispatchCompute(width_2d, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	QueryPerformanceCounter(&nEndTime);//停止计时  
	time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "二维Voronoi前置运行时间：" << time * 1000 << "ms" << endl;

	//第三步，填色
	QueryPerformanceCounter(&nBeginTime);//开始计时 
	ComputeShader phase3a("phase3a.comp");

	phase3a.use();

	glBindImageTexture(0, finally_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, position_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(2, middle_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(3, stack_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	
	cout << "max_y=" << max_y << endl;
	
	glUniform1i(glGetUniformLocation(phase3a.ShaderProgram, "width"), width_2d);
	glUniform1i(glGetUniformLocation(phase3a.ShaderProgram, "height"), height_2d);
	glUniform1d(glGetUniformLocation(phase3a.ShaderProgram, "max_y"), max_y);

	glDispatchCompute(width_2d, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glDeleteBuffers(1, &middle_buffer);
	QueryPerformanceCounter(&nEndTime);//停止计时  
	time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "填色运行时间：" << time * 1000 << "ms" << endl;
	//cout << "voronoi点已生成" << endl;

	return finally_buffer;
}


GLuint construct_DT(GLuint buffer) {
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	GLuint finally_tbo;
	glGenTextures(1, &finally_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, finally_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer);
	
	GLuint tri_buffer;
	glGenBuffers(1, &tri_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, tri_buffer);
	glBufferData(GL_ARRAY_BUFFER, (6 * width_2d *  height_2d + 6 * (width_2d + height_2d)) * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
	GLuint tri_tbo;
	glGenTextures(1, &tri_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, tri_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, tri_buffer);


	ComputeShader sendtri("sendtri1.comp");
	sendtri.use();
	glBindImageTexture(0, finally_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, tri_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glUniform1i(glGetUniformLocation(sendtri.ShaderProgram, "maxb"), width_2d);
	//总工作组数width_2d - 1*height_2d - 1*1
	glDispatchCompute(width_2d - 1, height_2d - 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	return tri_buffer;
}

void construct_edge_DT(GLuint buffer, GLuint triangle_buffer) {

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	GLuint finally_tbo;
	glGenTextures(1, &finally_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, finally_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer);

	glBindBuffer(GL_ARRAY_BUFFER, triangle_buffer);
	GLuint triangle_tbo;
	glGenTextures(1, &triangle_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, triangle_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, triangle_buffer);

	//边缓冲
	GLuint border_buffer;
	glGenBuffers(1, &border_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, border_buffer);
	glBufferData(GL_ARRAY_BUFFER, 2 * 2 * (width_2d + height_2d) * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
	GLuint border_tbo;
	glGenTextures(1, &border_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, border_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, border_buffer);
	
	//三角形缓冲
	GLuint edge_buffer;
	glGenBuffers(1, &edge_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, edge_buffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * 2 * (width_2d + height_2d) * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
	GLuint edge_tbo;
	glGenTextures(1, &edge_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, edge_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, edge_buffer);
	
	//修改边缘三角剖分
	ComputeShader edge_seed("edgeseed1.comp");
	edge_seed.use();
	glBindImageTexture(0, finally_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, edge_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(2, border_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(3, triangle_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glUniform1i(glGetUniformLocation(edge_seed.ShaderProgram, "width"), width_2d);
	glUniform1i(glGetUniformLocation(edge_seed.ShaderProgram, "height"), height_2d);
	glDispatchCompute(4, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void construct_CDT(GLuint tbuffer, GLuint line_buffer, int l_size) {
	LARGE_INTEGER nFreq;
	LARGE_INTEGER nBeginTime;
	LARGE_INTEGER nEndTime;
	double time = 0;
	double counts = 0;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&nBeginTime);//开始计时

	//影响域缓冲区
	glBindBuffer(GL_ARRAY_BUFFER, tbuffer);
	GLuint triangle_tbo;
	glGenTextures(1, &triangle_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, triangle_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, tbuffer);
	//int ce_size = lineVertices.size() / 2;
	//相交边缓冲区
	GLuint crossedge_buffer;
	glGenBuffers(1, &crossedge_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, crossedge_buffer);
	glBufferData(GL_ARRAY_BUFFER, 2 * 3 * 2 * width_2d * height_2d * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
	GLuint crossedge_tbo;
	glGenTextures(1, &crossedge_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, crossedge_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, crossedge_buffer);
	
	GLuint edge_out_buffer;
	glGenBuffers(1, &edge_out_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, edge_out_buffer);
	glBufferData(GL_ARRAY_BUFFER, 2 * 3 * 2 * width_2d * height_2d * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
	GLuint edge_out_tbo;
	glGenTextures(1, &edge_out_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, edge_out_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, edge_out_buffer);
	
	//约束边-三角形映射缓冲区
	GLuint edge_map_buffer;
	glGenBuffers(1, &edge_map_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, edge_map_buffer);
	int numEntries = 2 * 3 * width_2d * height_2d;
	// 每个位置初始化为 -1
	//std::vector<int> initData(numEntries, -1);
	glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(int) * numEntries, NULL, GL_DYNAMIC_DRAW);

	GLuint triangle_lock_buffer;
	glGenBuffers(1, &triangle_lock_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, triangle_lock_buffer);
	glBufferData(GL_TEXTURE_BUFFER, (2 * height_2d * width_2d + 2 * (width_2d + height_2d)) * sizeof(GLint), NULL, GL_DYNAMIC_DRAW);
	GLuint triangle_lock_tbo;
	glGenTextures(1, &triangle_lock_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, triangle_lock_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, triangle_lock_buffer);

	GLuint atomicCounterBuffer;
	glGenBuffers(1, &atomicCounterBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicCounterBuffer);
	// 分配 ce_size 个 uint（初始全部置 0）
	std::vector<GLuint> zeroData(l_size, 0u);
	glBufferData(GL_SHADER_STORAGE_BUFFER,zeroData.size() * sizeof(GLuint),zeroData.data(),GL_DYNAMIC_DRAW);
	
	GLuint atomicCounterBuffer2;
	glGenBuffers(1, &atomicCounterBuffer2);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicCounterBuffer2);
	glBufferData(GL_SHADER_STORAGE_BUFFER,zeroData.size() * sizeof(GLuint),zeroData.data(),GL_DYNAMIC_DRAW);

	GLuint count_ssbo;
	glGenBuffers(1, &count_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, count_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, l_size * sizeof(uint32_t), zeroData.data(), GL_DYNAMIC_COPY);

	GLuint offset_ssbo;
	glGenBuffers(1, &offset_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, offset_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, l_size * sizeof(uint32_t), zeroData.data(), GL_DYNAMIC_COPY);

	QueryPerformanceCounter(&nEndTime);//停止计时  
	time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "数据绑定及初始化运行时间：" << time * 1000 << "ms" << endl;

	QueryPerformanceCounter(&nBeginTime);//开始计时
	//设计动态长度的相交边缓冲区存储
	ComputeShader count_cross("count_cross.comp");
	count_cross.use();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, line_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, count_ssbo);
	glBindImageTexture(2, triangle_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glUniform1i(glGetUniformLocation(count_cross.ShaderProgram, "width"), width_2d);
	glUniform1i(glGetUniformLocation(count_cross.ShaderProgram, "height"), height_2d);
	glDispatchCompute(l_size, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	ComputeShader count_offset("count_offset.comp");
	count_offset.use();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, count_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, offset_ssbo);
	glUniform1i(glGetUniformLocation(count_offset.ShaderProgram, "lsize"), l_size);
	glDispatchCompute((l_size + 255) / 256, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	QueryPerformanceCounter(&nEndTime);//停止计时
	time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "局部统计与前缀和扫描运行时间：" << time * 1000 << "ms" << endl;

	QueryPerformanceCounter(&nBeginTime);//开始计时
	//找到相交边
	ComputeShader influenceedge_shader("find_influence_edge.comp");
	influenceedge_shader.use();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, line_buffer);  // 绑定到 binding=0
	glBindImageTexture(1, triangle_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(2, crossedge_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, edge_map_buffer);
	glBindImageTexture(4, triangle_lock_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, atomicCounterBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, offset_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, count_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, atomicCounterBuffer2);
	glUniform1i(glGetUniformLocation(influenceedge_shader.ShaderProgram, "width"), width_2d);
	glUniform1i(glGetUniformLocation(influenceedge_shader.ShaderProgram, "height"), height_2d);
	glDispatchCompute(l_size, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	
	QueryPerformanceCounter(&nEndTime);//停止计时
	time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "存储数据结构运行时间：" << time * 1000 << "ms" << endl;

	//修改影响域
	QueryPerformanceCounter(&nBeginTime);//开始计时
	ComputeShader process01_shader("process_edge.comp");
	ComputeShader init_shader("init.comp");
	int edgemapsize = 6 * height_2d * width_2d;
	ComputeShader clear_shader("clear.comp");
		//处理影响域
		process01_shader.use();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, line_buffer);  // 绑定到 binding=0
		glBindImageTexture(1, triangle_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(2, crossedge_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, edge_map_buffer);
		glBindImageTexture(4, triangle_lock_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
		glBindImageTexture(7, edge_out_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, offset_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, count_ssbo);
		glUniform1i(glGetUniformLocation(process01_shader.ShaderProgram, "width"), width_2d);
		glUniform1i(glGetUniformLocation(process01_shader.ShaderProgram, "height"), height_2d);
		glDispatchCompute(l_size, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		
		//清空缓冲区
		clear_shader.use();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, edge_map_buffer);
		glUniform1i(glGetUniformLocation(clear_shader.ShaderProgram, "clear_size"), edgemapsize);
		glDispatchCompute((edgemapsize + 63) / 64, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		init_shader.use();
		glBindImageTexture(0, triangle_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glUniform1i(glGetUniformLocation(init_shader.ShaderProgram, "size"), 6 * width_2d * height_2d + 6 * (height_2d + width_2d));
		glDispatchCompute(((6 * width_2d * height_2d + 6 * (height_2d + width_2d)) + 63) / 64, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//找影响域
		//清零
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicCounterBuffer);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, zeroData.data());
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicCounterBuffer2);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, zeroData.data());
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		influenceedge_shader.use();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, line_buffer);  // 绑定到 binding=0
		glBindImageTexture(1, triangle_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(2, crossedge_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, edge_map_buffer);
		glBindImageTexture(4, triangle_lock_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, atomicCounterBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, offset_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, count_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, atomicCounterBuffer2);
		glUniform1i(glGetUniformLocation(influenceedge_shader.ShaderProgram, "width"), width_2d);
		glUniform1i(glGetUniformLocation(influenceedge_shader.ShaderProgram, "height"), height_2d);
		glUniform1i(glGetUniformLocation(influenceedge_shader.ShaderProgram, "ce_size"), l_size);
		glDispatchCompute(l_size, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	QueryPerformanceCounter(&nEndTime);//停止计时  
	time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;  
	cout << "CDT修改影响域运行时间：" << time * 1000 << "ms" << endl;
}

void show(GLFWwindow* window,GLuint bgpoint_VAO,Shader bgpointShader,GLuint pointVAO,Shader pointShader,glm::vec4 offset,vector<glm::vec4> manyPoints,GLuint triangleVAO,Shader triangleShader,GLuint bordertriVAO) {
	while (!glfwWindowShouldClose(window)) {
		glm::mat4 projection = glm::perspective(glm::radians(30.0f), 16.0f / 9.0f, 0.1f, 5000.0f);
		glm::mat4 view = glm::mat4(1.0f);
		view = camera.GetViewMatrix();

		glm::mat4 mvp = projection * view;
		glClearColor(0.2, 0.3, 0.3, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(bgpoint_VAO);
		bgpointShader.use();
		glUniformMatrix4fv(glGetUniformLocation(bgpointShader.ShaderProgram, "mvp"), 1, GL_FALSE, value_ptr(mvp));	// 传入 MVP 矩阵
		glUniform1f(glGetUniformLocation(bgpointShader.ShaderProgram, "total"), width_2d);
		glUniform4fv(glGetUniformLocation(bgpointShader.ShaderProgram, "origin_offset"), 1, value_ptr(offset));
		glDrawArrays(GL_POINTS, 0, width_2d * height_2d);

		glBindVertexArray(pointVAO);
		pointShader.use();
		glUniformMatrix4fv(glGetUniformLocation(pointShader.ShaderProgram, "mvp"), 1, GL_FALSE, value_ptr(mvp));	// 传入 MVP 矩阵
		glPointSize(5.0f);
		glDrawArrays(GL_POINTS, 0, manyPoints.size());
		//glDrawArrays(GL_POINTS, 0, points.size());
		//glDrawArrays(GL_POINTS, 0, points1.size());
		//glDrawArrays(GL_POINTS, 0, points2.size());

		//绘制三角网
		glBindVertexArray(triangleVAO);
		triangleShader.use();
		glUniformMatrix4fv(glGetUniformLocation(triangleShader.ShaderProgram, "mvp"), 1, GL_FALSE, value_ptr(mvp));	// 传入 MVP 矩阵
		glUniform4fv(glGetUniformLocation(triangleShader.ShaderProgram, "ucolor"), 1, value_ptr(vec4(1, 1, 1, 1)));
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, width_2d * height_2d * 6);

		//绘制边缘三角形
		glBindVertexArray(bordertriVAO);
		glUniform4fv(glGetUniformLocation(triangleShader.ShaderProgram, "ucolor"), 1, value_ptr(vec4(1, 0, 0, 1)));
		glDrawArrays(GL_TRIANGLES, 0, 4 * 3 * width_2d);

		processInput(window);
		glfwSwapBuffers(window);
		glfwPollEvents();	//获取交互信息
	}
}

void get_data_from_file(vector<glm::vec4>& points, const char* path) {
	ifstream file;
	file.open(path);
	if (!file.is_open()) {
		std::cout << "Unable to open file";
		return;
	}
	double value;
	int i = 1;
	double x = 0, y = 0;
	while (file >> value) {
		if (i % 3 == 1) {
			x = value;
		}
		else if (i % 3 == 2) {
			y = value;
			points.push_back(glm::vec4(x, y, 0, 0));
		}
		++i;
	}
	file.close();
}

void output_point_to_file(vector<glm::vec4> &points, const char* path) {
	std::ofstream outFile(path); // 创建ofstream对象，并打开文件

	if (!outFile.is_open()) { // 检查文件是否成功打开
		std::cerr << "无法打开文件" << std::endl;
		return;
	}
	for (auto c : points) {
		outFile << c.x << " " << c.y << " " << 0 << endl;
	}
	outFile.close(); // 关闭文件
}

void output_data_to_file(GLuint triangleVBO, const char* path) {
	std::ofstream outFile(path); // 创建ofstream对象，并打开文件

	if (!outFile.is_open()) { // 检查文件是否成功打开
		std::cerr << "无法打开文件" << std::endl;
		return;
	}

	int index = 0;
	glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
	glm::vec4* ptr = (glm::vec4*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	//for (int i = 0; i < 6 * width_2d * height_2d + 2 * 3 * (width_2d + height_2d); ++i) {
	for (int i = 0; i < width_2d*height_2d; ++i) {
		if (ptr[i].w == -2) {
			//cout << ptr[i].x << " " << ptr[i].y << " " << ptr[i].z << " " << ptr[i].w << endl;
			//cout << i << endl;
			//outFile << ptr[i].x << " " << ptr[i].y << " " << ptr[i].z << " " << ptr[i].w << endl;
			outFile << ptr[i].z << " ";
			++index;
			if (index % 3 == 0) {
				outFile << endl;
			}
		}
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
	outFile.close(); // 关闭文件
}

bool compy(glm::vec4 a, glm::vec4 b) {
	if (a.y == b.y) {
		return a.x < b.x;
	}
	else {
		return a.y < b.y;
	}
}

bool compx(glm::vec4 a, glm::vec4 b) {
	if (a.x == b.x) {
		return a.y < b.y;
	}
	else {
		return a.x < b.x;
	}
}

// 计算2D叉积 (忽略z和w分量)
float crossProduct(const glm::vec4& a, const glm::vec4& b) {
	return a.x * b.y - a.y * b.x;
}

/*// 判断两条线段是否相交，并计算交点
bool doLinesIntersect(const glm::vec4& p1, const glm::vec4& p2, const glm::vec4& p3, const glm::vec4& p4, glm::vec4& intersection) {
	//约束线段端点相交不做处理
	if (p1.x == p3.x && p1.y == p3.y || p1.x == p4.x && p1.y == p4.y 
		|| p2.x == p3.x && p2.y == p3.y || p2.x == p4.x && p2.y == p4.y) {
		return false;
	}

	// 计算方向向量
	glm::vec4 r = p2 - p1;
	glm::vec4 s = p4 - p3;

	// 计算叉积
	float rxs = crossProduct(r, s);
	glm::vec4 qp = p3 - p1;
	float qpxr = crossProduct(qp, r);

	// 如果线段平行或共线
	if (std::abs(rxs) < 0.0001f) {
		return false;
	}

	// 计算参数t和u
	float t = crossProduct(qp, s) / rxs;
	float u = crossProduct(qp, r) / rxs;

	// 检查是否在线段范围内
	if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
		// 计算交点
		intersection = p1 + t * r;
		return true;
	}
	return false;
}*/
bool doLinesIntersect(const glm::vec4& p1, const glm::vec4& p2, const glm::vec4& p3, const glm::vec4& p4, glm::vec4& intersection) {
	//约束线段端点相交不做处理
	if ((p1.x == p3.x && p1.y == p3.y) || (p1.x == p4.x && p1.y == p4.y) ||
		(p2.x == p3.x && p2.y == p3.y) || (p2.x == p4.x && p2.y == p4.y)) {
		return false;
	}
	// 计算方向向量
	glm::vec4 r = p2 - p1;
	glm::vec4 s = p4 - p3; 
	glm::vec4 qp = p3 - p1;
	// 计算叉积
	float rxs = crossProduct(r, s);
	//float qpxr = crossProduct(qp, r);

	// 如果线段平行或共线
	if (std::abs(rxs) < 0.0001f) {
		return false;
	}
	// 计算参数t和u
	float t = crossProduct(qp, s) / rxs;
	float u = crossProduct(qp, r) / rxs;
	// 检查是否在线段范围内
	const float EPS = 1e-5f;
	if (t >= EPS && t <= 1.0f - EPS && u >= EPS && u <= 1.0f - EPS) {
		// 计算交点
		intersection = p1 + t * r;
		return true;
	}
	return false;
}


int main() {
	//初始化OpenGL,设置版本号和编译方式
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//创建一个OpenGL窗口
	GLFWwindow* window = glfwCreateWindow(WindowWidth, WindowHeight, "CS", NULL, NULL);
	if (window == nullptr) {
		printf("Open Window Failed");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);//帧缓冲大小改变的回调函数
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "Initialize GLAD Failed" << endl;
		return -1;
	}
	glEnable(GL_DEPTH_TEST);

	vector<glm::vec4> points{
		glm::vec4(5, 500, 0, 0),
		glm::vec4(10, 20, 0, 0),
		glm::vec4(50, 100, 0, 0),
	};

	vector<glm::vec4> points1 = {
		glm::vec4(0,0,0,0),
		glm::vec4(10,10,0,0),
		glm::vec4(32,46,0,0),
		glm::vec4(48,16,0,0),
		glm::vec4(56,12,0,0),
		glm::vec4(68,32,0,0),
		glm::vec4(72,16,0,0),
		glm::vec4(84,32,0,0),
		glm::vec4(150,118,0,0),
		glm::vec4(130,12,0,0),
		glm::vec4(144,16,0,0),
		glm::vec4(145,32,0,0),
		//glm::vec4(168,48,0,0),
		glm::vec4(20.1,80.5,0,0),
		glm::vec4(30,70,0,0),
		glm::vec4(60,120,0,0),
		glm::vec4(72,72,0,0),
		glm::vec4(96,84,0,0),
		glm::vec4(108,102,0,0),
		glm::vec4(132,72,0,0),
		glm::vec4(156,90,0,0),
		glm::vec4(180,100,0,0),
		//glm::vec4(12,130,0,0),
		glm::vec4(32,150,0,0),
		glm::vec4(40,144,0,0),
		glm::vec4(48,170,0,0),
		glm::vec4(70,180,0,0),
		//glm::vec4(83,130,0,0),
		glm::vec4(96,144,0,0),
		glm::vec4(100,156,0,0),
		glm::vec4(132,145,0,0),
		glm::vec4(167,133,0,0),
		glm::vec4(168,130,0,0),
		//glm::vec4(168.3,130.2,0,0),
		//glm::vec4(168.9,130.2,0,0),
		//glm::vec4(168.3,130.7,0,0),
		//glm::vec4(168.5,130.5,0,0),
		glm::vec4(178,166,0,0),
	};
	vector<glm::vec4> manyPoints;
	vector<pair<int, int>> point_loca;		//此变量记录离散空间中每个离散点在缓存空间的位置
	int pointnum = 900;

	vector<glm::vec4> points2;
	get_data_from_file(points2, "p3.txt");
	manyPoints = points2;
	cout << "manypoints[0]:" << manyPoints[0].x << " " << manyPoints[0].y << " " << manyPoints[0].z << " " << manyPoints[0].w << endl;

	//定义约束线段
	vector<glm::vec4> lineVertices; 
	get_data_from_file(lineVertices, "p6.txt"); 
		
	cout << "点数：" << manyPoints.size() << endl;
	cout << "约束线段的个数：" << lineVertices.size()/2 << endl;


	//获取点集的最小值和最大值
	double min_x = INT_MAX, min_y = INT_MAX;
	double max_x = INT_MIN, max_y = INT_MIN;
	
	for (int i = 0; i < manyPoints.size(); ++i) {
		glm::vec4 v = manyPoints[i];
		min_x = std::min(min_x, double(v.x));
		min_y = std::min(min_y, double(v.y));
		max_x = std::max(max_x, double(v.x));
		max_y = std::max(max_y, double(v.y));
	}

	//计算差值
	double delta_x = max_x - min_x;
	double delta_y = max_y - min_y;

	//获得差值的2的整数次方
	int temp_x = pow(2, ceil(log2(delta_x)));
	int temp_y = pow(2, ceil(log2(delta_y)));
	//width_2d和height_2d为离散空间的大小
	width_2d = std::max(temp_x, temp_y);
	height_2d = width_2d;

	cout << "根据差值计算出离散空间大小" << width_2d << " " << height_2d << endl;

	//offset表示点集数据位于左下角的点
	glm::vec4 offset = glm::vec4(int(min_x), int(min_y), 0, 0);
	//向四周插入点
	manyPoints.push_back(glm::vec4(int(min_x), int(min_y), 0, 0));
	manyPoints.push_back(glm::vec4(min_x, max_y, 0, 0));
	manyPoints.push_back(glm::vec4(max_x, min_y, 0, 0));
	manyPoints.push_back(glm::vec4(max_x, max_y, 0, 0));

	cout << "点数据位于左下角的点" << offset.x << " " << offset.y << endl;

	//以x为优先进行排序
	sort(manyPoints.begin(), manyPoints.end(), compx);
	//删除重复点
	auto last = unique(manyPoints.begin(), manyPoints.end());
	manyPoints.erase(last, manyPoints.end());

	//记录出现过的x值和y值(无重复，从小到大排序列)
	vector<double> xset, yset;
	for (int i = 0; i < manyPoints.size(); ++i) {
		if (i == 0) {
			xset.push_back(manyPoints[i].x);
		}
		else {
			if (manyPoints[i].x != xset.back()) {
				xset.push_back(manyPoints[i].x);
			}
		}
	}

	sort(manyPoints.begin(), manyPoints.end(), compy);
	for (int i = 0; i < manyPoints.size(); ++i) {
		if (i == 0) {
			yset.push_back(manyPoints[i].y);
		}
		else {
			if (manyPoints[i].y != yset.back()) {
				yset.push_back(manyPoints[i].y);
			}
		}
	}

	//根据记录的x值y值计算出离散空间中所有的x值和y值
	vector<double> allx, ally;
	int floatx = 0, floaty = 0;//记录有小数的个数
	for (auto c : xset) {
		double temp = c - int(c);
		//包含小数
		if (temp != 0) {
			++floatx;
			if (int(c) != int(allx.back())) {
				for (int i = allx.back() + 1; i < int(c); ++i) {
					allx.push_back(i);
				}
			}
			allx.push_back(c);
		}
		//不含小数
		else {
			if (allx.empty()) {
				allx.push_back(c);
			}
			else {
				for (int i = allx.back() + 1; i <= c; ++i) {
					allx.push_back(i);
				}
			}
		}
	}
	cout <<"allx:"<< allx.size() << endl;
	//补全x右侧范围
	for (int i = allx.back() + 1; i < width_2d + offset.x; ++i) {
		allx.push_back(i);
	}
	cout << "补全后allx:" << allx.size() << endl;

	for (auto c : yset) {
		double temp = c - int(c);
		if (temp != 0) {
			++floaty;
			if (int(c) != int(ally.back())) {
				for (int i = ally.back() + 1; i < int(c); ++i) {
					ally.push_back(i);
				}
			}
			ally.push_back(c);
		}
		else {
			if (ally.empty()) {
				ally.push_back(c);
			}
			else {
				for (int i = ally.back() + 1; i <= c; ++i) {
					ally.push_back(i);
				}
			}
		}
	}
	cout << "ally:" << ally.size() << endl;
	//补全y右侧范围
	for (int i = ally.back() + 1; i < height_2d+offset.y; ++i) {
		ally.push_back(i);
	}
	cout << "补全后ally:" << ally.size() << endl;

	//统计点集数据中出现的小数个数并更改离散空间的大小
	width_2d = allx.size();
	height_2d = ally.size();
	cout << "数据中小数的个数：" << floatx << " " << floaty << endl;
	cout << "离散空间大小：" << width_2d << " " << height_2d << endl;
	cout << allx.size() << " " << ally.size() << endl;

	//高精度计时
	LARGE_INTEGER nFreq;
	LARGE_INTEGER nBeginTime;
	LARGE_INTEGER nEndTime;
	LARGE_INTEGER nTotalStart;
	LARGE_INTEGER nTotalEnd;
	LARGE_INTEGER BEGIN, END;
	LARGE_INTEGER DTBEGIN, DTEND;
	LARGE_INTEGER CDTBEGIN, CDTEND;
	double time = 0;
	double counts = 0;
	QueryPerformanceFrequency(&nFreq);//返回计时器的频率（每秒的计数次数）

	QueryPerformanceCounter(&nTotalStart);//记录总时间的开始
	QueryPerformanceCounter(&BEGIN);

	int index = 0;
	GLuint position_buffer;		//存放坐标数据
	glGenBuffers(1, &position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glBufferData(GL_ARRAY_BUFFER, width_2d * height_2d * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
	//GPU 缓冲区的一个范围映射到 CPU 的内存，返回一个指向缓冲区的指针 positions。
	glm::vec4* positions = (glm::vec4*)glMapBufferRange(GL_ARRAY_BUFFER,
		0,
		width_2d * height_2d * sizeof(glm::vec4),
		GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (int y = 0; y < height_2d; ++y) {
		for (int x = 0; x < width_2d; ++x) {
			if (y == 0 && x == 0) {
				cout << "测试点：" << allx[x] << " ; " << ally[y] << endl;
			}
			positions[y * width_2d + x] = glm::vec4(allx[x], ally[y], 0, 0);
			if (index<manyPoints.size()&&manyPoints[index].x == allx[x] && manyPoints[index].y == ally[y]) {
				point_loca.push_back(pair<int, int>(y, x));		//记录点集在离散空间的位置
				++index;
			}
		}
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	GLuint finally_buffer;	//存放种子点数据
	glGenBuffers(1, &finally_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, finally_buffer);
	glBufferData(GL_ARRAY_BUFFER, width_2d * height_2d * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
	glm::vec4* p = (glm::vec4*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	for (int i = 0; i < manyPoints.size(); ++i) {
		glm::vec4 v = manyPoints[i];
		pair<int, int> loca = point_loca[i];	//获取点集在离散空间的位置
		p[loca.first*width_2d+loca.second]=glm::vec4(v.x, v.y, i, -2);	//将点集数据记录到离散空间相应的位置中
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	GLuint constraint_buffer;
	glGenBuffers(1, &constraint_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, constraint_buffer);  // 或 GL_ARRAY_BUFFER / GL_TEXTURE_BUFFER
	glBufferData(
		GL_SHADER_STORAGE_BUFFER, lineVertices.size() * sizeof(glm::vec4), lineVertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);    
	
	QueryPerformanceCounter(&END);
	time = (double)(END.QuadPart - BEGIN.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "数据导入GPU时间：" << time * 1000 << "ms" << endl;
	
	//构建Voronoi图
	//offset = glm::vec4(0, 0, 0, 0);
	QueryPerformanceCounter(&DTBEGIN);
	finally_buffer = construct_band_voronoi(manyPoints, position_buffer,finally_buffer,temp_y);

	//构建Delaunay三角网
	QueryPerformanceCounter(&BEGIN);//开始计时 
	GLuint triangleVBO = construct_DT(finally_buffer);
	QueryPerformanceCounter(&END);//停止计时  
	time = (double)(END.QuadPart - BEGIN.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "构网运行时间：" << time * 1000 << "ms" << endl;

	QueryPerformanceCounter(&BEGIN);//开始计时
	construct_edge_DT(finally_buffer, triangleVBO);
	QueryPerformanceCounter(&END);//停止计时
	time = (double)(END.QuadPart - BEGIN.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "补边运行时间：" << time * 1000 << "ms" << endl;

	QueryPerformanceCounter(&DTEND);
	time = (double)(DTEND.QuadPart - DTBEGIN.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s
	cout << "构建Delaunay三角剖分计算时间：" << time * 1000 << "ms" << endl;
	
	//Ours!!!!!!!!
	
	//构建约束Delaunay三角网
	QueryPerformanceCounter(&CDTBEGIN);//开始计时
	//GLuint influenttriVBO;
	if (lineVertices.size() > 0) {
		construct_CDT(triangleVBO, constraint_buffer, lineVertices.size() / 2);
	}
	QueryPerformanceCounter(&CDTEND);//停止计时
	time = (double)(CDTEND.QuadPart - CDTBEGIN.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "构建约束Delaunay三角网运行时间：" << time * 1000 << "ms" << endl;
	
	QueryPerformanceCounter(&nTotalEnd);//记录总时间的结尾
	time = (double)(nTotalEnd.QuadPart - nTotalStart.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "总运行时间：" << time * 1000 << "ms" << endl;

	glFinish();

	QueryPerformanceCounter(&BEGIN);
	//输出到CPU
	/*size_t triCount = 6 * width_2d * height_2d + 6 * (width_2d + height_2d);
	size_t bufferSize = triCount * sizeof(glm::vec4);
	glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
	const glm::vec4* triData = (const glm::vec4*)glMapBufferRange(
		GL_ARRAY_BUFFER,
		0,
		bufferSize,
		GL_MAP_READ_BIT
	);

	if (!triData) {
		fprintf(stderr, "Failed to map tri_buffer from GPU.\n");
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		return 0;
	}
	std::vector<glm::vec4> cpuTriBuffer;
	cpuTriBuffer.resize(triCount);
	memcpy(cpuTriBuffer.data(), triData, bufferSize);

	glUnmapBuffer(GL_ARRAY_BUFFER);*/
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	QueryPerformanceCounter(&END);
	time = (double)(END.QuadPart - BEGIN.QuadPart) / (double)nFreq.QuadPart;//计算程序执行时间单位为s  
	cout << "数据输出时间：" << time * 1000 << "ms" << endl;

	//输出
	//output_data_to_file(triangleVBO.tri_buffer1, bordertriVBO.tri_buffer1, "result.txt");
	output_data_to_file(triangleVBO, "result.txt");
	cout << "数据导出已完成" << endl;
	
	//背景点的VAO
	GLuint bgpoint_VAO;
	glGenVertexArrays(1, &bgpoint_VAO);
	glBindVertexArray(bgpoint_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);//为位置数据设置顶点属性指针
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, finally_buffer);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	glEnableVertexAttribArray(1);

	//准备插入点的VAO
	GLuint pointVAO;
	glGenVertexArrays(1, &pointVAO);
	glBindVertexArray(pointVAO);
	GLuint pointVBO;
	glGenBuffers(1, &pointVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
	glBufferData(GL_ARRAY_BUFFER, manyPoints.size() * sizeof(glm::vec4), &manyPoints[0], GL_STATIC_DRAW);
	//glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec4), &points[0], GL_STATIC_DRAW);
	//glBufferData(GL_ARRAY_BUFFER, points1.size() * sizeof(glm::vec4), &points1[0], GL_STATIC_DRAW);
	//glBufferData(GL_ARRAY_BUFFER, points2.size() * sizeof(glm::vec4), &points2[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	glEnableVertexAttribArray(0);

	//重映射点VAO
	//GLuint temp_pointVAO;
	//glGenVertexArrays(1, &temp_pointVAO);
	//glBindVertexArray(temp_pointVAO);
	//GLuint temp_pointVBO;
	//glGenBuffers(1, &temp_pointVBO);
	//glBindBuffer(GL_ARRAY_BUFFER, temp_pointVBO);
	//glBufferData(GL_ARRAY_BUFFER, temp_points.size() * sizeof(glm::vec4), &temp_points[0], GL_STATIC_DRAW);
	//glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	//glEnableVertexAttribArray(0);

	//三角形的VAO
	GLuint triangleVAO;
	glGenVertexArrays(1, &triangleVAO);
	glBindVertexArray(triangleVAO);
	glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	glEnableVertexAttribArray(0);

	//约束段
	//GLuint crossedgeVAO;
	//glGenVertexArrays(1, &crossedgeVAO);
	//glBindVertexArray(crossedgeVAO);
	//glBindBuffer(GL_ARRAY_BUFFER, influenttriVBO);
	//glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	//glEnableVertexAttribArray(0);

	//约束线段的VAO
	GLuint lineVAO, lineVBO;
	glGenVertexArrays(1, &lineVAO);
	glBindVertexArray(lineVAO);

	glGenBuffers(1, &lineVBO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec4), &lineVertices[0], GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	glEnableVertexAttribArray(0);
	
	Shader bgpointShader("bgpointVertex.vert", "bgpointFragment.frag");
	Shader pointShader("pointVertex.vert", "pointFragment.frag");
	Shader triangleShader("triangleVertex.vert", "triangleFragment.frag");
    Shader lineShader("lineVertex.vert", "lineFragment.frag");

	//Shader bgpointShader(bgvertexSource, bgfragmentSource,1);
	//Shader pointShader(pointVertexSource, pointFragmentSource, 1);
	//Shader triangleShader(trianglevertexSource, triangleFragmentSource, 1);

	GLint data[7];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, data);//每个维度上计算着色器可以处理的最大工作组数量。
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, data + 1);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, data + 2);

	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, data + 3);//每个工作组可以执行的最大工作项数。
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, data + 4);//每个工作组的最大尺寸，分别是x、y、z三个维度。
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, data + 5);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, data + 6);

	cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT:" << data[0] << " " << data[1] << " " << data[2] << endl;
	cout << "GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS:" << data[3] << endl;
	cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE:" << data[4] << " " << data[5] << " " << data[6] << endl;

	//show(window, bgpoint_VAO, bgpointShader, pointVAO, pointShader, offset, manyPoints, triangleVAO, triangleShader,bordertriVAO);

	cout << sizeof(glm::vec4) << endl;

	while (!glfwWindowShouldClose(window)) {
		//透视投影矩阵
		//视场角（FOV）为30度，宽高比为16:9，近平面为0.1，远平面为5000的投影矩阵
		glm::mat4 projection = glm::perspective(glm::radians(30.0f), 16.0f / 9.0f, 0.1f, 5000.0f);
		glm::mat4 view = glm::mat4(1.0f);
		view = camera.GetViewMatrix();

		glm::mat4 mvp = projection * view;
		glClearColor(1, 1, 1, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		
		//glBindVertexArray(bgpoint_VAO);
		//bgpointShader.use();
		//glUniformMatrix4fv(glGetUniformLocation(bgpointShader.ShaderProgram, "mvp"), 1, GL_FALSE, value_ptr(mvp));	// 传入 MVP 矩阵,将模型坐标转换为裁剪空间坐标
		//glUniform1f(glGetUniformLocation(bgpointShader.ShaderProgram, "total"), width_2d);
		//glUniform4fv(glGetUniformLocation(bgpointShader.ShaderProgram, "origin_offset"), 1, value_ptr(offset));
		//glDrawArrays(GL_POINTS, 0, width_2d *height_2d);

		//绘制三角网
		triangleShader.use();
		glUniformMatrix4fv(glGetUniformLocation(triangleShader.ShaderProgram, "mvp"), 1, GL_FALSE, value_ptr(mvp));	// 传入 MVP 矩阵
		glBindVertexArray(triangleVAO);
		glUniform4fv(glGetUniformLocation(triangleShader.ShaderProgram, "ucolor"), 1, value_ptr(vec4(0, 0, 0, 1)));
		//绘制模式为线框模式
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(1.0f);
		//矩形单元需要 6 个顶点(一个矩形两个三角形)
		glDrawArrays(GL_TRIANGLES, 0, width_2d * height_2d * 6 + 6 * (width_2d + height_2d));
		
		glBindVertexArray(pointVAO);
		pointShader.use();
		glUniformMatrix4fv(glGetUniformLocation(pointShader.ShaderProgram, "mvp"), 1, GL_FALSE, value_ptr(mvp));	// 传入 MVP 矩阵
		glPointSize(3.0f);
		glDrawArrays(GL_POINTS, 0, manyPoints.size());
		
		//绘制约束线段
		glBindVertexArray(lineVAO);
		lineShader.use();
		glUniformMatrix4fv(glGetUniformLocation(lineShader.ShaderProgram, "mvp"), 1, GL_FALSE, value_ptr(mvp));
		glLineWidth(1.5f);
		glDrawArrays(GL_LINES, 0, lineVertices.size());
		glBindVertexArray(0);

		//绘制相交线段
	   /* glBindVertexArray(crossedgeVAO);
		lineShader.use();
		glUniformMatrix4fv(glGetUniformLocation(lineShader.ShaderProgram, "mvp"), 1, GL_FALSE, value_ptr(mvp));
		glDrawArrays(GL_LINES, 0,  width_2d * lineVertices.size() );
		glBindVertexArray(0);*/

		processInput(window);
		glfwSwapBuffers(window);  //交换前后缓冲区，显示渲染结果
		glfwPollEvents();	//获取交互信息
	}
	glfwTerminate();
	return 0;
}