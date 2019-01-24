#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"cg.lib")
#pragma comment(lib,"cgGL.lib")
static CGcontext myCgContext;
static CGprofile myCgVertexProfile;
static CGprogram myCgVertexProgram;
static CGprofile myCgFragmentProfile;
static CGprogram myCgFragmentProgram;
static CGparameter gravity;
static CGparameter curTime;
static CGparameter changeCoordMatrix;
static const char* frameTitle = "Cg Test";
static const char* cgVFileName = "VertexCG.cg";
static const char* cgVFuncName = "VertexMain";
static const char* cgFFileName = "FragmentCG.cg";
static const char* cgFFuncName = "TextureMain";
static GLubyte myTexture[3 * 512 * 512];
static const char* textureDir = ".//dog.bmp";


void OnDraw();
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
	glClearColor(0, 0, 0, 0);
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
	CheckCgError("Create v Program Error");
	cgGLLoadProgram(myCgVertexProgram);
	CheckCgError("Load Program Error");

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
	CheckCgError("Create f Program Error");
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
	glClear(GL_COLOR_BUFFER_BIT);
	cgGLBindProgram(myCgVertexProgram);
	CheckCgError("Bind Vertex Program Error");
	cgGLEnableProfile(myCgVertexProfile);
	CheckCgError("Enable Vertex Profile Error");

	cgGLBindProgram(myCgFragmentProgram);
	CheckCgError("Bind Fragment Program Error");
	cgGLEnableProfile(myCgFragmentProfile);
	CheckCgError("Enable Fragment Profile Error");

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