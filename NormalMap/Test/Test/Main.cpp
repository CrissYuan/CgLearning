#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include <math.h>
#include "matrix.h"

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgGL.lib")
static CGcontext myCgContext;
static CGprofile myCgVertexProfile;
static CGprogram myCgVertexProgram;
static CGprofile myCgFragmentProfile;
static CGprogram myCgFragmentProgram;
static CGparameter changeCoordMatrix;
static CGparameter globalAmbient; //环境光颜色
static CGparameter lightColor;    //灯光颜色
static CGparameter lightPosition; //灯的位置
static CGparameter eyePosition;   //眼睛的位置
static CGparameter Ke;            //材质自身颜色
static CGparameter Ka;            //环境光系数
static CGparameter shininess;     //材质光滑程度
static CGparameter textureMap;
static CGparameter normalMap;

static const char* frameTitle = "Cg Test";
static const char* cgVFileName = "VertexCG.cg";
static const char* cgVFuncName = "VertexMain";
static const char* cgFFileName = "FragmentCG.cg";
static const char* cgFFuncName = "TextureMain";
static const char* mainTextureDir = ".//brick.bmp";
static const char* noramlMapDir = ".//noramlMap.bmp";
static float curOffset = 0;
static float stepOffset = 0.001;
static float mGlobalAmbient[3] = {0.1, 0.1, 0.05};
static float mLightColor[3] = { 1, 1, 0.95 };
float projectionMatrix[16]; //投影矩阵
static GLubyte mainTextureImg[3 * 512 * 512];
static GLubyte normalMapImg[3 * 512 * 512];
float m_lightAngle = 4;
float m_eyeHeight = 0;
float m_eyeAngle = 0;
bool m_animating = true; //控制动画

enum 
{
	TO_TEXTURE_MAP = 0,
	TO_NORMAL_MAP
};

GLuint texObj[2];

void OnDraw();
void ReShape(int width, int height);
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
void LoadBMP(GLubyte* Img, const char* Filename);
void Idle();
void Mouse(int button, int state, int x, int y);
void Motion(int x, int y);

int main(int argc, char *argv[])
{
	LoadBMP(mainTextureImg, mainTextureDir);
	LoadBMP(normalMapImg, noramlMapDir);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow(frameTitle);
	glutDisplayFunc(&OnDraw);
	glutKeyboardFunc(&OnKeyBoard);
	glutReshapeFunc(&ReShape);
	glutMouseFunc(&Mouse);
	glutMotionFunc(&Motion);
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "%s: failed to initialize Glew.\n");
		exit(1);
	}
	glClearColor(0, 0, 0, 0);
	glEnable(GL_CULL_FACE);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(2, texObj);

	/* 加载纹理贴图 */
	glBindTexture(GL_TEXTURE_2D, texObj[TO_TEXTURE_MAP]);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB8, 512, 512, GL_RGB, GL_UNSIGNED_BYTE, mainTextureImg);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	/* 加载法线贴图 */
	glBindTexture(GL_TEXTURE_2D, texObj[TO_NORMAL_MAP]);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB8, 512, 512, GL_RGB, GL_UNSIGNED_BYTE, normalMapImg);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	myCgContext = cgCreateContext();
	cgGLSetManageTextureParameters(myCgContext, CG_TRUE);

	myCgVertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
	cgGLSetOptimalOptions(myCgVertexProfile);
	CheckCgError("Set Profile Error");
	myCgVertexProgram = cgCreateProgramFromFile(
		myCgContext,
		CG_SOURCE,
		cgVFileName,
		myCgVertexProfile,
		cgVFuncName,
		0);
	CheckCgError("Create Program Error");
	cgGLLoadProgram(myCgVertexProgram);
	CheckCgError("Load Program Error");
	#define GET_VERTEX_PROGRAM(name) \
	name = \
	cgGetNamedParameter(myCgVertexProgram, #name);\
	CheckCgError("Get Named "#name" Parameter Error");

	GET_VERTEX_PROGRAM(changeCoordMatrix);

	myCgFragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
	cgGLSetOptimalOptions(myCgFragmentProfile);
	CheckCgError("Set Profile Error");
	myCgFragmentProgram = cgCreateProgramFromFile(
		myCgContext,
		CG_SOURCE,
		cgFFileName,
		myCgFragmentProfile,
		cgFFuncName,
		0);
	CheckCgError("Create Program Error");
	cgGLLoadProgram(myCgFragmentProgram);
	CheckCgError("Load Program Error");
	#define GET_FRAGMENT_PROGRAM(name) \
	name = \
	cgGetNamedParameter(myCgFragmentProgram, #name);\
	CheckCgError("Get Named "#name" Parameter Error");
	GET_FRAGMENT_PROGRAM(globalAmbient);
	GET_FRAGMENT_PROGRAM(lightColor);
	GET_FRAGMENT_PROGRAM(lightPosition);
	GET_FRAGMENT_PROGRAM(eyePosition);
	GET_FRAGMENT_PROGRAM(Ke);
	GET_FRAGMENT_PROGRAM(Ka);
	GET_FRAGMENT_PROGRAM(shininess);
	GET_FRAGMENT_PROGRAM(textureMap);
	GET_FRAGMENT_PROGRAM(normalMap);

	cgGLSetTextureParameter(textureMap, texObj[TO_TEXTURE_MAP]);
	cgGLSetTextureParameter(normalMap, texObj[TO_NORMAL_MAP]);
	CheckCgError("Get Named Parameter Error");
	glutMainLoop();
	return 0;
}

void SetWallMaterial()
{
	const float mKe[3] = { 0, 0, 0 };//自发光
	const float mKa[3] = { 0.3, 0.2, 0.01 };//环境光系数
	const float mShininess = 10;
	cgSetParameter3fv(Ke, mKe);
	cgSetParameter3fv(Ka, mKa);
	cgSetParameter1f(shininess, mShininess);
}

void OnDraw()
{
	float mLightPosition[3] = {
		5.0f * sin(m_lightAngle), 5.0f * cos(m_lightAngle), 4
	};
	float mEyePosition[3] = {
		20.0f * sin(m_eyeAngle), m_eyeHeight, 20.0f * cos(m_eyeAngle)
	};

	glClear(GL_COLOR_BUFFER_BIT);
	cgGLBindProgram(myCgVertexProgram);
	CheckCgError("Bind Vertex Program Error");
	cgGLEnableProfile(myCgVertexProfile);
	CheckCgError("Enable Vertex Profile Error");

	cgGLBindProgram(myCgFragmentProgram);
	CheckCgError("Bind Fragment Program Error");
	cgGLEnableProfile(myCgFragmentProfile);
	CheckCgError("Enable Fragment Profile Error");

	cgSetParameter4fv(globalAmbient, mGlobalAmbient);
	cgSetParameter4fv(lightColor, mLightColor);

	glLoadIdentity();
	gluLookAt(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 0, 0, 0, 1, 0);
	cgSetParameter3fv(lightPosition, mLightPosition);
	cgSetParameter3fv(eyePosition, mEyePosition);
	cgGLSetStateMatrixParameter(changeCoordMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);

	cgGLEnableTextureParameter(textureMap);
	CheckCgError("Enable Texture Parameter textureMap Error");
	cgGLEnableTextureParameter(normalMap);
	CheckCgError("Enable Texture Parameter normalMap Error");

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	SetWallMaterial();
	glBegin(GL_QUADS);
		//back wall
		glNormal3f(0, 0, 1);
		glMultiTexCoord3f(GL_TEXTURE2, 1, 0, 0);
		glTexCoord2f(0, 0);
		glVertex2f(-7, -7);

		glTexCoord2f(1, 0);
		glVertex2f(7, -7);

		glTexCoord2f(1, 1);
		glVertex2f(7, 7);

		glTexCoord2f(0, 1);
		glVertex2f(-7, 7);

		//floor
		glNormal3f(0, 1, 0);
		glMultiTexCoord3f(GL_TEXTURE2, 1, 0, 0);
		glTexCoord2f(0, 0);
		glVertex3f(-7, -7, 14);

		glTexCoord2f(1, 0);
		glVertex3f(7, -7, 14);

		glTexCoord2f(1, 1);
		glVertex3f(7, -7, 0);

		glTexCoord2f(0, 1);
		glVertex3f(-7, -7, 0);

		//celling
		glNormal3f(0, -1, 0);
		glMultiTexCoord3f(GL_TEXTURE2, 1, 0, 0);
		glTexCoord2f(0, 0);
		glVertex3f(-7, 7, 0);

		glTexCoord2f(1, 0);
		glVertex3f(7, 7, 0);

		glTexCoord2f(1, 1);
		glVertex3f(7, 7, 14);

		glTexCoord2f(0, 1);
		glVertex3f(-7, 7, 14);

		//left wall
		glNormal3f(1, 0, 0);
		glMultiTexCoord3f(GL_TEXTURE2, 0, -1, 0);
		glTexCoord2f(0, 0);
		glVertex3f(-7, -7, 14);

		glTexCoord2f(1, 0);
		glVertex3f(-7, -7, 0);

		glTexCoord2f(1, 1);
		glVertex3f(-7, 7, 0);

		glTexCoord2f(0, 1);
		glVertex3f(-7, 7, 14);

		//right wall
		glNormal3f(-1, 0, 0);
		glMultiTexCoord3f(GL_TEXTURE2, 0, 0, 1);
		glTexCoord2f(0, 0);
		glVertex3f(7, -7, 0);

		glTexCoord2f(1, 0);
		glVertex3f(7, -7, 14);

		glTexCoord2f(1, 1);
		glVertex3f(7, 7, 14);

		glTexCoord2f(0, 1);
		glVertex3f(7, 7, 0);

		//front wall
		glNormal3f(0, 0, -1);
		glMultiTexCoord3f(GL_TEXTURE2, 1, 0, 0);
		glTexCoord2f(0, 0);
		glVertex3f(-7, 7, 14);

		glTexCoord2f(1, 0);
		glVertex3f(7, 7, 14);

		glTexCoord2f(1, 1);
		glVertex3f(7, -7, 14);

		glTexCoord2f(0, 1);
		glVertex3f(-7, -7, 14);
	glEnd();
	cgGLDisableProfile(myCgVertexProfile);
	CheckCgError("Disable Vertex Profile Error");

	cgGLDisableProfile(myCgFragmentProfile);
	CheckCgError("Disable Fragment Profile Error");

	glTranslatef(mLightPosition[0], mLightPosition[1], mLightPosition[2]);
	glColor3f(0.9, 0.9, 0.9);
	glutSolidSphere(0.2, 12, 12);

	glutSwapBuffers();
}

void ReShape(int width, int height)
{
	double aspectRatio = (float)width / (float)height;
	double fieldOfView = 75.0;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fieldOfView, aspectRatio, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, width, height);
}

void Idle()
{
	if (m_animating)
	{
		m_lightAngle += 0.0008;
		if (m_lightAngle > 3.1415926535 * 2)
		{
			m_lightAngle -= 3.1415926535 * 2;
		}
	}
	glutPostRedisplay();
}

void OnKeyBoard(unsigned char c, int x, int y)
{
	switch (c)
	{
	case 27:
		cgDestroyProgram(myCgVertexProgram);
		cgDestroyProgram(myCgFragmentProgram);
		cgDestroyContext(myCgContext);
		exit(0);
		break;
	case ' ':
		m_animating = !m_animating;
		if (m_animating)
		{
			glutIdleFunc(Idle);
		}
		else
		{
			glutIdleFunc(NULL);
		}
		break;
	default:
		break;
	}
}

void CheckCgError(const char* situation)
{
	CGerror error;
	const char* errorStr = cgGetLastErrorString(&error);
	if (error != CG_NO_ERROR)
	{
		printf("%s >>> %s >>> %s\n", frameTitle, situation, errorStr);
		if (error == CG_COMPILER_ERROR)
		{
			printf("%s >>> %s\n", cgVFileName, cgGetLastListing(myCgContext));
		}
		exit(1);
	}
}

void LoadBMP(GLubyte* Img, const char* Filename) //加载.bmp纹理方法
{
	FILE *file = fopen(Filename, "rb");
	if (not file) return;
	fread(Img, sizeof(unsigned char), 54, file); //前54位为.bmp头结构
	fread(Img, sizeof(unsigned char), 3 * 512 * 512, file);

	for (int i = 0; i < 3 * 512 * 512; i += 3) {
		Img[i] ^= Img[i + 2];
		Img[i + 2] ^= Img[i];
		Img[i] ^= Img[i + 2];
	}
	fclose(file);
}

int moving = 0;
int beginx, beginy;

void Motion(int x, int y)
{
	const float heightBound = 20;

	if (moving)
	{
		m_eyeAngle += 0.005 * (beginx - x);
		m_eyeHeight += 0.01 * (y - beginy);
		if (m_eyeHeight > heightBound)
		{
			m_eyeHeight = heightBound;
		}
		if (m_eyeHeight < -heightBound)
		{
			m_eyeHeight = -heightBound;
		}
		beginx = x;
		beginy = y;
		glutPostRedisplay();
	}
}

void Mouse(int button, int state, int x, int y)
{
	const int spinButton = GLUT_LEFT_BUTTON;

	if (button == spinButton && state == GLUT_DOWN)
	{
		moving = 1;
		beginx = x;
		beginy = y;
	}
	if (button == spinButton && state == GLUT_UP)
	{
		moving = 0;
	}
}