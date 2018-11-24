//顶点着色器主要是对顶点进行相应的坐标系变换和仿射变换

#version 330 core

in vec3 vPosition;  //顶点位置
in vec3 vNormal;    //每个顶点的法向量

// 旋转矩阵
uniform mat4 rotationMatrix;
// 模视变换矩阵
uniform mat4 modelViewMatrix;
// 投影矩阵
uniform mat4 projMatrix;

out vec3 N;   //法向量
out vec3 V;   //透视投影

// Phong 光照模型的实现 (per-fragment shading)

void main()
{
	//一个物体如果想正常显示在屏幕上，需要以下几个步骤
	//1、将物体从模型坐标系转到世界坐标系，这样的话多个物体才有所谓的相对位置
	//2、将世界坐标系转到相机坐标系，世界这么大，你的眼睛选择看哪一部分，就由你自己为中心建一个坐标系
	//3、前面两点可以由模视矩阵完成modelViewMatrix
	//4、我们看见的东西其实一张画面，也就是我们得将三维的投影到视见体中，这时候需要一个投影矩阵*模视矩阵
	//5、如果是影子的话怎么处理，先将物体转到相机坐标系，然后进行投影，即乘以阴影矩阵

	vec4 v = projMatrix * modelViewMatrix * vec4(vPosition, 1.0);    //进行前面的5步将其弄到我们眼中一个二维平面上
	
	//我们的阴影矩阵最后还要进行透视除法，公式决定的，没办法
	vec4 point = vec4(v.xyz / v.w, 1.0);

	//注意，旋转之类的变化是在相机坐标系下进行的，所以最后乘以变化矩阵
	gl_Position = rotationMatrix * vec4(v.xyz / v.w, 1.0);  //这里进行了旋转变换

	// 将顶点变换到相机坐标系下
	vec4 vertPos_cameraspace = modelViewMatrix * vec4(vPosition, 1.0);

	// 将法向量变换到相机坐标系下并传入片元着色器，由于光源什么的都是在相机坐标系下处理，所以得将顶点转移到相机坐标系下处理
	N = (modelViewMatrix * vec4(vNormal, 0.0)).xyz;
	
	// 对顶点坐标做透视除法，阴影处理
	V = vertPos_cameraspace.xyz / vertPos_cameraspace.w;
}
