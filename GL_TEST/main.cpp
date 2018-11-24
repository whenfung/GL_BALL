
#include "Angel.h"
#include "TriMesh.h"

#pragma comment(lib, "glew32.lib")

using namespace std;

//各种对象引用
GLuint programID;
GLuint vertexArrayID;
GLuint vertexBufferID;
GLuint vertexNormalID;
GLuint vertexIndexBuffer;

GLuint vPositionID;
GLuint rotationMatrixID;
GLuint modelViewMatrixID;
GLuint projMatrixID;
GLuint lightPosID;
GLuint vNormalID;
GLuint shadowID;

int mainWindow;

// 相机视见体参数
float l = -2.0, r = 2.0;    // 左右裁剪平面
float b = -2.0, t = 2.0;    // 上下裁剪平面
float n = -2.0, f = 2.0;    // 远近裁剪平面
float rotationAngle = -5.0; // 旋转角度，由于阴影投射在y=0平面上，我们需要绕X轴旋转才能看见

std::vector<vec3> points;
std::vector<vec3i> faces;

vec4 black(0.0, 0.0, 0.0, 1.0);

float lightPos[3] = { -0.5, 2.0, 0.5 };   //光源位置

TriMesh* mesh = new TriMesh();

namespace Camera
{
	//正交透视矩阵，其视见体是一个标准长方体，可以用自带函数
	mat4 ortho(const GLfloat left, const GLfloat right,
		const GLfloat bottom, const GLfloat top,
		const GLfloat zNear, const GLfloat zFar)
	{
		vec4 c1 = vec4(2 / (right - left), 0, 0, 0);
		vec4 c2 = vec4(0, 2 / (top - bottom), 0, 0);
		vec4 c3 = vec4(0, 0, -2 / (zFar - zNear), 0);
		vec4 c4 = vec4((right + left) / (left - right), (top + bottom) / (bottom - top), (zFar + zNear) / (zNear - zFar), 1);
		return mat4(c1, c2, c3, c4);
	}

	//透视投影，近大远小，可以用自带函数
	mat4 perspective(const GLfloat fovy, const GLfloat aspect,
		const GLfloat zNear, const GLfloat zFar)
	{
		GLfloat top = zNear * tan(fovy);
		GLfloat right = top * aspect;
		vec4 c1 = vec4(zNear / right, 0, 0, 0);
		vec4 c2 = vec4(0, zNear / top, 0, 0);
		vec4 c3 = vec4(0, 0, (zFar + zNear) / (zNear - zFar), -1);
		vec4 c4 = vec4(0, 0, (2 * zFar * zNear) / (zNear - zFar), 0);
		return mat4(c1, c2, c3, c4);
	}

	// 计算观察矩阵函数，可以用自带函数
	mat4 lookAt(const vec4& eye, const vec4& at, const vec4& up)
	{
		vec4 n = normalize(eye - at);
		vec4 u = vec4(normalize(cross(up, n)), 0);
		vec4 v = vec4(normalize(cross(n, u)), 0);
		vec4 t = vec4(0, 0, 0, 1);
		return mat4(u, v, n, t) * Translate(-eye);
	}

	mat4 modelMatrix(1.0);   //默认模型矩阵就是单位矩阵
	mat4 viewMatrix(1.0);    //默认观察矩阵就是单位矩阵，那么中点就在世界坐标系原点
	mat4 projMatrix = ortho(-2, 2, -2, 2, -2, 2);  //投影矩阵，在一个4*4*4的正方体内可见
}

void init()
{
	// 设置背景为灰色，为了看见影子
	glClearColor(0.4f, 0.4f, 0.4f, 0.0f);

	programID = InitShader("vshader.glsl", "fshader.glsl");

	// 从顶点着色器中获取相应变量的位置
	vPositionID = glGetAttribLocation(programID, "vPosition");
	vNormalID = glGetAttribLocation(programID, "vNormal");
	modelViewMatrixID = glGetUniformLocation(programID, "modelViewMatrix");
	projMatrixID = glGetUniformLocation(programID, "projMatrix");
	rotationMatrixID = glGetUniformLocation(programID, "rotationMatrix");
	lightPosID = glGetUniformLocation(programID, "lightPos");
	shadowID = glGetUniformLocation(programID, "isShadow");

	//读取OFF文件
	mesh->read_off("sphere.off");

	vector<vec3f> vs = mesh->v();  //顶点数组
	vector<vec3i> fs = mesh->f();  //片面数组
	vector<vec3f> ns;              //顶点向量数组


	vec3f shift = vs[0];

	for (int i = 0; i < vs.size(); ++i)
		if (vs[i].y < shift.y) shift = vs[i];

	for (int i = 0; i < vs.size(); ++i)  //为了能够得到影子，将球心往上移动
	{
		vs[i].y -= shift.y-0.1;
		ns.push_back(vs[i] - vec3(0.0, -shift.y, 0.0));
	}

	// 前面我们得到了所有想要的数据和变量名，下面开始传输数据
	//生成VAO（顶点数组对象）
	glGenVertexArrays(1, &vertexArrayID);
	glBindVertexArray(vertexArrayID);

	// 生成VBO，并绑定顶点坐标
	glGenBuffers(1, &vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(vec3f), vs.data(), GL_STATIC_DRAW);

	//生成绑定顶点向量缓存对象
	glGenBuffers(1, &vertexNormalID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexNormalID);
	glBufferData(GL_ARRAY_BUFFER, ns.size() * sizeof(vec3f), ns.data(), GL_STATIC_DRAW);

	//生成片面的顶点索引
	glGenBuffers(1, &vertexIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, fs.size() * sizeof(vec3i), fs.data(), GL_STATIC_DRAW);

	// OpenGL相应状态设置
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
}

// 开始渲染，我们的数据传到GPU存储后，需要渲染才能显示到屏幕上
void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);

	glEnableVertexAttribArray(vPositionID);  //开启顶点数组

	mat4 modelViewMatrix = Camera::viewMatrix * Camera::modelMatrix;
	// 计算旋转矩阵，绕X轴旋转使得阴影看得更清晰
	mat4 rotationMatrix = RotateX(rotationAngle); 
	glUniformMatrix4fv(rotationMatrixID, 1, GL_TRUE, rotationMatrix);

	float lx = lightPos[0];
	float ly = lightPos[1];
	float lz = lightPos[2];
	// 计算阴影矩阵
	mat4 shadowProjMatrix(
		-ly, 0.0, 0.0, 0.0,
		lx, 0.0, lz, 1.0,
		0.0, 0.0, -ly, 0.0,
		0.0, 0.0, 0.0, -ly);
	// 阴影是在相机坐标系下处理的，所以是乘以模视矩阵
	shadowProjMatrix = shadowProjMatrix * modelViewMatrix; //阴影的模视矩阵
	glUniformMatrix4fv(projMatrixID, 1, GL_TRUE, Camera::projMatrix);
	glUniformMatrix4fv(modelViewMatrixID, 1, GL_TRUE, shadowProjMatrix);
	// 设置阴影标志位为1
	glUniform1i(shadowID, 1);
	// 如果光源的位置在小球上方则绘制阴影
	if (ly > 0) {
		glDrawElements(  //利用顶点索引进行顶点着色器绘画，在我的理解是以下标的形式访问
			GL_TRIANGLES,
			int(mesh->f().size() * 3),
			GL_UNSIGNED_INT,
			(void*)0
		);
	}

	//绘画实物的时候就要将模视矩阵修改为原来的，并设置为可光照
	glUniformMatrix4fv(modelViewMatrixID, 1, GL_TRUE, modelViewMatrix);
	// 设置阴影标志位为0
	glUniform1i(shadowID, 0);

	// 绘制小球
	glDrawElements(
		GL_TRIANGLES,
		int(mesh->f().size() * 3),
		GL_UNSIGNED_INT,
		(void*)0
	);

	// 传入光源信息
	glUniform3fv(lightPosID, 1, lightPos);

	//绑定顶点缓存
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glVertexAttribPointer(
		vPositionID,
		3,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	//绑定并开启顶点向量缓存
	glEnableVertexAttribArray(vNormalID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexNormalID);
	glVertexAttribPointer(
		vNormalID,
		3,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	//绑定顶点索引缓存
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndexBuffer);

	glDisableVertexAttribArray(vPositionID);
	glUseProgram(0);

	glutSwapBuffers();
}

// 重新设置窗口
void reshape(GLsizei w, GLsizei h)
{
	glViewport(0, 0, w, h);
}

// 鼠标响应函数
void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		lightPos[0] = (x - 250) / 10;
		lightPos[1] = (250 - y) / 10;
	}
	glutPostWindowRedisplay(mainWindow);
}

// 键盘响应函数
void specialKey(int key, int x, int y)
{
	if (key == GLUT_KEY_UP) {
		rotationAngle += 1.0;
	}
	if (key == GLUT_KEY_DOWN) {
		rotationAngle -= 1.0;
	}
	glutPostRedisplay();
}


//回调函数，这个函数使得变化不会太僵硬
void idle(void)
{
	glutPostRedisplay();
}


//清除内存
void clean()
{
	glDeleteBuffers(1, &vertexBufferID);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &vertexArrayID);

	if (mesh)
	{
		delete mesh;
		mesh = NULL;
	}
}


//主函数
int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);  //双缓冲
	glutInitWindowSize(500, 500);
	mainWindow = glutCreateWindow("邓文丰 2016150182");

	glewExperimental = GL_TRUE;
	glewInit();
	init();

	//各种回调函数
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutSpecialFunc(specialKey);
	glutIdleFunc(idle);

	glutMainLoop();

	clean();
	return 0;
}