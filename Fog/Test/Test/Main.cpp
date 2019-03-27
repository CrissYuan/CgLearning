#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/wglew.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include "matrix.h"

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgGL.lib")

#define ct_assert(b)         ct_assert_i(b,__LINE__)
#define ct_assert_i(b,line)  ct_assert_ii(b,line)
#define ct_assert_ii(b,line) void compile_time_assertion_failed_in_line_##line(int _compile_time_assertion_failed_in_line_##line[(b) ? 1 : -1])
#define CITY_COLS 20
#define CITY_ROWS 20

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
static float eyeHeight = 8;
static float eyeAngle = 0;
static int beginx, beginy;
static int supports_texture_anisotropy = 0;
static int moving = 0;
static GLubyte myTexture[3 * 512 * 512];
static const char* textureDir = ".//cat.bmp";

enum {
	TO_SIDES = 1,
	TO_ROOF,
	TO_PAVEMRNT
};

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
ct_assert(sizeof(DDS_file_header) == 128);
#if defined(__LITTLE_ENDIAN__) || defined(_WIN32)
#else
static const unsigned int nativeIntOrder = 0x03020100;
#define LE_INT32_BYTE_OFFSET(a) (((unsigned char*)&nativeIntOrder)[a])
#endif

static int int_le2native(int v)
{
#if defined(__LITTLE_ENDIAN__) || defined(_WIN32)
	return v;
#else
	union {
		int i;
		unsigned char b[4];
	} src, dst;

	src.i = v;
	dst.b[0] = src.b[LE_INT32_BYTE_OFFSET(0)];
	dst.b[1] = src.b[LE_INT32_BYTE_OFFSET(1)];
	dst.b[2] = src.b[LE_INT32_BYTE_OFFSET(2)];
	dst.b[3] = src.b[LE_INT32_BYTE_OFFSET(3)];
	return dst.i;
#endif
}

void OnDraw();
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
void LoadBMP(const char* Filename);
void ReShape(int width, int height);
void OnMouseMotion(int x, int y);
void OnMouse(int button, int state, int x, int y);
void LoadDecalFromDDS(const char *filename);
static void DrawBuilding(float x, float y, float height);
static void DrawCity();
static float Random0To1();

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

	for (words = (int *)data, i = 0; i < sizeof(DDS_file_header) / sizeof(int); i++) {
		words[i] = int_le2native(words[i]);
	}

	#define FOURCC(a) ((a[0]) | (a[1] << 8) | (a[2] << 16) | (a[3] << 24))
	#define EXPECT(f,v) \
		if ((f) != (v)) { \
			fprintf(stderr, "%s: field %s mismatch (got 0x%x, expected 0x%x)\n", \
			frameTitle, #f, (f), (v)); exit(1); \
		}
		header = (DDS_file_header *)data;
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
	EXPECT(header->caps.caps2, 0);

	beginning = (char *)data;
	image = (char*)&header[1];
	{
		int levels = header->mipMapCount;
		int width = header->width;
		int height = header->height;
		const int border = 0;

		for (level = 0; level < levels; level++) {
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

	if (header->mipMapCount > 1) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, header->mipMapCount - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	free(data);
}

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

	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "failed to initialize GLEW.\n");
		exit(1);
	}

	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);

	supports_texture_anisotropy = glutExtensionSupported("GL_EXT_texture_filter_anisotropic");

	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glBindTexture(GL_TEXTURE_2D, 31); //绑定纹理编号为31
	//LoadBMP(textureDir);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB8,512 ,512, GL_RGB, GL_UNSIGNED_BYTE,myTexture);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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
	//cgGLSetTextureParameter(textureParameter, 31); //31为纹理编号
	cgGLSetTextureParameter(textureParameter, TO_SIDES);
	CheckCgError("Set Texture Parameter Error");

	glBindTexture(GL_TEXTURE_2D, TO_SIDES);
	LoadDecalFromDDS("BuildingWindows.dds");
	if (supports_texture_anisotropy)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
	}

	glBindTexture(GL_TEXTURE_2D, TO_ROOF);
	LoadDecalFromDDS("BuildingRoof.dds");
	if (supports_texture_anisotropy)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
	}

	glBindTexture(GL_TEXTURE_2D, TO_PAVEMRNT);
	LoadDecalFromDDS("Pavement.dds");
	if (supports_texture_anisotropy)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
	}

	glutMainLoop();
	return 0;
}

static void DrawBuilding(float x, float y, float height)
{
	static GLfloat streetColor[3] = { 1, 1, 1 };
	//绘制街道
	glBindTexture(GL_TEXTURE_2D, TO_PAVEMRNT);
	glColor3fv(streetColor);
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(1.0 / 4.0, 1.0 / 4.0);
		glVertex3f(x + 1, 0, y + 1);
		glTexCoord2f(4.0 / 4.0, 1.0 / 4.0);
		glVertex3f(x + 4, 0, y + 1);
		glTexCoord2f(4.0 / 4.0, 0.0 / 4.0);
		glVertex3f(x + 4, 0, y + 0);
		glTexCoord2f(0.0 / 4.0, 0.0 / 4.0);
		glVertex3f(x + 0, 0, y + 0);
		glTexCoord2f(0.0 / 4.0, 4.0 / 4.0);
		glVertex3f(x + 0, 0, y + 4);
		glTexCoord2f(1.0 / 4.0, 4.0 / 4.0);
		glVertex3f(x + 1, 0, y + 4);
	glEnd();

	//绘制楼顶
	static GLfloat roof_coords[4][2] = { {1, 1}, {1, 0}, {0, 0}, {0, 1} };
	GLfloat rand_color = Random0To1() / 3 + 0.5;
	static GLfloat roofColor[3] = { rand_color, rand_color, rand_color };
	const int roof_index = rand() % 4;
	glBindTexture(GL_TEXTURE_2D, TO_ROOF);
	glColor3fv(roofColor);
	glBegin(GL_QUADS);
		glTexCoord2fv(roof_coords[(roof_index + 0) % 4]);
		glVertex3f(x + 4, height, y + 4);
		glTexCoord2fv(roof_coords[(roof_index + 1) % 4]);
		glVertex3f(x + 4, height, y + 1);
		glTexCoord2fv(roof_coords[(roof_index + 2) % 4]);
		glVertex3f(x + 1, height, y + 1);
		glTexCoord2fv(roof_coords[(roof_index + 3) % 4]);
		glVertex3f(x + 1, height, y + 4);
	glEnd();

	//绘制楼体
	float topTex = 1 + floor(height / 2);
	float deltaHeight = height / topTex;
	float tex0, tex1, height0, height1 = 0;
	GLfloat sideColor[3] = { Random0To1()*0.6 + 0.4, Random0To1()*0.6 + 0.4, Random0To1()*0.6 + 0.4 };
	glBindTexture(GL_TEXTURE_2D, TO_SIDES);
	glColor3fv(sideColor);
	glBegin(GL_QUADS);
		for (tex0 = 0, tex1 = 1, height0 = 0, height1 = deltaHeight;
			tex0 < topTex; 
			tex0++, tex1++, height0 += deltaHeight, height1 += deltaHeight)
		{
			//side1
			glTexCoord2f(0, tex0);
			glVertex3f(x + 1, height0, y + 1);
			glTexCoord2f(0, tex1);
			glVertex3f(x + 1, height1, y + 1);
			glTexCoord2f(1, tex1);
			glVertex3f(x + 4, height1, y + 1);
			glTexCoord2f(1, tex0);
			glVertex3f(x + 4, height0, y + 1);
			//side2
			glTexCoord2f(1, tex0);
			glVertex3f(x + 1, height0, y + 4);
			glTexCoord2f(1, tex1);
			glVertex3f(x + 1, height1, y + 4);
			glTexCoord2f(0, tex1);
			glVertex3f(x + 1, height1, y + 1);
			glTexCoord2f(0, tex0);
			glVertex3f(x + 1, height0, y + 1);
			//side3
			glTexCoord2f(0, tex0);
			glVertex3f(x + 4, height0, y + 1);
			glTexCoord2f(0, tex1);
			glVertex3f(x + 4, height1, y + 1);
			glTexCoord2f(1, tex1);
			glVertex3f(x + 4, height1, y + 4);
			glTexCoord2f(1, tex0);
			glVertex3f(x + 4, height0, y + 4);
			//side4
			glTexCoord2f(1, tex0);
			glVertex3f(x + 4, height0, y + 4);
			glTexCoord2f(1, tex1);
			glVertex3f(x + 4, height1, y + 4);
			glTexCoord2f(0, tex1);
			glVertex3f(x + 1, height1, y + 4);
			glTexCoord2f(0, tex0);
			glVertex3f(x + 1, height0, y + 4);
		}
	glEnd();
}

static void DrawCity()
{
	//srand(time(0));
	srand(999);
	for (int i = 0; i < CITY_COLS; i++)
	{
		for (int j = 0; j < CITY_ROWS; j++)
		{
			float x, y, height = 0;
			x = 4 * i - (2 * CITY_COLS + 0.5);
			y = 4 * j - (2 * CITY_ROWS + 0.5);
			height = 1.0 + Random0To1() * 10;
			DrawBuilding(x, y, height);
		}
	}
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

	makeLookAtMatrix(mEyePosition[0], mEyePosition[1], mEyePosition[2], 0, 8, 0, 0, 1, 0, viewMatrix);
	makeTranslateMatrix(0, 0, 0, translateMatrix);
	makeRotateMatrix(0, 0, 1, 0, rotateMatrix);
	multMatrix(finalMatrix, translateMatrix, rotateMatrix);
	multMatrix(finalMatrix, viewMatrix, finalMatrix);
	multMatrix(finalMatrix, projectionMatrix, finalMatrix);
	cgSetMatrixParameterfr(changeCoordMatrix, finalMatrix);

	cgUpdateProgramParameters(myCgVertexProgram);
	cgUpdateProgramParameters(myCgFragmentProgram);

	DrawCity();

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
	const float minHeight = 0,
		maxHeight = 95;

	if (moving) {
		eyeAngle += 0.01*(beginx - x);
		eyeHeight += 0.04*(beginy - y);
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

static float Random0To1()
{
	return ((float)rand()) / RAND_MAX;
}