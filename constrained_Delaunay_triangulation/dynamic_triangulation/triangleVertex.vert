#version 460 core
layout(location=0) in vec4 aPos;

uniform mat4 mvp;
uniform vec4 ucolor=vec4(1,1,1,1);//默认值-在glUniform4fv没有被调用或者设置为空时才会使用

out vec4 color;

void main(){
	color=ucolor;
	gl_Position=mvp*vec4(aPos.xy,0,1);
}