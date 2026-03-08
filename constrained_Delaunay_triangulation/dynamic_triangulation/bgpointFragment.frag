#version 460 core

in vec4 color;

out vec4 FragColor;

uniform float total;

void main(){
	float scale = 2.0; // 越大，亮度对比越明显
	//归一化 压缩高亮度值 增强低亮度值
	vec4 acolor=vec4(log(1 + scale * color.x/total),log(1 + scale * color.y/total),(color.x + scale * color.y)/total,0);
	FragColor = acolor;
}