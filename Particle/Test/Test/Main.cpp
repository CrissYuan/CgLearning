#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include <math.h>

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

bool m_animating = true; //控制动画
bool m_verBose = false; //是否输出调试信息
int m_pass = 0; //回收使用死亡的粒子
float m_curTime = 0; //当前时间
typedef struct
{
	float i_position[3];
	float i_velocity[3];
	float i_startTime;
	bool isAlive;
} Particle;

#define MAX_NUM_PARTICLES 300
Particle m_particleSystem[MAX_NUM_PARTICLES];
float FloatRand()
{
	return rand() / (float)RAND_MAX;
}
#define RANDOM_RANGE(min, max) (min + (max - min) * FloatRand())


void OnDraw();
void OnKeyBoard(unsigned char c, int x, int y);
void CheckCgError(const char* situation);
void Idle();
void ResetParticles();
void AdvanceParticles();
void Visibility(int state);

int main(int argc, char *argv[])
{
	ResetParticles();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(300, 300);
	glutCreateWindow(frameTitle);
	glutDisplayFunc(&OnDraw);
	glutVisibilityFunc(Visibility);
	glutKeyboardFunc(&OnKeyBoard);
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "%s: failed to initialize Glew.\n");
		exit(0);
	}
	glClearColor(0, 0, 0, 0);
	glPointSize(16); //点的大小
	glEnable(GL_POINT_SMOOTH);

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
	curTime = cgGetNamedParameter(myCgVertexProgram, "curTime");
	CheckCgError("Get curTime Parameter Error");
	gravity = cgGetNamedParameter(myCgVertexProgram, "gravity");
	CheckCgError("Get gravity Parameter Error");
	changeCoordMatrix = cgGetNamedParameter(myCgVertexProgram, "changeCoordMatrix");
	CheckCgError("Get changeCoordMatrix Parameter Error");

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
	glutMainLoop();
	return 0;
}

void OnDraw()
{
	const float m_gravity = -9.8;

	glClear(GL_COLOR_BUFFER_BIT);
	cgGLBindProgram(myCgVertexProgram);
	CheckCgError("Bind Vertex Program Error");
	cgGLEnableProfile(myCgVertexProfile);
	CheckCgError("Enable Vertex Profile Error");

	cgGLBindProgram(myCgFragmentProgram);
	CheckCgError("Bind Fragment Program Error");
	cgGLEnableProfile(myCgFragmentProfile);
	CheckCgError("Enable Fragment Profile Error");

	glLoadIdentity();
	cgSetParameter1f(curTime, m_curTime);
	cgSetParameter4f(gravity, 0, m_gravity, 0, 1);
	cgGLSetStateMatrixParameter(changeCoordMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);

	glBegin(GL_POINTS);
		for (int i = 0; i < MAX_NUM_PARTICLES; i++)
		{
			if (m_particleSystem[i].isAlive)
			{
				glTexCoord3fv(m_particleSystem[i].i_velocity);
				glMultiTexCoord1f(GL_TEXTURE1, m_particleSystem[i].i_startTime);
				glVertex3fv(m_particleSystem[i].i_position);
				if (m_verBose)
				{
					printf("Draw %d (%f, %f, %f) at %f\n", 
							i, 
							m_particleSystem[i].i_velocity[0],
							m_particleSystem[i].i_velocity[1],
							m_particleSystem[i].i_velocity[2],
							m_curTime
							);
					printf("Pass %d \n", m_pass);
				}
			}
		}
	glEnd();
	cgGLDisableProfile(myCgVertexProfile);
	CheckCgError("Disable Vertex Profile Error");

	cgGLDisableProfile(myCgFragmentProfile);
	CheckCgError("Disable Fragment Profile Error");

	glutSwapBuffers();
}

void Idle()
{
	if (m_animating)
	{
		m_curTime += 0.0002;
		AdvanceParticles();
	}
	glutPostRedisplay();
}

void ResetParticles()
{
	m_curTime = 0;
	m_pass = 0;
	for (int i = 0; i < MAX_NUM_PARTICLES; i++)
	{
		float radius = 0.25;
		float initialY = -0.6;
		m_particleSystem[i].i_position[0] = radius * cos(i * 0.5);
		m_particleSystem[i].i_position[1] = initialY;
		m_particleSystem[i].i_position[2] = radius * sin(i * 0.5);
		m_particleSystem[i].i_startTime = RANDOM_RANGE(0, 10);
		m_particleSystem[i].isAlive = false;
	}
}

void AdvanceParticles()
{
	float deathTime = m_curTime - 3.0;
	m_pass++;
	for (int i = 0; i < MAX_NUM_PARTICLES; i++)
	{
		if (!m_particleSystem[i].isAlive && m_particleSystem[i].i_startTime <= m_curTime)
		{
			m_particleSystem[i].i_velocity[0] = RANDOM_RANGE(-1, 1);
			m_particleSystem[i].i_velocity[1] = RANDOM_RANGE(0, 6);
			m_particleSystem[i].i_velocity[2] = RANDOM_RANGE(-1, 1);
			m_particleSystem[i].i_startTime = m_curTime;
			m_particleSystem[i].isAlive = true;
			if (m_verBose)
			{
				printf("Brith %d (%f, %f, %f) at %f\n",
						i,
						m_particleSystem[i].i_velocity[0],
						m_particleSystem[i].i_velocity[1],
						m_particleSystem[i].i_velocity[2],
						m_curTime);
			}
		}
		if (m_particleSystem[i].isAlive && m_particleSystem[i].i_startTime <= deathTime)
		{
			m_particleSystem[i].isAlive = false;
			if (m_verBose)
			{
				printf("Death %d (%f, %f, %f) at %f\n",
					i,
					m_particleSystem[i].i_velocity[0],
					m_particleSystem[i].i_velocity[1],
					m_particleSystem[i].i_velocity[2],
					m_curTime);
			}
		}
	}
}

void Visibility(int state)
{
	if (state == GLUT_VISIBLE && m_animating)
	{
		glutIdleFunc(Idle);
	}
	else
	{
		glutIdleFunc(NULL);
	}
}

void OnKeyBoard(unsigned char c, int x, int y)
{
	static bool useCGPointSize = false;
	switch (c)
	{
	case 27:
		cgDestroyProgram(myCgVertexProgram);
		cgDestroyProgram(myCgFragmentProgram);
		cgDestroyContext(myCgContext);
		exit(0);
		break;
	case 'p':
	case 'P':
		useCGPointSize = !useCGPointSize;
		if (useCGPointSize)
		{
			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
		}
		else
		{
			glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
		}
		glutPostRedisplay();
		break;
	case 'R':
	case 'r':
		ResetParticles();
		glutPostRedisplay();
		break;
	case 'v':
	case 'V':
		m_verBose = !m_verBose;
		break;
	case ' ':
		m_animating = !m_animating;
		if (m_animating)
		{
			glutIdleFunc(Idle);
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