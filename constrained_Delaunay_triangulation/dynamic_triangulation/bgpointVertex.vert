#version 460 core
layout(location=0) in vec4 aPos;
layout(location=1) in vec4 aColor;

out vec4 color;

uniform mat4 mvp;
uniform vec4 origin_offset;

void main(){
	gl_Position=mvp*vec4(aPos.xy,0,1);
	color=vec4(aColor.x-origin_offset.x,aColor.y-origin_offset.y,0,0);
}