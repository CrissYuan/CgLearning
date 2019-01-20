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
static CGparameter lightColor[2]; //灯光颜色
static CGparameter lightPosition[2]; //灯的位置
static CGparameter Kc[2]; //灯的位置
static CGparameter Kl[2]; //灯的位置
static CGparameter Kq[2]; //灯的位置
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
static float stepOffset = 0.001;
static float mGlobalAmbient[3] = { 0.1, 0.1, 0.05 };
static float mLightColor[2][3] = { { 1, 1, 0.6 }, { 1, 0, 0.1 } };

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

	#define GET_FRAGMENT_PROGRAM_2(vsName, cgName) \
	vsName = \
	cgGetNamedParameter(myCgFragmentProgram, #cgName);\
	CheckCgError("Get Named "#cgName" Parameter Error");

	GET_FRAGMENT_PROGRAM_2(lightColor[0], lights[0].lightColor);
	GET_FRAGMENT_PROGRAM_2(lightColor[1], lights[1].lightColor); 
	GET_FRAGMENT_PROGRAM_2(lightPosition[0], lights[0].lightPosition);
	GET_FRAGMENT_PROGRAM_2(lightPosition[1], lights[1].lightPosition);
	GET_FRAGMENT_PROGRAM_2(Kc[0], lights[0].Kc);
	GET_FRAGMENT_PROGRAM_2(Kc[1], lights[1].Kc);
	GET_FRAGMENT_PROGRAM_2(Kl[0], lights[0].Kl);
	GET_FRAGMENT_PROGRAM_2(Kl[1], lights[1].Kl);
	GET_FRAGMENT_PROGRAM_2(Kq[0], lights[0].Kq);
	GET_FRAGMENT_PROGRAM_2(Kq[1], lights[1].Kq);
	GET_FRAGMENT_PROGRAM_2(Ke, material.Ke);
	GET_FRAGMENT_PROGRAM_2(Ka, material.Ka);
	GET_FRAGMENT_PROGRAM_2(Kd, material.Kd);
	GET_FRAGMENT_PROGRAM_2(Ks, material.Ks);
	GET_FRAGMENT_PROGRAM_2(shininess, material.shininess);

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

void SetRubineMaterial()
{
	const float mKe[3] = { 0, 0, 0 };//自发光
	const float mKa[3] = { 0.3, 0.2, 0.01 };//环境光系数
	const float mKd[3] = { 0.78, 0.1, 0.1 };//漫反射光系数
	const float mKs[3] = { 0.8, 0.5, 0.5 };//镜面射光系数
	const float mShininess = 30;
	cgSetParameter3fv(Ke, mKe);
	cgSetParameter3fv(Ka, mKa);
	cgSetParameter3fv(Kd, mKd);
	cgSetParameter3fv(Ks, mKs);
	cgSetParameter1f(shininess, mShininess);
}

void SetLightMaterial()
{
	const float zero[3] = { 0.0,0.0,0.0 };
	cgSetParameter3fv(Ke, mLightColor[0]);
	cgSetParameter3fv(Ka, zero);
	cgSetParameter3fv(Kd, zero);
	cgSetParameter3fv(Ks, zero);
	cgSetParameter1f(shininess, 0.0);
}

void SetLightMaterial2()
{
	const float zero[3] = { 0.0,0.0,0.0 };
	cgSetParameter3fv(Ke, mLightColor[1]);
	cgSetParameter3fv(Ka, zero);
	cgSetParameter3fv(Kd, zero);
	cgSetParameter3fv(Ks, zero);
	cgSetParameter1f(shininess, 0.0);
}

void SetWallMaterial()
{
	const float mKe[3] = { 0, 0, 0 };//自发光
	const float mKa[3] = { 0.1, 0.2, 0.3 };//环境光系数
	const float mKd[3] = { 0.2, 0.5, 0.8 };//漫反射光系数
	const float mKs[3] = { 0.1, 0.3, 0.9 };//镜面射光系数
	const float mShininess = 10;
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
	float mLightPosition[2][4] = { {5 * sin(curOffset), 1, 5 * cos(curOffset), 1}, {5 * sin(-curOffset), 2, 5 * cos(-curOffset), 1} };
	float mKc[2] = {0.3, 0.6};
	float mKl[2] = {0.1, 0.2};
	float mKq[2] = {0.03, 0.06};
	float mEyePosition[4] = { 0, 0, 20 , 1};

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
	makeRotateMatrix(60, 1, 0, 0, rotateMatrix);
	buildLookAtMatrix(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 0, 0, 0, 1, 0, viewMatrix);
	multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	invertMatrix(invMatrix, finalMatrix);
	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);

	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);
	cgSetParameter4fv(globalAmbient, mGlobalAmbient);
	cgSetParameter3fv(lightColor[0], mLightColor[0]);
	cgSetParameter3fv(lightColor[1], mLightColor[1]);

	cgSetParameter1f(Kc[0], mKc[0]);
	cgSetParameter1f(Kc[1], mKc[0]);
	cgSetParameter1f(Kl[0], mKl[0]);
	cgSetParameter1f(Kl[1], mKl[0]);
	cgSetParameter1f(Kq[0], mKq[0]);
	cgSetParameter1f(Kq[1], mKq[0]);
	
	transform(tempPosition, invMatrix, mLightPosition[0]);
	cgSetParameter4fv(lightPosition[0], tempPosition);
	transform(tempPosition, invMatrix, mLightPosition[1]);
	cgSetParameter4fv(lightPosition[1], tempPosition);
	transform(tempPosition, invMatrix, mEyePosition);
	cgSetParameter4fv(eyePosition, tempPosition);

	SetGoldMaterial();
	glutSolidCone(1.5, 3.5, 20, 20);

	makeTranslateMatrix(2, 0, 0, translationMatrix);
	makeRotateMatrix(60, 1, 0, 0, rotateMatrix);
	multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	invertMatrix(invMatrix, finalMatrix);
	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);
	transform(tempPosition, invMatrix, mLightPosition[0]);
	cgSetParameter4fv(lightPosition[0], tempPosition);
	transform(tempPosition, invMatrix, mLightPosition[1]);
	cgSetParameter4fv(lightPosition[1], tempPosition);
	transform(tempPosition, invMatrix, mEyePosition);
	cgSetParameter4fv(eyePosition, tempPosition);

	SetRubineMaterial();
	glutSolidSphere(2.0, 40, 40);


	cgSetParameter3f(lightPosition[0], 0, 0, 0);
	makeTranslateMatrix(mLightPosition[0][0], mLightPosition[0][1], mLightPosition[0][2], translationMatrix);
	
	multMatrix(finalMatrix, viewMatrix, translationMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	SetLightMaterial();
	glutSolidSphere(0.2, 12, 12);

	cgSetParameter3f(lightPosition[1], 0, 0, 0);
	makeTranslateMatrix(mLightPosition[1][0], mLightPosition[1][1], mLightPosition[1][2], translationMatrix);

	multMatrix(finalMatrix, viewMatrix, translationMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	SetLightMaterial2();
	glutSolidSphere(0.2, 12, 12);


	multMatrix(finalMatrix, projectionMatrix, viewMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);
	cgSetParameter3fv(lightPosition[0], mLightPosition[0]);
	cgSetParameter3fv(lightPosition[1], mLightPosition[1]);
	SetWallMaterial();
	glBegin(GL_QUADS);
		glNormal3f(0, 1, 0);
		glVertex3f( 12, -3, -12);
		glVertex3f(-12, -3, -12);
		glVertex3f(-12, -3, 12);
		glVertex3f( 12, -3, 12);

		glNormal3f(0, 0, 1);
		glVertex3f( 12, -12, -8);
		glVertex3f(-12, -12, -8);
		glVertex3f(-12,  12, -8);
		glVertex3f( 12,  12, -8);

		glNormal3f(0, -1, 0);
		glVertex3f( 12, 5, -12);
		glVertex3f(-12, 5, -12);
		glVertex3f(-12, 5, 12);
		glVertex3f( 12, 5, 12);

		glNormal3f(1, 0, 0);
		glVertex3f(-6,  12, -12);
		glVertex3f(-6, -12, -12);
		glVertex3f(-6, -12, 12);
		glVertex3f(-6,  12, 12);

		glNormal3f(-1, 0, 0);
		glVertex3f(6, 12, -12);
		glVertex3f(6, -12, -12);
		glVertex3f(6, -12, 12);
		glVertex3f(6, 12, 12);
	glEnd();

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
	buildPerspectiveMatrix(fieldView, aspectRatio, 1.0, 50.0, projectionMatrix);
	glViewport(0, 0, width, height);
}

void PicMoving()
{
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