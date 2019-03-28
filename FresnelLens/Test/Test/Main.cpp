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
static CGparameter fresnelBias;
static CGparameter fresnelScale;
static CGparameter fresnelPower;
static CGparameter etaRatio;
static CGparameter changeCoordMatrix;
static CGparameter modelToWroldCoordMatrix;
static CGparameter eyePosition;

static CGparameter reflectivity;
static CGparameter environmentMapRef;
static CGparameter environmentMapRed;
static CGparameter environmentMapGreen;
static CGparameter environmentMapBlue;

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
static float mEtaRatio[3] = {1.1, 1.2, 1.3};

float projectionMatrix[16]; //Í¶Ó°¾ØÕó


void OnDraw();
void ReShape(int width, int height);
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
void LoadCubeMapFromDDS(const char *filename);
void LoadDecalFromDDS(const char *filename);
void DrawSurroundings(const GLfloat* mEyePosition);
void DrawMonkeyHead();
void OnMousePressed(int button, int state, int x, int y);
void OnMouseDrag(int x, int y);

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
	glClearColor(0, 0, 0, 0);
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
	GET_VERTEX_PROGRAM(fresnelBias);
	GET_VERTEX_PROGRAM(fresnelScale);
	GET_VERTEX_PROGRAM(fresnelPower);
	GET_VERTEX_PROGRAM(etaRatio);

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
	GET_FRAGMENT_PROGRAM(environmentMapRef);
	GET_FRAGMENT_PROGRAM(environmentMapRed);
	GET_FRAGMENT_PROGRAM(environmentMapGreen);
	GET_FRAGMENT_PROGRAM(environmentMapBlue);
	CheckCgError("Get Named Parameter Error");

	cgGLSetTextureParameter(environmentMapRef, 2);
	cgGLSetTextureParameter(environmentMapRed, 2);
	cgGLSetTextureParameter(environmentMapGreen, 2);
	cgGLSetTextureParameter(environmentMapBlue, 2);

	glBindTexture(GL_TEXTURE_2D, 1);
	LoadDecalFromDDS(".//TilePattern.dds");
	 
	glBindTexture(GL_TEXTURE_CUBE_MAP, 2);
	LoadCubeMapFromDDS(".//CloudyHillsCubemap.dds");

	glutMainLoop();
	return 0;
}

void OnDraw()
{
	float translationMatrix[16], rotateMatrix[16], viewMatrix[16], finalMatrix[16];
	float mEyePosition[4] = { 6 * sin(eyeAngle), eyeHeight, 6 * cos(eyeAngle) , 1};

	cgSetParameter1f(reflectivity, mReflectivity);
	cgSetParameter1f(fresnelBias, 0);
	cgSetParameter1f(fresnelScale, 1);
	cgSetParameter1f(fresnelPower, 5);
	cgSetParameter3fv(etaRatio, mEtaRatio);
	

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

	DrawSurroundings(mEyePosition);

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

void DrawSurroundings(const GLfloat* mEyePosition)
{
	static const GLfloat vertex[4 * 6][3] =
	{
		{ 1, -1, -1},{ 1,  1, -1},{ 1,  1,  1},{ 1, -1,  1},
		{-1, -1, -1},{-1,  1, -1},{-1,  1,  1},{-1, -1,  1},
		{-1,  1, -1},{ 1,  1, -1},{ 1,  1,  1},{-1,  1,  1},
		{-1, -1, -1},{ 1, -1, -1},{ 1, -1,  1},{-1, -1,  1},
		{-1, -1,  1},{ 1, -1,  1},{ 1,  1,  1},{-1,  1,  1},
		{-1, -1, -1},{ 1, -1, -1},{ 1,  1, -1},{-1,  1, -1},
	};
	const float SurroundingsScale = 8;
	glLoadIdentity();
	gluLookAt(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 0, 0, 0, 1, 0);
	glScalef(SurroundingsScale, SurroundingsScale, SurroundingsScale);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 2);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPEAT);
	glBegin(GL_QUADS);
		for (int i = 0; i < 4 * 6; i++)
		{
			glTexCoord3fv(vertex[i]);
			glVertex3fv(vertex[i]);
		}
	glEnd();
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

/** Simple image loaders for DirectX's DirectDraw Surface (DDS) format **/

/* Structure matching the Microsoft's "DDS File Reference" documentation. */
typedef struct {
	int magic; /* must be "DDS\0" */
	int size; /* must be 124 */
	int flags;
	int height;
	int width;
	int pitchOrLinearSize;
	int depth;
	int mipMapCount;
	int reserved[11];
	struct {
		int size;
		int flags;
		int fourCC;
		int bitsPerPixel;
		int redMask;
		int greenMask;
		int blueMask;
		int alphaMask;
	} pixelFormat;
	struct {
		int caps;
		int caps2;
		int caps3;
		int caps4;
	} caps;
	int reserved2[1];
} DDS_file_header;

/* This is a "good enough" loader for DDS cube maps compressed in the DXT1 format. */
void LoadCubeMapFromDDS(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	long size;
	void *data;
	char *beginning, *image;
	int *words;
	size_t bytes;
	DDS_file_header *header;
	int i, face, level;

	if (!file) {
		fprintf(stderr, "%s: could not open cube map %s\n", frameTitle, filename);
		exit(1);
	}

	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	if (size < 0) {
		fprintf(stderr, "%s: ftell failed\n", frameTitle);
		exit(1);
	}
	fseek(file, 0L, SEEK_SET);
	data = (char*)malloc((size_t)(size));
	if (data == NULL) {
		fprintf(stderr, "%s: malloc failed\n", frameTitle);
		exit(1);
	}
	bytes = fread(data, 1, (size_t)(size), file);
	fclose(file);

	if (bytes < sizeof(DDS_file_header)) {
		fprintf(stderr, "%s: DDS header to short for %s\n", frameTitle, filename);
		exit(1);
	}

#define FOURCC(a) ((a[0]) | (a[1] << 8) | (a[2] << 16) | (a[3] << 24))
#define EXPECT(f,v) \
  if ((f) != (v)) { \
    fprintf(stderr, "%s: field %s mismatch (got 0x%x, expected 0x%x)\n", \
      frameTitle, #f, (f), (v)); exit(1); \
  }

	/* Examine the header to make sure it is what we expect. */
	header = (DDS_file_header*)data;
	EXPECT(header->magic, FOURCC("DDS "));

#define DDSD_CAPS               0x00000001  /* caps field is valid */
#define DDSD_HEIGHT             0x00000002  /* height field is valid */
#define DDSD_WIDTH              0x00000004  /* width field is valid */
#define DDSD_PIXELFORMAT        0x00001000  /* pixelFormat field is valid */
#define DDSD_MIPMAPCOUNT        0x00020000  /* mipMapCount field is valid */

#define DDSD_NEEDED (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | \
                     DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT)

	EXPECT(header->flags & DDSD_NEEDED, DDSD_NEEDED);

	EXPECT(header->size, 124);
	EXPECT(header->depth, 0);
	EXPECT(header->pixelFormat.size, 32);  /* 32 bytes in a DXT1 block */
	EXPECT(header->pixelFormat.fourCC, FOURCC("DXT1"));

	/* From the DirectX SDK's ddraw.h */
#define DDSCAPS2_CUBEMAP                        0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX              0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX              0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY              0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY              0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ              0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ              0x00008000
#define DDSCAPS2_CUBEMAP_ALLFACES ( DDSCAPS2_CUBEMAP_POSITIVEX |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEX |\
                                    DDSCAPS2_CUBEMAP_POSITIVEY |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEY |\
                                    DDSCAPS2_CUBEMAP_POSITIVEZ |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEZ )
	EXPECT(header->caps.caps2, DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES);

	beginning = (char*)data;
	image = (char*)&header[1];
	/* For each face of the cube map (in +X, -X, +Y, -Y, +Z, and -Z order)... */
	for (face = 0; face < 6; face++) {
		int levels = header->mipMapCount;
		int width = header->width;
		int height = header->height;
		const int border = 0;

		/* For each mipmap level... */
		for (level = 0; level < levels; level++) {
			/* DXT1 has contains two 16-bit (565) colors and a 2-bit field for
			   each of the 16 texels in a given 4x4 block.  That's 64 bits
			   per block or 8 bytes. */
			const int bytesPer4x4Block = 8;
			/* Careful formula to compute the size of a DXT1 mipmap level.
			   This formula accounts for the fact that mipmap levels get
			   no smaller than a 4x4 block. */
			GLsizei imageSizeInBytes = ((width + 3) >> 2)*((height + 3) >> 2) * bytesPer4x4Block;
			size_t offsetInToRead = image + imageSizeInBytes - beginning;

			if (offsetInToRead > bytes) {
				fprintf(stderr, "%s: DDS images over read the data!\n", frameTitle);
				exit(1);
			}
			glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level,
				GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
				width, height, border, imageSizeInBytes, image);
			image += imageSizeInBytes;

			/* Half the width and height either iteration, but do not allow
			   the width or height to become less than 1. */
			width = width >> 1;
			if (width < 1) {
				width = 1;
			}
			height = height >> 1;
			if (height < 1) {
				height = 1;
			}
		}
	}
	assert(image <= beginning + bytes);

	/* Configure texture parameters reasonably. */
	if (header->mipMapCount > 1) {
		/* Clamp the range of levels to however levels the DDS file actually has.
		   If the DDS file has less than a full mipmap chain all the way down,
		   this allows OpenGL to still use the texture. */
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, header->mipMapCount - 1);
		/* Use better trilinear mipmap minification filter instead of the default. */
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else {
		/* OpenGL's default minification filter (GL_NEAREST_MIPMAP_LINEAR) requires
		   mipmaps this DDS file does not have so switch to a linear filter that
		   doesn't require mipmaps. */
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	/* To eliminate artifacts at the seems from the default wrap mode (GL_REPEAT),
	   switch the wrap modes to clamp to edge. */
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	free(data);
}

/* This is a "good enough" loader for DDS 2D decals compressed in the DXT1 format. */
void LoadDecalFromDDS(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	long size;
	void *data;
	char *beginning, *image;
	int *words;
	size_t bytes;
	DDS_file_header *header;
	int i, level;

	if (!file) {
		fprintf(stderr, "%s: could not open decal %s\n", frameTitle, filename);
		exit(1);
	}

	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	if (size < 0) {
		fprintf(stderr, "%s: ftell failed\n", frameTitle);
		exit(1);
	}
	fseek(file, 0L, SEEK_SET);
	data = (char*)malloc((size_t)(size));
	if (data == NULL) {
		fprintf(stderr, "%s: malloc failed\n", frameTitle);
		exit(1);
	}
	bytes = fread(data, 1, (size_t)(size), file);
	fclose(file);

	if (bytes < sizeof(DDS_file_header)) {
		fprintf(stderr, "%s: DDS header to short for %s\n", frameTitle, filename);
		exit(1);
	}

	/* Examine the header to make sure it is what we expect. */
	header = (DDS_file_header*)data;
	EXPECT(header->magic, FOURCC("DDS "));
	EXPECT(header->flags & DDSD_NEEDED, DDSD_NEEDED);
	EXPECT(header->size, 124);
	EXPECT(header->depth, 0);
	EXPECT(header->pixelFormat.size, 32);  /* 32 bytes in a DXT1 block */
	EXPECT(header->pixelFormat.fourCC, FOURCC("DXT1"));
	EXPECT(header->caps.caps2, 0);

	beginning = (char*)data;
	image = (char*)&header[1];
	{
		int levels = header->mipMapCount;
		int width = header->width;
		int height = header->height;
		const int border = 0;

		/* For each mipmap level... */
		for (level = 0; level < levels; level++) {
			/* DXT1 has contains two 16-bit (565) colors and a 2-bit field for
			   each of the 16 texels in a given 4x4 block.  That's 64 bits
			   per block or 8 bytes. */
			const int bytesPer4x4Block = 8;
			GLsizei imageSizeInBytes = ((width + 3) >> 2)*((height + 3) >> 2) * bytesPer4x4Block;
			size_t offsetInToRead = image + imageSizeInBytes - beginning;

			if (offsetInToRead > bytes) {
				fprintf(stderr, "%s: DDS images over read the data!\n", frameTitle);
				exit(1);
			}
			glCompressedTexImage2D(GL_TEXTURE_2D, level,
				GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
				width, height, border, imageSizeInBytes, image);
			image += imageSizeInBytes;

			/* Half the width and height either iteration, but do not allow
			   the width or height to become less than 1. */
			width = width >> 1;
			if (width < 1) {
				width = 1;
			}
			height = height >> 1;
			if (height < 1) {
				height = 1;
			}
		}
	}
	assert(image <= beginning + bytes);

	/* Configure texture parameters reasonably. */
	if (header->mipMapCount > 1) {
		/* Clamp the range of levels to however levels the DDS file actually has.
		   If the DDS file has less than a full mipmap chain all the way down,
		   this allows OpenGL to still use the texture. */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, header->mipMapCount - 1);
		/* Use better trilinear mipmap minification filter instead of the default. */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else {
		/* OpenGL's default minification filter (GL_NEAREST_MIPMAP_LINEAR) requires
		   mipmaps this DDS file does not have so switch to a linear filter that
		   doesn't require mipmaps. */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	/* Fine to leave a decal using the default wrap modes (GL_REPEAT). */
	free(data);
}