#include <stdio.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glut.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include <math.h>
#include "matrix.h"
#include "md2.h"
#include "md2file.h"
#include "md2render.h"
#include "gli.h"
#include "loadtex.h"

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgGL.lib")
static CGcontext myCgContext;
static CGprofile myCgVertexProfile;
static CGprogram myCgVertexProgram;
static CGprofile myCgFragmentProfile;
static CGprogram myCgFragmentProgram;
static CGparameter weight;
static CGparameter scale;
static CGparameter changeCoordMatrix;
static CGparameter globalAmbient; //环境光颜色
static CGparameter lightColor; //灯光颜色
static CGparameter lightPosition; //灯的位置
static CGparameter Kc;
static CGparameter Kl;
static CGparameter Kq;
static CGparameter eyePosition;  //眼睛的位置
static CGparameter Ke; //材质自身颜色
static CGparameter Ka; //环境光系数
static CGparameter Kd; //漫反射光系数
static CGparameter Ks; //镜面射光系数
static CGparameter shininess; //材质光滑程度
static const char* frameTitle = "Cg Test";
static const char* cgVFileName = "VertexCG.cg";
static const char* cgVFuncName = "VertexMain";
static const char* cgFFileName = "FragmentCG.cg";
static const char* cgFFuncName = "TextureMain";
static Md2Model *m_knightModel;
static MD2render *m_knightRender;
static gliGenericImage *m_texture;
static float knightKnob = 0;
static int frameA = 0;
static int frameB = 0;
static float curOffset = 0;
static float stepOffset = 0.001;
static float mGlobalAmbient[3] = { 0.2, 0.2, 0.2 };
static float mLightColor[3] = { 1, 1, 1 };
static int lastTime;
static bool animating = false;
static bool lighting = true;
static bool isWireFrame = false;

float projectionMatrix[16]; //投影矩阵


void OnDraw();
void ReShape(int width, int height);
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
float AddDelta(float knightKnob, float detal, int numFrames);
void Visibility(int state);
void SetModelMaterial();

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(300, 300);
	glutCreateWindow(frameTitle);
	glutDisplayFunc(&OnDraw);
	glutVisibilityFunc(&Visibility);
	glutKeyboardFunc(&OnKeyBoard);
	glutReshapeFunc(&ReShape);

	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "failed to initalize GLEW.\n");
		exit(1);
	}
	m_knightModel = md2ReadModel(".//knight.md2");
	m_knightRender = createMD2render(m_knightModel);
	m_texture = readImage(".//knight.tga");
	m_texture = loadTextureDecal(m_texture, 1);
	gliFree(m_texture);

	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
	glLineWidth(2);
	myCgContext = cgCreateContext();
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
	GET_VERTEX_PROGRAM(weight);

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
	GET_FRAGMENT_PROGRAM(eyePosition);
	GET_FRAGMENT_PROGRAM(scale);
	cgSetParameter2f(scale, 1.0 / m_knightModel->header.skinWidth, 1.0 / m_knightModel->header.skinHeight);

	#define GET_FRAGMENT_PROGRAM_2(vsName, cgName) \
	vsName = \
	cgGetNamedParameter(myCgFragmentProgram, #cgName);\
	CheckCgError("Get Named "#cgName" Parameter Error");

	GET_FRAGMENT_PROGRAM_2(lightColor, light.lightColor);
	GET_FRAGMENT_PROGRAM_2(lightPosition, light.lightPosition);
	GET_FRAGMENT_PROGRAM_2(Kc, light.Kc);
	GET_FRAGMENT_PROGRAM_2(Kl, light.Kl);
	GET_FRAGMENT_PROGRAM_2(Kq, light.Kq);
	GET_FRAGMENT_PROGRAM_2(Ke, material.Ke);
	GET_FRAGMENT_PROGRAM_2(Ka, material.Ka);
	GET_FRAGMENT_PROGRAM_2(Kd, material.Kd);
	GET_FRAGMENT_PROGRAM_2(Ks, material.Ks);
	GET_FRAGMENT_PROGRAM_2(shininess, material.shininess);

	CheckCgError("Get Named Parameter Error");
	glutMainLoop();
	return 0;
}

void SetModelMaterial()
{
	const float mKe[3] = { 0, 0, 0 };//自发光
	const float mKa[3] = { 0.6, 0.6, 0.6 };//环境光系数
	const float mKd[3] = { 0.9, 0.9, 0.9 };//漫反射光系数
	const float mKs[3] = { 0.99, 0.99, 0.99 };//镜面射光系数
	const float mShininess = 8;
	cgSetParameter3fv(Ke, mKe);
	cgSetParameter3fv(Ka, mKa);
	cgSetParameter3fv(Kd, mKd);
	cgSetParameter3fv(Ks, mKs);
	cgSetParameter1f(shininess, mShininess);
}

float AddDelta(float knightKnob, float detal, int numFrames)
{
	knightKnob += detal;
	while (numFrames <= knightKnob)
	{
		knightKnob -= numFrames;
	}
	if (knightKnob < 0)
		knightKnob = 0;
	return knightKnob;
}

void OnDraw()
{
	float eyeRadius = 120;
	frameA = floor(knightKnob);
	frameB = frameA + 1 > m_knightModel->header.numFrames ? 0 : frameA + 1;
	float m_EyePosition[3] = { cos(3.14/5) * eyeRadius, 0, sin(3.14 / 5) * eyeRadius };
	float translationMatrix[16], rotateMatrix[16], viewMatrix[16], finalMatrix[16];
	float tempPosition[4];
	float mLightPosition[4] = {150, 120, 150, 1};
	float mKc = 0.0002;
	float mKl = 0.0001;
	float mKq = 0.00003;
	float mEyePosition[4] = { m_EyePosition[0], m_EyePosition[1], m_EyePosition[2] , 1};
	if (lighting)
	{
		mLightColor[0] = 1;
		mLightColor[1] = 1;
		mLightColor[2] = 1;
	}
	else
	{
		mLightColor[0] = 0;
		mLightColor[1] = 0;
		mLightColor[2] = 0;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cgGLBindProgram(myCgVertexProgram);
	CheckCgError("Bind Vertex Program Error");
	cgGLEnableProfile(myCgVertexProfile);
	CheckCgError("Enable Vertex Profile Error");

	cgGLBindProgram(myCgFragmentProgram);
	CheckCgError("Bind Fragment Program Error");
	cgGLEnableProfile(myCgFragmentProfile);
	CheckCgError("Enable Fragment Profile Error");

	buildLookAtMatrix(m_EyePosition[0], m_EyePosition[1], m_EyePosition[2], 0, 0, 0, 0, 1, 0, viewMatrix);
	multMatrix(finalMatrix, projectionMatrix, viewMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);
	cgSetParameter1f(weight, knightKnob - floor(knightKnob));

	cgSetParameter4fv(globalAmbient, mGlobalAmbient);
	SetModelMaterial();
	cgSetParameter3fv(lightColor, mLightColor);
	cgSetParameter1f(Kc, mKc);
	cgSetParameter1f(Kl, mKl);
	cgSetParameter1f(Kq, mKq);

	cgSetParameter4fv(lightPosition, mLightPosition);
	cgSetParameter4fv(eyePosition, mEyePosition);

	drawMD2render(m_knightRender, frameA, frameB);

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	cgGLDisableProfile(myCgVertexProfile);
	CheckCgError("Disable Vertex Profile Error");

	cgGLDisableProfile(myCgFragmentProfile);
	CheckCgError("Disable Fragment Profile Error");

	glutSwapBuffers();
}

void ReShape(int width, int height)
{
	double aspectRatio = (float)width / (float)height;
	double fieldView = 30;
	buildPerspectiveMatrix(fieldView, aspectRatio, 1.0, 500.0, projectionMatrix);
	glViewport(0, 0, width, height);
}

void PicMoving()
{
	const float numSystem = 1000.0;
	const int frameNumPerSec = 6;
	int nowTime = glutGet(GLUT_ELAPSED_TIME);
	float detalTime = (nowTime - lastTime) / numSystem;
	lastTime = nowTime;
	detalTime *= frameNumPerSec;
	knightKnob = AddDelta(knightKnob, detalTime, m_knightModel->header.numFrames);
	curOffset += stepOffset;
	if (curOffset >= 2 * myPi)
	{
		curOffset -= 2 * myPi;
	}
	glutPostRedisplay();
}

void Visibility(int state)
{
	if (state == GLUT_VISIBLE && animating)
	{
		lastTime = glutGet(GLUT_ELAPSED_TIME);
		glutIdleFunc(PicMoving);
	}
	else {
		glutIdleFunc(NULL);
	}
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
		animating = !animating;
		if (animating)
		{
			lastTime = glutGet(GLUT_ELAPSED_TIME);
			glutIdleFunc(PicMoving);
		}
		else
		{
			glutIdleFunc(NULL);
		}
		break;
	case 'L':
	case 'l':
		lighting = !lighting;
		glutPostRedisplay();
		break;
	case 'W':
	case 'w':
		isWireFrame = !isWireFrame;
		if (isWireFrame)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glutPostRedisplay();
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
