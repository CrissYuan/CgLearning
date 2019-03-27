#include <stdio.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glut.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "matrix.h"
#include "MonkeyHead.h"

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgGL.lib")
static CGcontext myCgContext;
static CGprofile myCgVertexProfile;
static CGprogram myCgVertexProgram;
static CGprofile myCgFragmentProfile;
static CGprogram myCgFragmentProgram;
static CGparameter changeCoordMatrix;
static CGparameter modelToWroldCoordMatrix;
static CGparameter eyePosition;
static CGparameter reflectivity;
static CGparameter decalMap;
static CGparameter environmentMap;

static const char* frameTitle = "Cg Test";
static const char* cgVFileName = "VertexCG.cg";
static const char* cgVFuncName = "VertexMain";
static const char* cgFFileName = "FragmentCG.cg";
static const char* cgFFuncName = "TextureMain";
static float curOffset = 0;
static float stepOffset = 0.001;
static float mReflectivity = 0.50f;
static bool  moving = false;
static int   beginx, beginy;
static float eyeAngle = 0;
static float eyeHeight = 0;

float projectionMatrix[16]; //Í¶Ó°¾ØÕó


void OnDraw();
void ReShape(int width, int height);
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
void DrawMonkeyHead();
void OnMousePressed(int button, int state, int x, int y);
void OnMouseDrag(int x, int y);
float DiffuseRamp(float x);
float SpecularRamp(float x);
float EdgeRamp(float x);

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(512, 512);
	glutCreateWindow(frameTitle);
	glutDisplayFunc(&OnDraw);
	glutKeyboardFunc(&OnKeyBoard);
	glutReshapeFunc(&ReShape);
	glutMouseFunc(&OnMousePressed);
	glutMotionFunc(&OnMouseDrag);
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "failed to initalize GLEW.\n");
		exit(1);
	}
	glClearColor(0.5, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
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
	GET_VERTEX_PROGRAM(modelToWroldCoordMatrix);
	GET_VERTEX_PROGRAM(eyePosition);

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
	GET_FRAGMENT_PROGRAM(reflectivity);
	CheckCgError("Get Named Parameter Error");


	glutMainLoop();
	return 0;
}

void OnDraw()
{
	float translationMatrix[16], rotateMatrix[16], viewMatrix[16], finalMatrix[16];
	float mEyePosition[4] = { 6 * sin(eyeAngle), eyeHeight, 6 * cos(eyeAngle) , 1};

	cgSetParameter1f(reflectivity, mReflectivity);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cgGLBindProgram(myCgVertexProgram);
	CheckCgError("Bind Vertex Program Error");
	cgGLEnableProfile(myCgVertexProfile);
	CheckCgError("Enable Vertex Profile Error");

	cgGLBindProgram(myCgFragmentProgram);
	CheckCgError("Bind Fragment Program Error");
	cgGLEnableProfile(myCgFragmentProfile);
	CheckCgError("Enable Fragment Profile Error");

	buildLookAtMatrix(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 0, 0, 0, 1, 0, viewMatrix);

	cgSetParameter3fv(eyePosition, mEyePosition);

	makeTranslateMatrix(0, 0, 0, translationMatrix);
	makeRotateMatrix(curOffset*30, 0, 1, 0, rotateMatrix);
	multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	cgSetMatrixParameterfr(modelToWroldCoordMatrix, finalMatrix);
	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	//glutSolidSphere(1.5, 40, 40);
	DrawMonkeyHead();

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
	double fieldView = 40;
	buildPerspectiveMatrix(fieldView, aspectRatio, 1.0, 50.0, projectionMatrix);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fieldView, aspectRatio, 1.0, 50.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, width, height);
}

void PicMoving()
{
	curOffset += stepOffset;
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

void OnMousePressed(int button, int state, int x, int y)
{
	const int btn = GLUT_LEFT_BUTTON;
	if (btn == button && state == GLUT_DOWN)
	{
		moving = true;
		beginx = x;
		beginy = y;
	}
	if (btn == button && state == GLUT_UP)
	{
		moving = false;
	}
}

void OnMouseDrag(int x, int y)
{
	const float bound = 8;
	if (moving)
	{
		eyeAngle += (beginx - x) * 0.005;
		eyeHeight -= (beginy - y) * 0.005;
		if (eyeHeight > bound)
		{
			eyeHeight = bound;
		}
		else if (eyeHeight < -bound)
		{
			eyeHeight = -bound;
		}
		beginx = x;
		beginy = y;
		glutPostRedisplay();
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

void DrawMonkeyHead()
{
	static GLfloat *texcoords = NULL;
	if (texcoords == NULL)
	{
		const int numVertices = sizeof(MonkeyHead_vertices) / (3 * sizeof(MonkeyHead_vertices[0]));
		const float scaleFactor = 1.5;
		texcoords = (GLfloat *)malloc(2 * numVertices * sizeof(GLfloat));
		if (texcoords == NULL)
		{
			fprintf(stderr, "%s: malloc failed.\n", frameTitle);
			exit(1);
		}
		for (int i = 0; i < numVertices; i++)
		{
			texcoords[i * 2 + 0] = scaleFactor * texcoords[i * 3 + 0];
			texcoords[i * 2 + 1] = scaleFactor * texcoords[i * 3 + 1];
		}
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), MonkeyHead_vertices);
	glNormalPointer(GL_FLOAT, 3 * sizeof(GLfloat), MonkeyHead_vertices);
	glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), MonkeyHead_vertices);
	glDrawElements(GL_TRIANGLES, 3 * MonkeyHead_num_of_triangles, GL_UNSIGNED_SHORT, MonkeyHead_triangles);
}

float DiffuseRamp(float x)
{
	if (x > 0.5)
	{
		return x * x * (3 - 2 * x);
	}
	else
	{
		return 0.5;
	}
}

float SpecularRamp(float x)
{
	if (x > 0.2)
	{
		return x;
	}
	else
	{
		return 0;
	}
}

float EdgeRamp(float x)
{
	if (x < 0.2)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void LoadRamp()
{

}