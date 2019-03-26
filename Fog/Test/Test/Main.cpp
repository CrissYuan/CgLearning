#include <stdio.h>
#include <GL/glut.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
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
static CGparameter textureParameter;
static const char* frameTitle = "Cg Test";
static const char* cgVFileName = "VertexCG.cg";
static const char* cgVFuncName = "VertexMain";
static const char* cgFFileName = "FragmentCG.cg";
static const char* cgFFuncName = "FragmentMain";

static float projectionMatrix[16];
static float eyeHeight = 0;
static float eyeAngle = 0;
static int beginx, beginy;
static int moving = 0;
static GLubyte myTexture[3 * 512 * 512];
static const char* textureDir = ".//cat.bmp";


void OnDraw();
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
void LoadBMP(const char* Filename);
void ReShape(int width, int height);
void OnMouseMotion(int x, int y);
void OnMouse(int button, int state, int x, int y);

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(300, 300);
	glutCreateWindow(frameTitle);
	glutDisplayFunc(&OnDraw);
	glutKeyboardFunc(&OnKeyBoard);
	glutReshapeFunc(&ReShape);
	glutMouseFunc(&OnMouse);
	glutMotionFunc(&OnMouseMotion);
	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, 31); //绑定纹理编号为31
	LoadBMP(textureDir);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB8,512 ,512, GL_RGB, GL_UNSIGNED_BYTE,myTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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
	textureParameter = cgGetNamedParameter(myCgFragmentProgram, "pic");
	CheckCgError("Get Named Parameter Error");
	cgGLSetTextureParameter(textureParameter, 31); //31为纹理编号
	CheckCgError("Set Texture Parameter Error");
	glutMainLoop();
	return 0;
}

void OnDraw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cgGLBindProgram(myCgVertexProgram);
	CheckCgError("Bind Vertex Program Error");
	cgGLEnableProfile(myCgVertexProfile);
	CheckCgError("Enable Vertex Profile Error");

	cgGLBindProgram(myCgFragmentProgram);
	CheckCgError("Bind Fragment Program Error");
	cgGLEnableProfile(myCgFragmentProfile);
	CheckCgError("Enable Fragment Profile Error");

	float translateMatrix[16], rotateMatrix[16], viewMatrix[16], finalMatrix[16];
	float eyeRadius = 7;
	float mEyePosition[4] = { eyeRadius * sin(eyeAngle), eyeHeight, eyeRadius * cos(eyeAngle) , 1 };

	makeLookAtMatrix(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 0, 0, 0, 1, 0, viewMatrix);
	makeTranslateMatrix(0, 0, 0, translateMatrix);
	makeRotateMatrix(0, 0, 1, 0, rotateMatrix);
	multMatrix(finalMatrix, translateMatrix, rotateMatrix);
	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	glBegin(GL_TRIANGLES);
		glTexCoord2f(0.0, 1.0);
		glVertex2f(-0.8, 0.8);
		glTexCoord2f(1.0, 1.0);
		glVertex2f(0.8, 0.8);
		glTexCoord2f(0.5, 0.0);
		glVertex2f(0.0, -0.8);
	glEnd();
	cgGLDisableProfile(myCgVertexProfile);
	CheckCgError("Disable Vertex Profile Error");

	cgGLDisableProfile(myCgFragmentProfile);
	CheckCgError("Disable Fragment Profile Error");

	cgGLDisableTextureParameter(textureParameter);
	CheckCgError("Disable Texture Parameter Error");
	glutSwapBuffers();
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

void ReShape(int width, int height)
{
	double aspectRatio = (float)width / (float)height;
	double fieldOfView = 40;
	makePerspectiveMatrix(fieldOfView, aspectRatio, 0.1, 500.0, projectionMatrix);
	glViewport(0, 0, width, height);
}

void OnMouseMotion(int x, int y)
{
	const float minHeight = 15,
		maxHeight = 95;

	if (moving) {
		eyeAngle += 0.01*(beginx - x);
		eyeHeight += 0.0004*(beginy - y);
		if (eyeHeight > maxHeight) {
			eyeHeight = maxHeight;
		}
		if (eyeHeight < minHeight) {
			eyeHeight = minHeight;
		}
		beginx = x;
		beginy = y;
		glutPostRedisplay();
	}
}

void OnMouse(int button, int state, int x, int y)
{
	const int spinButton = GLUT_LEFT_BUTTON;

	if (button == spinButton && state == GLUT_DOWN) {
		moving = 1;
		beginx = x;
		beginy = y;
	}
	if (button == spinButton && state == GLUT_UP) {
		moving = 0;
	}
}
