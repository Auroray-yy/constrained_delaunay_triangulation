#version 460 core
layout(location=0) in vec4 aPos;

uniform mat4 mvp;

out vec4 color;

void main(){
	int ix=int(aPos.x);
	int iy=int(aPos.y);
	float dx=aPos.x-ix,dy=aPos.y-iy;
	if(dx!=0.0f||dy!=0.0f){
		color=vec4(0,0,0,1);
	}else{
		color=vec4(0,0,0,1);
	}
	//color=vec4(1,0,0,1);
	gl_Position=mvp*vec4(aPos.xy,0.01,1);
}