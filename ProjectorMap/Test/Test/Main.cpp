#include <stdio.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glut.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgGL.lib")
static CGcontext myCgContext;
static CGprofile myCgVertexProfile;
static CGprogram myCgVertexProgram;
static CGprofile myCgFragmentProfile;
static CGprogram myCgFragmentProgram;
static CGparameter changeCoordMatrix;
static CGparameter textureMatrix;
static CGparameter lightPosition;
static CGparameter Kd;
static const char* frameTitle = "Cg Test";
static const char* cgVFileName = "VertexCG.cg";
static const char* cgVFuncName = "VertexMain";
static const char* cgFFileName = "FragmentCG.cg";
static const char* cgFFuncName = "FragmentMain";
static GLubyte myTexture[3 * 512 * 512];
static const char* textureDir = ".//cat.bmp";
static float projectionMatrix[16];
static const double myPi = 3.14159265358979323846;
static float mEyeAngle = 0;
static float mEyeHeight = 0;
static float mLightAngle = 0;
static float mLightHeight = 1;
static float mKd[4] = { 1, 1, 1, 1 };

static bool  moving = false;
static int   beginx, beginy;
static bool  lmoving = false;
static int   lbeginx, lbeginy;


void OnDraw();
void DrawRoom();
void SetupDemonSampler();
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
void LoadBMP(const char* Filename);
void ReShape(int width, int height);
void OnMousePressed(int button, int state, int x, int y);
void OnMouseDrag(int x, int y);
static void buildPerspectiveMatrix(double fieldOfView, double aspectRatio, double zNear, double zFar, float m[16]);
static void buildLookAtMatrix(double eyex, double eyey, double eyez, double centerx, double centery, double centerz, double upx, double upy, double upz, float m[16]);
static void normalizeVector(float v[3]);
static void makeRotateMatrix(float angle, float ax, float ay, float az, float m[16]);
static void makeClipToTextureMatrix(float m[16]);
static void makeTranslateMatrix(float x, float y, float z, float m[16]);
static void multMatrix(float dst[16], const float src1[16], const float src2[16]);
static void invertMatrix(float *out, const float *m);
static void transform(float dst[4], const float mat[16], const float vec[4]);
void loadMVP(const float modelView[16]);
static void buildTextureMatrix(const float viewMatrix[16], const float modelMatrix[16], float textureMatrix[16]);

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 300);
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
	GET_VERTEX_PROGRAM(textureMatrix); 

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
	GET_FRAGMENT_PROGRAM(lightPosition);
	GET_FRAGMENT_PROGRAM(Kd);

	cgSetParameter4fv(Kd, mKd);

	SetupDemonSampler();

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

	float translationMatrix[16], rotateMatrix[16], viewMatrix[16], 
		finalMatrix[16], modelViewMatrix[16], invMatrix[16],
		texMatrix[16], lightViewMatrix[16];
	float mSpaceLightPosition[4];
	float mEyePosition[4] = { 10 * sin(mEyeAngle), mEyeHeight, 10 * cos(mEyeAngle) , 1 };
	float mLightPosition[4] = { 4.5 * sin(mLightAngle), mLightHeight, 4.5 * cos(mLightAngle) , 1 };

	buildLookAtMatrix(mLightPosition[0], mLightPosition[1], mLightPosition[2], 0, 0, 0, 0, -1, 0, lightViewMatrix);
	

	buildLookAtMatrix(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 0, 0, 0, 1, 0, viewMatrix);
	//球
	makeTranslateMatrix(2, 0, 0, translationMatrix);
	makeRotateMatrix(70, 1, 1, 1, rotateMatrix);
	multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	buildTextureMatrix(lightViewMatrix, finalMatrix, texMatrix);
	cgSetMatrixParameterfr(textureMatrix, texMatrix);

	invertMatrix(invMatrix, finalMatrix);
	transform(mSpaceLightPosition, invMatrix, mLightPosition);
	cgSetParameter3fv(lightPosition, mSpaceLightPosition);

	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);

	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	glutSolidSphere(2, 50, 50);

	//立方体
	makeTranslateMatrix(-3, 0, 0, translationMatrix);
	makeRotateMatrix(20, 0, 1, 0, rotateMatrix);
	multMatrix(finalMatrix, translationMatrix, rotateMatrix);
	buildTextureMatrix(lightViewMatrix, finalMatrix, texMatrix);
	cgSetMatrixParameterfr(textureMatrix, texMatrix);

	invertMatrix(invMatrix, finalMatrix);
	transform(mSpaceLightPosition, invMatrix, mLightPosition);
	cgSetParameter3fv(lightPosition, mSpaceLightPosition);

	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	glutSolidCube(2.5);

	//房间
	cgSetParameter3fv(lightPosition, mLightPosition);
	multMatrix(finalMatrix, projectionMatrix, viewMatrix);

	makeTranslateMatrix(0, 0, 0, translationMatrix);
	buildTextureMatrix(lightViewMatrix, translationMatrix, texMatrix);

	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);
	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);
	DrawRoom();

	cgGLDisableProfile(myCgVertexProfile);
	CheckCgError("Disable Vertex Profile Error");

	cgGLDisableProfile(myCgFragmentProfile);
	CheckCgError("Disable Fragment Profile Error");

	makeTranslateMatrix(mLightPosition[0], mLightPosition[1], mLightPosition[2], finalMatrix);
	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	glPushMatrix();
	loadMVP(finalMatrix);
	glColor3f(0.9, 0.9, 0.3);
	glutSolidSphere(0.15, 10, 10);
	glColor3f(1, 1, 1);
	glPopMatrix();

	glutSwapBuffers();
}

void DrawRoom()
{
	glEnable(GL_CULL_FACE);

	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);
	glVertex3f(12, -2, -12);
	glVertex3f(-12, -2, -12);
	glVertex3f(-12, -2, 12);
	glVertex3f(12, -2, 12);

	glNormal3f(0, 0, 1);
	glVertex3f(-12, -2, -12);
	glVertex3f(12, -2, -12);
	glVertex3f(12, 10, -12);
	glVertex3f(-12, 10, -12);

	glNormal3f(0, 0, -1);
	glVertex3f(12, -2, 12);
	glVertex3f(-12, -2, 12);
	glVertex3f(-12, 10, 12);
	glVertex3f(12, 10, 12);

	glNormal3f(0, -1, 0);
	glVertex3f(-12, 10, -12);
	glVertex3f(12, 10, -12);
	glVertex3f(12, 10, 12);
	glVertex3f(-12, 10, 12);

	glNormal3f(1, 0, 0);
	glVertex3f(-12, -2, 12);
	glVertex3f(-12, -2, -12);
	glVertex3f(-12, 10, -12);
	glVertex3f(-12, 10, 12);

	glNormal3f(-1, 0, 0);
	glVertex3f(12, -2, -12);
	glVertex3f(12, -2, 12);
	glVertex3f(12, 10, 12);
	glVertex3f(12, 10, -12);
	glEnd();

	glDisable(GL_CULL_FACE);
}

void SetupDemonSampler()
{
	GLuint texobj = 666;
	CGparameter sampler;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); /* Tightly packed texture data. */

	glBindTexture(GL_TEXTURE_2D, texobj);
	/* Load demon decal texture with mipmaps. */
	LoadBMP(textureDir);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB8,
		512, 512, GL_RGB, GL_UNSIGNED_BYTE, myTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);

	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, mKd);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	sampler = cgGetNamedParameter(myCgFragmentProgram, "projectiveMap");
	CheckCgError("getting projectiveMap sampler parameter");
	cgGLSetTextureParameter(sampler, texobj);
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
	buildPerspectiveMatrix(fieldOfView, aspectRatio, 0.1, 50.0, projectionMatrix);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fieldOfView, aspectRatio, 0.1, 50.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, width, height);
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




/* Build a row-major (C-style) 4x4 matrix transform based on the
   parameters for gluPerspective. */
static void buildPerspectiveMatrix(double fieldOfView,
	double aspectRatio,
	double zNear, double zFar,
	float m[16])
{
	double sine, cotangent, deltaZ;
	double radians = fieldOfView / 2.0 * myPi / 180.0;

	deltaZ = zFar - zNear;
	sine = sin(radians);
	/* Should be non-zero to avoid division by zero. */
	assert(deltaZ);
	assert(sine);
	assert(aspectRatio);
	cotangent = cos(radians) / sine;

	m[0 * 4 + 0] = cotangent / aspectRatio;
	m[0 * 4 + 1] = 0.0;
	m[0 * 4 + 2] = 0.0;
	m[0 * 4 + 3] = 0.0;

	m[1 * 4 + 0] = 0.0;
	m[1 * 4 + 1] = cotangent;
	m[1 * 4 + 2] = 0.0;
	m[1 * 4 + 3] = 0.0;

	m[2 * 4 + 0] = 0.0;
	m[2 * 4 + 1] = 0.0;
	m[2 * 4 + 2] = -(zFar + zNear) / deltaZ;
	m[2 * 4 + 3] = -2 * zNear * zFar / deltaZ;

	m[3 * 4 + 0] = 0.0;
	m[3 * 4 + 1] = 0.0;
	m[3 * 4 + 2] = -1;
	m[3 * 4 + 3] = 0;
}

/* Build a row-major (C-style) 4x4 matrix transform based on the
   parameters for gluLookAt. */
static void buildLookAtMatrix(double eyex, double eyey, double eyez,
	double centerx, double centery, double centerz,
	double upx, double upy, double upz,
	float m[16])
{
	double x[3], y[3], z[3], mag;

	/* Difference eye and center vectors to make Z vector. */
	z[0] = eyex - centerx;
	z[1] = eyey - centery;
	z[2] = eyez - centerz;
	/* Normalize Z. */
	mag = sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
	if (mag) {
		z[0] /= mag;
		z[1] /= mag;
		z[2] /= mag;
	}

	/* Up vector makes Y vector. */
	y[0] = upx;
	y[1] = upy;
	y[2] = upz;

	/* X vector = Y cross Z. */
	x[0] = y[1] * z[2] - y[2] * z[1];
	x[1] = -y[0] * z[2] + y[2] * z[0];
	x[2] = y[0] * z[1] - y[1] * z[0];

	/* Recompute Y = Z cross X. */
	y[0] = z[1] * x[2] - z[2] * x[1];
	y[1] = -z[0] * x[2] + z[2] * x[0];
	y[2] = z[0] * x[1] - z[1] * x[0];

	/* Normalize X. */
	mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
	if (mag) {
		x[0] /= mag;
		x[1] /= mag;
		x[2] /= mag;
	}

	/* Normalize Y. */
	mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
	if (mag) {
		y[0] /= mag;
		y[1] /= mag;
		y[2] /= mag;
	}

	/* Build resulting view matrix. */
	m[0 * 4 + 0] = x[0];  m[0 * 4 + 1] = x[1];
	m[0 * 4 + 2] = x[2];  m[0 * 4 + 3] = -x[0] * eyex + -x[1] * eyey + -x[2] * eyez;

	m[1 * 4 + 0] = y[0];  m[1 * 4 + 1] = y[1];
	m[1 * 4 + 2] = y[2];  m[1 * 4 + 3] = -y[0] * eyex + -y[1] * eyey + -y[2] * eyez;

	m[2 * 4 + 0] = z[0];  m[2 * 4 + 1] = z[1];
	m[2 * 4 + 2] = z[2];  m[2 * 4 + 3] = -z[0] * eyex + -z[1] * eyey + -z[2] * eyez;

	m[3 * 4 + 0] = 0.0;   m[3 * 4 + 1] = 0.0;  m[3 * 4 + 2] = 0.0;  m[3 * 4 + 3] = 1.0;
}

static void normalizeVector(float v[3])
{
	float mag;

	mag = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (mag) {
		float oneOverMag = 1.0 / mag;

		v[0] *= oneOverMag;
		v[1] *= oneOverMag;
		v[2] *= oneOverMag;
	}
}

/* Build a row-major (C-style) 4x4 matrix transform based on the
   parameters for glRotatef. */
static void makeRotateMatrix(float angle,
	float ax, float ay, float az,
	float m[16])
{
	float radians, sine, cosine, ab, bc, ca, tx, ty, tz;
	float axis[3];

	axis[0] = ax;
	axis[1] = ay;
	axis[2] = az;
	normalizeVector(axis);

	radians = angle * myPi / 180.0;
	sine = sin(radians);
	cosine = cos(radians);
	ab = axis[0] * axis[1] * (1 - cosine);
	bc = axis[1] * axis[2] * (1 - cosine);
	ca = axis[2] * axis[0] * (1 - cosine);
	tx = axis[0] * axis[0];
	ty = axis[1] * axis[1];
	tz = axis[2] * axis[2];

	m[0] = tx + cosine * (1 - tx);
	m[1] = ab + axis[2] * sine;
	m[2] = ca - axis[1] * sine;
	m[3] = 0.0f;
	m[4] = ab - axis[2] * sine;
	m[5] = ty + cosine * (1 - ty);
	m[6] = bc + axis[0] * sine;
	m[7] = 0.0f;
	m[8] = ca + axis[1] * sine;
	m[9] = bc - axis[0] * sine;
	m[10] = tz + cosine * (1 - tz);
	m[11] = 0;
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = 1;
}

static void makeClipToTextureMatrix(float m[16])
{
	m[0] = 0.5f;  m[1] = 0;     m[2] = 0;     m[3] = 0.5f;
	m[4] = 0;     m[5] = 0.5f;  m[6] = 0;     m[7] = 0.5f;
	m[8] = 0;     m[9] = 0;     m[10] = 0.5f;  m[11] = 0.5f;
	m[12] = 0;     m[13] = 0;     m[14] = 0;     m[15] = 1;
}

/* Build a row-major (C-style) 4x4 matrix transform based on the
   parameters for glTranslatef. */
static void makeTranslateMatrix(float x, float y, float z, float m[16])
{
	m[0] = 1;  m[1] = 0;  m[2] = 0;  m[3] = x;
	m[4] = 0;  m[5] = 1;  m[6] = 0;  m[7] = y;
	m[8] = 0;  m[9] = 0;  m[10] = 1;  m[11] = z;
	m[12] = 0;  m[13] = 0;  m[14] = 0;  m[15] = 1;
}

/* Simple 4x4 matrix by 4x4 matrix multiply. */
static void multMatrix(float dst[16],
	const float src1[16], const float src2[16])
{
	float tmp[16];
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			tmp[i * 4 + j] = src1[i * 4 + 0] * src2[0 * 4 + j] +
				src1[i * 4 + 1] * src2[1 * 4 + j] +
				src1[i * 4 + 2] * src2[2 * 4 + j] +
				src1[i * 4 + 3] * src2[3 * 4 + j];
		}
	}
	/* Copy result to dst (so dst can also be src1 or src2). */
	for (i = 0; i < 16; i++)
		dst[i] = tmp[i];
}

/* Invert a row-major (C-style) 4x4 matrix. */
static void invertMatrix(float *out, const float *m)
{
	/* Assumes matrices are ROW major. */
#define SWAP_ROWS(a, b) { GLdouble *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(r)*4+(c)]

	double wtmp[4][8];
	double m0, m1, m2, m3, s;
	double *r0, *r1, *r2, *r3;

	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

	r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
		r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
		r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

		r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
		r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
		r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

		r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
		r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
		r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

		r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
		r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
		r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

	/* Choose myPivot, or die. */
	if (fabs(r3[0]) > fabs(r2[0])) SWAP_ROWS(r3, r2);
	if (fabs(r2[0]) > fabs(r1[0])) SWAP_ROWS(r2, r1);
	if (fabs(r1[0]) > fabs(r0[0])) SWAP_ROWS(r1, r0);
	if (0.0 == r0[0]) {
		assert(!"could not invert matrix");
	}

	/* Eliminate first variable. */
	m1 = r1[0] / r0[0]; m2 = r2[0] / r0[0]; m3 = r3[0] / r0[0];
	s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
	s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
	s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
	s = r0[5];
	if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
	s = r0[6];
	if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
	s = r0[7];
	if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

	/* Choose myPivot, or die. */
	if (fabs(r3[1]) > fabs(r2[1])) SWAP_ROWS(r3, r2);
	if (fabs(r2[1]) > fabs(r1[1])) SWAP_ROWS(r2, r1);
	if (0.0 == r1[1]) {
		assert(!"could not invert matrix");
	}

	/* Eliminate second variable. */
	m2 = r2[1] / r1[1]; m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
	s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
	s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
	s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
	s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

	/* Choose myPivot, or die. */
	if (fabs(r3[2]) > fabs(r2[2])) SWAP_ROWS(r3, r2);
	if (0.0 == r2[2]) {
		assert(!"could not invert matrix");
	}

	/* Eliminate third variable. */
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
		r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
		r3[7] -= m3 * r2[7];

	/* Last check. */
	if (0.0 == r3[3]) {
		assert(!"could not invert matrix");
	}

	s = 1.0 / r3[3];              /* Now back substitute row 3. */
	r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

	m2 = r2[3];                 /* Now back substitute row 2. */
	s = 1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
		r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
		r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
		r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

	m1 = r1[2];                 /* Now back substitute row 1. */
	s = 1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
		r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
		r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

	m0 = r0[1];                 /* Now back substitute row 0. */
	s = 1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
		r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

	MAT(out, 0, 0) = r0[4]; MAT(out, 0, 1) = r0[5],
		MAT(out, 0, 2) = r0[6]; MAT(out, 0, 3) = r0[7],
		MAT(out, 1, 0) = r1[4]; MAT(out, 1, 1) = r1[5],
		MAT(out, 1, 2) = r1[6]; MAT(out, 1, 3) = r1[7],
		MAT(out, 2, 0) = r2[4]; MAT(out, 2, 1) = r2[5],
		MAT(out, 2, 2) = r2[6]; MAT(out, 2, 3) = r2[7],
		MAT(out, 3, 0) = r3[4]; MAT(out, 3, 1) = r3[5],
		MAT(out, 3, 2) = r3[6]; MAT(out, 3, 3) = r3[7];

#undef MAT
#undef SWAP_ROWS
}

/* Simple 4x4 matrix by 4-component column vector multiply. */
static void transform(float dst[4],
	const float mat[16], const float vec[4])
{
	double tmp[4], invW;
	int i;

	for (i = 0; i < 4; i++) {
		tmp[i] = mat[i * 4 + 0] * vec[0] +
			mat[i * 4 + 1] * vec[1] +
			mat[i * 4 + 2] * vec[2] +
			mat[i * 4 + 3] * vec[3];
	}
	invW = 1 / tmp[3];
	/* Apply perspective divide and copy to dst (so dst can vec). */
	for (i = 0; i < 3; i++)
		dst[i] = tmp[i] * invW;
	dst[3] = 1;
}

void
loadMVP(const float modelView[16])
{
	float transpose[16];
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			transpose[i * 4 + j] = modelView[j * 4 + i];
		}
	}
	glLoadMatrixf(transpose);
}

static void buildTextureMatrix(const float viewMatrix[16],
	const float modelMatrix[16],
	float textureMatrix[16])
{
	static float eyeToClipMatrix[16];
	float modelViewMatrix[16];
	static int needsInit = 1;

	if (needsInit) {
		const float fieldOfView = 50.0f;
		const float aspectRatio = 1;
		float textureProjectionMatrix[16];
		float clipToTextureMatrix[16];

		/* Build texture projection matrix once. */
		buildPerspectiveMatrix(fieldOfView, aspectRatio,
			0.25, 20.0,  /* Znear and Zfar */
			textureProjectionMatrix);

		makeClipToTextureMatrix(clipToTextureMatrix);

		/* eyeToClip = clipToTexture * textureProjection */
		multMatrix(eyeToClipMatrix,
			clipToTextureMatrix, textureProjectionMatrix);
		needsInit = 1;
	}

	/* modelView = view * model */
	multMatrix(modelViewMatrix, viewMatrix, modelMatrix);
	/* texture = eyeToClip * modelView */
	multMatrix(textureMatrix, eyeToClipMatrix, modelViewMatrix);
}
