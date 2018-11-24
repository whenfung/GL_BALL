//片元着色器的功能主要是利用光源位置给物体打光

#version 330 core

in vec3 N;
in vec3 V;

uniform vec3 lightPos;
uniform int isShadow;

out vec4 fragmentColor;

void main()
{
	// 设置三维物体的材质属性
	vec3 ambiColor = vec3(0.4, 0.2, 0.2);          //环境光
	vec3 diffColor = vec3(0.5, 0.9, 0.9);          //漫反射
	vec3 specColor = vec3(0.3, 0.6, 0.6);          //镜面

	//  计算N，L，V，R四个向量并归一化，归一化就是为了求角度的cos值之类的
	vec3 N_norm = normalize(N);                   //顶点法向量
	vec3 L_norm = normalize(lightPos - V);        //顶点指向光，也就是视线方向
	vec3 V_norm = normalize(-V);                  //镜头到物体的方向,view
	vec3 R_norm = reflect(L_norm, N_norm);        //反射光

	// 计算漫反射系数和镜面反射系数，clamp的意思是内积值在0到1之前，如果小选0，如果大选1
	float lambertian = clamp(dot(L_norm, N_norm), 0.0, 1.0); // 只考虑法向量分量上的光能进行漫反射
	float specular = clamp(dot(R_norm, V_norm), 0.0, 1.0);    //镜面反射，只有镜面反射在V方向上的分量才能进入肉眼
	
	// 根据是否为阴影选择颜色
	if (isShadow == 1) 
	{
		fragmentColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else 
	{ 
		//这里的pow中的指数代表高光指数，高光指数越小，高光区域越大，高光指数接近无穷的话就是镜子的反射
		fragmentColor = vec4(ambiColor + diffColor * lambertian + specColor * pow(specular, 10.0), 1.0);
	}
}
