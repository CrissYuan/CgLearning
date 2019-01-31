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
//static CGparameter globalAmbient; //��������ɫ
//static CGparameter lightColor[2]; //�ƹ���ɫ
//static CGparameter lightPosition[2]; //�Ƶ�λ��
//static CGparameter Kc[2];
//static CGparameter Kl[2];
//static CGparameter Kq[2];
//static CGparameter direction[2]; //�Ƶ�λ��
//static CGparameter cosInnerCone[2]; //�Ƶ�λ��
//static CGparameter cosOuterCone[2]; //�Ƶ�λ��
//static CGparameter eyePosition;  //�۾���λ��
//static CGparameter Ke; //����������ɫ
//static CGparameter Ka; //������ϵ��
//static CGparameter Kd; //�������ϵ��
//static CGparameter Ks; //�������ϵ��
//static CGparameter shininess; //���ʹ⻬�̶�
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
static float mGlobalAmbient[3] = { 0.1, 0.1, 0.05 };
static float mLightColor[2][3] = { { 1, 1, 1 }, { 1, 1, 1 } };

float projectionMatrix[16]; //ͶӰ����


void OnDraw();
void ReShape(int width, int height);
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
//void SetGoldMaterial();

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
	//GET_FRAGMENT_PROGRAM(globalAmbient);
	//GET_FRAGMENT_PROGRAM(eyePosition);
	GET_FRAGMENT_PROGRAM(scale);
	cgSetParameter2f(scale, 1.0 / m_knightModel->header.skinWidth, 1.0 / m_knightModel->header.skinHeight);

	//#define GET_FRAGMENT_PROGRAM_2(vsName, cgName) \
	//vsName = \
	//cgGetNamedParameter(myCgFragmentProgram, #cgName);\
	//CheckCgError("Get Named "#cgName" Parameter Error");

	//GET_FRAGMENT_PROGRAM_2(lightColor[0], lights[0].lightColor);
	//GET_FRAGMENT_PROGRAM_2(lightColor[1], lights[1].lightColor); 
	//GET_FRAGMENT_PROGRAM_2(lightPosition[0], lights[0].lightPosition);
	//GET_FRAGMENT_PROGRAM_2(lightPosition[1], lights[1].lightPosition);
	//GET_FRAGMENT_PROGRAM_2(Kc[0], lights[0].Kc);
	//GET_FRAGMENT_PROGRAM_2(Kc[1], lights[1].Kc);
	//GET_FRAGMENT_PROGRAM_2(Kl[0], lights[0].Kl);
	//GET_FRAGMENT_PROGRAM_2(Kl[1], lights[1].Kl);
	//GET_FRAGMENT_PROGRAM_2(Kq[0], lights[0].Kq);
	//GET_FRAGMENT_PROGRAM_2(Kq[1], lights[1].Kq);
	//GET_FRAGMENT_PROGRAM_2(direction[0], lights[0].direction);
	//GET_FRAGMENT_PROGRAM_2(direction[1], lights[1].direction);
	//GET_FRAGMENT_PROGRAM_2(cosInnerCone[0], lights[0].cosInnerCone);
	//GET_FRAGMENT_PROGRAM_2(cosInnerCone[1], lights[1].cosInnerCone);
	//GET_FRAGMENT_PROGRAM_2(cosOuterCone[0], lights[0].cosOuterCone);
	//GET_FRAGMENT_PROGRAM_2(cosOuterCone[1], lights[1].cosOuterCone);
	//GET_FRAGMENT_PROGRAM_2(Ke, material.Ke);
	//GET_FRAGMENT_PROGRAM_2(Ka, material.Ka);
	//GET_FRAGMENT_PROGRAM_2(Kd, material.Kd);
	//GET_FRAGMENT_PROGRAM_2(Ks, material.Ks);
	//GET_FRAGMENT_PROGRAM_2(shininess, material.shininess);

	CheckCgError("Get Named Parameter Error");
	glutMainLoop();
	return 0;
}

//void SetGoldMaterial()
//{
//	const float mKe[3] = {0, 0, 0};//�Է���
//	const float mKa[3] = {0.3, 0.2, 0.01};//������ϵ��
//	const float mKd[3] = {0.78, 0.6, 0.1};//�������ϵ��
//	const float mKs[3] = {0.95, 0.95, 0.85};//�������ϵ��
//	const float mShininess = 28;
//	cgSetParameter3fv(Ke, mKe);
//	cgSetParameter3fv(Ka, mKa);
//	cgSetParameter3fv(Kd, mKd);
//	cgSetParameter3fv(Ks, mKs);
//	cgSetParameter1f(shininess, mShininess);
//}

//void SetRubineMaterial()
//{
//	const float mKe[3] = { 0, 0, 0 };//�Է���
//	const float mKa[3] = { 0.3, 0.2, 0.01 };//������ϵ��
//	const float mKd[3] = { 0.78, 0.1, 0.1 };//�������ϵ��
//	const float mKs[3] = { 0.8, 0.5, 0.5 };//�������ϵ��
//	const float mShininess = 30;
//	cgSetParameter3fv(Ke, mKe);
//	cgSetParameter3fv(Ka, mKa);
//	cgSetParameter3fv(Kd, mKd);
//	cgSetParameter3fv(Ks, mKs);
//	cgSetParameter1f(shininess, mShininess);
//}

//void SetLightMaterial()
//{
//	const float zero[3] = { 0.0,0.0,0.0 };
//	cgSetParameter3fv(Ke, mLightColor[0]);
//	cgSetParameter3fv(Ka, zero);
//	cgSetParameter3fv(Kd, zero);
//	cgSetParameter3fv(Ks, zero);
//	cgSetParameter1f(shininess, 0.0);
//}

//void SetLightMaterial2()
//{
//	const float zero[3] = { 0.0,0.0,0.0 };
//	cgSetParameter3fv(Ke, mLightColor[1]);
//	cgSetParameter3fv(Ka, zero);
//	cgSetParameter3fv(Kd, zero);
//	cgSetParameter3fv(Ks, zero);
//	cgSetParameter1f(shininess, 0.0);
//}

//void SetWallMaterial()
//{
//	const float mKe[3] = { 0, 0, 0 };//�Է���
//	const float mKa[3] = { 0.1, 0.2, 0.3 };//������ϵ��
//	const float mKd[3] = { 0.2, 0.5, 0.8 };//�������ϵ��
//	const float mKs[3] = { 0.1, 0.3, 0.9 };//�������ϵ��
//	const float mShininess = 10;
//	cgSetParameter3fv(Ke, mKe);
//	cgSetParameter3fv(Ka, mKa);
//	cgSetParameter3fv(Kd, mKd);
//	cgSetParameter3fv(Ks, mKs);
//	cgSetParameter1f(shininess, mShininess);
//}

float AddDelta(float knightKnob, float detal, int numFrames)
{
	knightKnob += detal;
	while (numFrames <= knightKnob)
	{
		knightKnob -= numFrames;
		if (knightKnob < 0)
			knightKnob = 0;
	}
	return knightKnob;
}

void OnDraw()
{
	float eyeRadius = 85;
	frameA = floor(knightKnob);
	frameB = floor(AddDelta(knightKnob, 1 ,m_knightModel->header.numFrames));
	float m_EyePosition[3] = { sin(curOffset) * eyeRadius, 0, cos(curOffset) * eyeRadius };
	float translationMatrix[16], rotateMatrix[16], viewMatrix[16], finalMatrix[16], invMatrix[16];
	//float tempPosition[4];
	//float temp3Position[3];
	//float mLightPosition[2][4] = { {5 * sin(curOffset), 1, 5 * cos(curOffset), 1}, {5 * sin(-curOffset), 2, 5 * cos(-curOffset), 1} };
	//float mKc[2] = {0.3, 0.6};
	//float mKl[2] = {0.1, 0.2};
	//float mKq[2] = {0.03, 0.06};
	//float mEyePosition[4] = { 0, 0, 20 , 1};
	//float mDirection[2][3] = {
	//	{-mLightPosition[0][0], -mLightPosition[0][1], -mLightPosition[0][2]},
	//	{-mLightPosition[1][0], -mLightPosition[1][1], -mLightPosition[1][2]}
	//};

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//cgSetParameter1f(cosInnerCone[0], 0.9);
	//cgSetParameter1f(cosInnerCone[1], 0.9);
	//cgSetParameter1f(cosOuterCone[0], 0.8);
	//cgSetParameter1f(cosOuterCone[1], 0.8);

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

	drawMD2render(m_knightRender, frameA, frameB);
	//makeTranslateMatrix(-2, -1.5, 0, translationMatrix);
	//makeRotateMatrix(60, 1, 0, 0, rotateMatrix);
	//buildLookAtMatrix(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 0, 0, 0, 1, 0, viewMatrix);
	//multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	//invertMatrix(invMatrix, finalMatrix);
	//multMatrix(finalMatrix, viewMatrix, finalMatrix);
	//multMatrix(finalMatrix, projectionMatrix, finalMatrix);

	//cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);
	//cgSetParameter4fv(globalAmbient, mGlobalAmbient);
	//cgSetParameter3fv(lightColor[0], mLightColor[0]);
	//cgSetParameter3fv(lightColor[1], mLightColor[1]);

	//cgSetParameter1f(Kc[0], mKc[0]);
	//cgSetParameter1f(Kc[1], mKc[0]);
	//cgSetParameter1f(Kl[0], mKl[0]);
	//cgSetParameter1f(Kl[1], mKl[0]);
	//cgSetParameter1f(Kq[0], mKq[0]);
	//cgSetParameter1f(Kq[1], mKq[0]);
	
	//transform(tempPosition, invMatrix, mLightPosition[0]);
	//cgSetParameter4fv(lightPosition[0], tempPosition);
	//transform(tempPosition, invMatrix, mLightPosition[1]);
	//cgSetParameter4fv(lightPosition[1], tempPosition);
	//transform(tempPosition, invMatrix, mEyePosition);
	//cgSetParameter4fv(eyePosition, tempPosition);
	//transformDirection(temp3Position, invMatrix, mDirection[0]);
	//normalizeVector(temp3Position);
	//cgSetParameter3fv(direction[0], temp3Position);
	//transformDirection(temp3Position, invMatrix, mDirection[1]);
	//normalizeVector(temp3Position);
	//cgSetParameter3fv(direction[1], temp3Position);

	//SetGoldMaterial();
	//glutSolidCone(1.5, 3.5, 20, 20);

	//makeTranslateMatrix(2, 0, 0, translationMatrix);
	//makeRotateMatrix(60, 1, 0, 0, rotateMatrix);
	//multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	//invertMatrix(invMatrix, finalMatrix);
	//multMatrix(finalMatrix, viewMatrix, finalMatrix);
	//multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	//cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);
	//transform(tempPosition, invMatrix, mLightPosition[0]);
	//cgSetParameter4fv(lightPosition[0], tempPosition);
	//transform(tempPosition, invMatrix, mLightPosition[1]);
	//cgSetParameter4fv(lightPosition[1], tempPosition);
	//transform(tempPosition, invMatrix, mEyePosition);
	//cgSetParameter4fv(eyePosition, tempPosition);
	//transformDirection(temp3Position, invMatrix, mDirection[0]);
	//normalizeVector(temp3Position);
	//cgSetParameter3fv(direction[0], temp3Position);
	//transformDirection(temp3Position, invMatrix, mDirection[1]);
	//normalizeVector(temp3Position);
	//cgSetParameter3fv(direction[1], temp3Position);

	//SetRubineMaterial();
	//glutSolidSphere(2.0, 40, 40);


	//multMatrix(finalMatrix, projectionMatrix, viewMatrix);
	//cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);
	//cgSetParameter3fv(lightPosition[0], mLightPosition[0]);
	//cgSetParameter3fv(lightPosition[1], mLightPosition[1]);
	//normalizeVector(mDirection[0]);
	//cgSetParameter3fv(direction[0], mDirection[0]);
	//normalizeVector(mDirection[1]);
	//cgSetParameter3fv(direction[1], mDirection[1]);
	//SetWallMaterial();
	//glBegin(GL_QUADS);
	//	glNormal3f(0, 1, 0);
	//	glVertex3f( 12, -3, -12);
	//	glVertex3f(-12, -3, -12);
	//	glVertex3f(-12, -3, 12);
	//	glVertex3f( 12, -3, 12);

	//	glNormal3f(0, 0, 1);
	//	glVertex3f( 12, -12, -8);
	//	glVertex3f(-12, -12, -8);
	//	glVertex3f(-12,  12, -8);
	//	glVertex3f( 12,  12, -8);

	//	glNormal3f(0, -1, 0);
	//	glVertex3f( 12, 5, -12);
	//	glVertex3f(-12, 5, -12);
	//	glVertex3f(-12, 5, 12);
	//	glVertex3f( 12, 5, 12);

	//	glNormal3f(1, 0, 0);
	//	glVertex3f(-6,  12, -12);
	//	glVertex3f(-6, -12, -12);
	//	glVertex3f(-6, -12, 12);
	//	glVertex3f(-6,  12, 12);

	//	glNormal3f(-1, 0, 0);
	//	glVertex3f(6, 12, -12);
	//	glVertex3f(6, -12, -12);
	//	glVertex3f(6, -12, 12);
	//	glVertex3f(6, 12, 12);
	//glEnd();


	//cgSetParameter3f(lightPosition[0], 0, 0, 0);
	//makeTranslateMatrix(mLightPosition[0][0], mLightPosition[0][1], mLightPosition[0][2], translationMatrix);

	//multMatrix(finalMatrix, viewMatrix, translationMatrix);
	//multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	//cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	//SetLightMaterial();
	//glutSolidSphere(0.2, 12, 12);

	//cgSetParameter3f(lightPosition[1], 0, 0, 0);
	//makeTranslateMatrix(mLightPosition[1][0], mLightPosition[1][1], mLightPosition[1][2], translationMatrix);

	//multMatrix(finalMatrix, viewMatrix, translationMatrix);
	//multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	//cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	//SetLightMaterial2();
	//glutSolidSphere(0.2, 12, 12);

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

static int lastTime;
void PicMoving()
{
	const float numSystem = 1000.0;
	const int frameNumPerSec = 10;
	int nowTime = glutGet(GLUT_ELAPSED_TIME);
	float detalTime = (nowTime - lastTime) / frameNumPerSec;
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