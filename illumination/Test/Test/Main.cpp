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
static CGparameter finalColor;
static const char* frameTitle = "Cg Test";
static const char* cgVFileName = "VertexCG.cg";
static const char* cgVFuncName = "VertexMain";
static const char* cgFFileName = "FragmentCG.cg";
static const char* cgFFuncName = "TextureMain";
static GLubyte myTexture[3 * 512 * 512];
static const char* textureDir = ".//cat.bmp";
static float curOffset = 0;
static float stepOffset = 0.005;
float projectionMatrix[16]; //投影矩阵


void OnDraw();
void ReShape(int width, int height);
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
void LoadBMP(const char* Filename);

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
	changeCoordMatrix = cgGetNamedParameter(myCgVertexProgram, "changeCoordMatrix");
	CheckCgError("Get Named Parameter Error");

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
	finalColor = cgGetNamedParameter(myCgFragmentProgram, "iColor");
	CheckCgError("Get Named Parameter Error");
	glutMainLoop();
	return 0;
}

void OnDraw()
{
	float translationMatrix[16], rotateMatrix[16], viewMatrix[16], finalMatrix[16];
	
	glClear(GL_COLOR_BUFFER_BIT);
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
	buildLookAtMatrix(sin(curOffset) * 13, 0, cos(curOffset) * 13, 0, 0, 0, 0, 1, 0, viewMatrix);
	multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);

	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	cgSetParameter4f(finalColor, 1, 0.96, 0.5, 1);

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	glutWireCone(1.5, 3.5, 20, 20);

	makeTranslateMatrix(2, 0, 0, translationMatrix);
	makeRotateMatrix(60, 1, 0, 0, rotateMatrix);
	multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);

	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	cgSetParameter4f(finalColor, 1, 0, 0, 1);

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	glutWireSphere(2.0, 30, 30);

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

void LoadBMP(const char* Filename) //加载.bmp纹理方法
{
	FILE *file = fopen(Filename, "rb");
	if (not file) return;
	fread(myTexture, sizeof(unsigned char), 54, file); //前54位为.bmp头结构
	fread(myTexture, sizeof(unsigned char), 3 * 512 * 512, file);

	for (int i = 0; i < 3 * 512 * 512; i += 3) {
		myTexture[i] ^= myTexture[i + 2];
		myTexture[i + 2] ^= myTexture[i];
		myTexture[i] ^= myTexture[i + 2];
	}
	fclose(file);
}