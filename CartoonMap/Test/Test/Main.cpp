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
static CGparameter eyePosition;
static CGparameter lightPosition;
static CGparameter shininess;
static CGparameter Kd;
static CGparameter Ks;

static const char* frameTitle = "Cg Test";
static const char* cgVFileName = "VertexCG.cg";
static const char* cgVFuncName = "VertexMain";
static const char* cgFFileName = "FragmentCG.cg";
static const char* cgFFuncName = "TextureMain";
static float curOffset = 0;
static float stepOffset = 0.001;

static bool  moving = false;
static int   beginx, beginy;
static bool  lmoving = false;
static int   lbeginx, lbeginy;

static float mEyeAngle = 0;
static float mEyeHeight = 0;
static float mLightAngle = 0;
static float mLightHeight = 1;
static float mShininess = 9;
static float mKd[4] = { 0.15, 0.18, 0.33, 0.1 };
static float mKs[4] = { 0.66, 0.73, 1.00, 0.0 };

enum
{
	TO_DIFFUSE_RAMP = 1,
	TO_SPECULAR_RAMP,
	TO_EDGE_RAMP
};

float projectionMatrix[16]; //Õ∂”∞æÿ’Û


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
void LoadRamp(GLuint texIndex, int size, float(*func)(float x));

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
	glClearColor(0.41, 0.07, 0.14, 0);
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
	GET_FRAGMENT_PROGRAM(eyePosition);
	GET_FRAGMENT_PROGRAM(lightPosition);
	GET_FRAGMENT_PROGRAM(shininess);
	GET_FRAGMENT_PROGRAM(Kd);
	GET_FRAGMENT_PROGRAM(Ks);

	cgSetParameter1f(shininess, mShininess);

	cgSetParameter4fv(Kd, mKd);
	cgSetParameter4fv(Ks, mKs);
	for (int i = 0; i < 3; i++)
	{
		static GLuint texIndex[3] = { TO_DIFFUSE_RAMP, TO_SPECULAR_RAMP,TO_EDGE_RAMP };
		static const char* parameterName[3] = { "diffuseRamp", "specularRamp", "edgeRamp" };
		static float(*func[3]) (float x) = { DiffuseRamp, SpecularRamp, EdgeRamp };
		CGparameter sampler;
		sampler = cgGetNamedParameter(myCgFragmentProgram, parameterName[i]);
		LoadRamp(texIndex[i], 256, func[i]);
		cgGLSetTextureParameter(sampler, texIndex[i]);
	}
	CheckCgError("Get Named Parameter Error");

	glutMainLoop();
	return 0;
}

void LoadMVP(const float modelView[16])
{
	float target[16];
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			target[i * 4 + j] = modelView[j * 4 + i];
		}
	}
	glLoadMatrixf(target);
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

	float translationMatrix[16], rotateMatrix[16], viewMatrix[16], finalMatrix[16], modelViewMatrix[16];
	float mEyePosition[4] = { 8 * sin(mEyeAngle), mEyeHeight, 8 * cos(mEyeAngle) , 1 };
	float mLightPosition[4] = { 3.5 * sin(mLightAngle), mLightHeight, 3.5 * cos(mLightAngle) , 1 };

	buildLookAtMatrix(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 0, 0, 0, 1, 0, viewMatrix);

	cgSetParameter3fv(eyePosition, mEyePosition);
	cgSetParameter3fv(lightPosition, mLightPosition);

	makeTranslateMatrix(0, 0, 0, translationMatrix);
	makeRotateMatrix(curOffset*30, 0, 1, 0, rotateMatrix);
	multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	multMatrix(modelViewMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, modelViewMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	DrawMonkeyHead();

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	cgGLDisableProfile(myCgVertexProfile);
	CheckCgError("Disable Vertex Profile Error");

	cgGLDisableProfile(myCgFragmentProfile);
	CheckCgError("Disable Fragment Profile Error");

	glPushMatrix();
	LoadMVP(modelViewMatrix);
	glTranslatef(mLightPosition[0]*0.5, mLightPosition[1]*0.5, mLightPosition[2]*0.5);
	glColor3f(1, 1, 0.5);
	glutSolidSphere(0.05, 10, 10);
	glColor3f(1, 1, 1);
	glPopMatrix();

	glutSwapBuffers();
}

void ReShape(int width, int height)
{
	double aspectRatio = (float)width / (float)height;
	double fieldView = 40;
	buildPerspectiveMatrix(fieldView, aspectRatio, 0.1, 50.0, projectionMatrix);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fieldView, aspectRatio, 0.1, 50.0);
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
	case '+':
		mShininess *= 1.05;
		printf("mShininess = %f\n", mShininess);
		cgSetParameter1f(shininess, mShininess);
		glutPostRedisplay();
		break;
	case '-':
		mShininess /= 1.05;
		printf("mShininess = %f\n", mShininess);
		cgSetParameter1f(shininess, mShininess);
		glutPostRedisplay();
		break;
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
	const int leftBtn = GLUT_LEFT_BUTTON;
	const int midBtn = GLUT_MIDDLE_BUTTON;
	if (leftBtn == button && state == GLUT_DOWN)
	{
		moving = true;
		beginx = x;
		beginy = y;
	}
	if (leftBtn == button && state == GLUT_UP)
	{
		moving = false;
	}

	if (midBtn == button && state == GLUT_DOWN)
	{
		lmoving = true;
		lbeginx = x;
		lbeginy = y;
	}
	if (midBtn == button && state == GLUT_UP)
	{
		lmoving = false;
	}
}

void OnMouseDrag(int x, int y)
{
	const float bound = 8;
	if (moving)
	{
		mEyeAngle += (beginx - x) * 0.005;
		mEyeHeight -= (beginy - y) * 0.005;
		if (mEyeHeight > bound)
		{
			mEyeHeight = bound;
		}
		else if (mEyeHeight < -bound)
		{
			mEyeHeight = -bound;
		}
		beginx = x;
		beginy = y;
		glutPostRedisplay();
	}
	if (lmoving)
	{
		mLightAngle += (lbeginx - x) * 0.005;
		mLightHeight -= (lbeginy - y) * 0.005;
		if (mLightHeight > bound)
		{
			mLightHeight = bound;
		}
		else if (mLightHeight < -bound)
		{
			mLightHeight = -bound;
		}
		lbeginx = x;
		lbeginy = y;
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
	glNormalPointer(GL_FLOAT, 3 * sizeof(GLfloat), MonkeyHead_normals);
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

void LoadRamp(GLuint texIndex, int size, float (*func)(float x))
{
	int bytesForRamp = size * sizeof(float);
	float *ramp = (float*)malloc(bytesForRamp);
	if (NULL == ramp)
	{
		fprintf(stderr, "memory allocation failed\n");
		exit(1);
	}
	float *slot = ramp;
	float dx = 1.0 / size;
	float x;
	int i;

	for (i = 0, x = 0.0, slot = ramp; i < size; i++, x += dx, slot++)
	{
		float v = func(x);
		*slot = v;
	}

	glBindTexture(GL_TEXTURE_1D, texIndex);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_LUMINANCE, size, 0, GL_LUMINANCE, GL_FLOAT, ramp);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}