#include <stdio.h>
#include <GL/glut.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include <math.h>
#include "matrix.h"

#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"glut32.lib")
#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgGL.lib")
static CGcontext myCgContext;
static CGprofile myCgVertexProfile;
static CGprogram myCgVertexProgram;
static CGprofile myCgFragmentProfile;
static CGprogram myCgFragmentProgram;
static CGparameter changeCoordMatrix;
static CGparameter globalAmbient; //环境光颜色
static CGparameter lightColor; //灯光颜色
static CGparameter lightPosition; //灯的位置
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
static float curOffset = 0;
static float stepOffset = 0.0005;
static float mGlobalAmbient[3] = {0.1, 0.1, 0.05};
static float mLightColor[3] = { 1, 1, 0.95 };

float projectionMatrix[16]; //投影矩阵


void OnDraw();
void ReShape(int width, int height);
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
void SetGoldMaterial();

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(300, 300);
	glutCreateWindow(frameTitle);
	glutDisplayFunc(&OnDraw);
	glutKeyboardFunc(&OnKeyBoard);
	glutReshapeFunc(ReShape);
	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
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
	GET_VERTEX_PROGRAM(globalAmbient);
	GET_VERTEX_PROGRAM(lightColor);
	GET_VERTEX_PROGRAM(lightPosition);
	GET_VERTEX_PROGRAM(eyePosition);
	GET_VERTEX_PROGRAM(Ke);
	GET_VERTEX_PROGRAM(Ka);
	GET_VERTEX_PROGRAM(Kd);
	GET_VERTEX_PROGRAM(Ks);
	GET_VERTEX_PROGRAM(shininess);

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
	CheckCgError("Get Named Parameter Error");
	glutMainLoop();
	return 0;
}

void SetGoldMaterial()
{
	const float mKe[3] = {0, 0, 0};//自发光
	const float mKa[3] = {0.3, 0.2, 0.01};//环境光系数
	const float mKd[3] = {0.78, 0.6, 0.1};//漫反射光系数
	const float mKs[3] = {0.95, 0.95, 0.85};//镜面射光系数
	const float mShininess = 28;
	cgSetParameter3fv(Ke, mKe);
	cgSetParameter3fv(Ka, mKa);
	cgSetParameter3fv(Kd, mKd);
	cgSetParameter3fv(Ks, mKs);
	cgSetParameter1f(shininess, mShininess);
}

void OnDraw()
{
	float translationMatrix[16], rotateMatrix[16], viewMatrix[16], finalMatrix[16], invMatrix[16];
	float tempPosition[4];
	float mLightPosition[4] = { 5, 3, 5 , 1 };
	float mEyePosition[4] = { 0, 0, 13 , 1};

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cgGLBindProgram(myCgVertexProgram);
	CheckCgError("Bind Vertex Program Error");
	cgGLEnableProfile(myCgVertexProfile);
	CheckCgError("Enable Vertex Profile Error");

	cgGLBindProgram(myCgFragmentProgram);
	CheckCgError("Bind Fragment Program Error");
	cgGLEnableProfile(myCgFragmentProfile);
	CheckCgError("Enable Fragment Profile Error");

	makeTranslateMatrix(-2, -1.5, 0, translationMatrix);
	makeRotateMatrix(60 , 1, 0, 0, rotateMatrix);
	//buildLookAtMatrix(sin(curOffset) * 13, 0, cos(curOffset) * 13, 0, 0, 0, 0, 1, 0, viewMatrix);
	buildLookAtMatrix(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 0, 0, 0, 1, 0, viewMatrix);
	multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	invertMatrix(invMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);

	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);
	cgSetParameter4fv(globalAmbient, mGlobalAmbient);
	cgSetParameter4fv(lightColor, mLightColor);
	
	transform(tempPosition, invMatrix, mLightPosition);
	cgSetParameter4fv(lightPosition, tempPosition);
	transform(tempPosition, invMatrix, mEyePosition);
	cgSetParameter4fv(eyePosition, tempPosition);
	SetGoldMaterial();
	glutSolidCone(1.5, 3.5, 20, 20);

	//makeTranslateMatrix(2, 0, 0, translationMatrix);
	//makeRotateMatrix(60, 1, 0, 0, rotateMatrix);
	//multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	//multMatrix(finalMatrix, viewMatrix, finalMatrix);
	//multMatrix(finalMatrix, projectionMatrix, finalMatrix);

	//cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	//glutSolidSphere(2.0, 30, 30);

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
	buildPerspectiveMatrix(fieldView, aspectRatio, 1.0, 20.0, projectionMatrix);
	glViewport(0, 0, width, height);
}

void PicMoving()
{
	//if (curOffset > 0.3 || curOffset < -0.3)
	//{
	//	stepOffset *= -1;
	//}
	curOffset += stepOffset;
	if (curOffset >= 2 * myPi)
	{
		curOffset -= 2 * myPi;
	}
	glutPostRedisplay();
}

void OnKeyBoard(unsigned char c, int x, int y)
{
	static bool animating = false;
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
			glutIdleFunc(PicMoving);
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