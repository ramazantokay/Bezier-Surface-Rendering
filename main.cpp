#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <iomanip>

#include <GL/glew.h>
//#include <OpenGL/gl3.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp> // GL Math library header
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtx/normal.hpp>

//#include <glm/gtc/quaternion.hpp>
//#include <glm/gtx/quaternion.hpp>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

GLuint gProgram[2];
int gWidth, gHeight;

GLint modelingMatrixLoc[2];
GLint viewingMatrixLoc[2];
GLint projectionMatrixLoc[2];
GLint eyePosLoc[2];

GLint numberOfPointLightsLoc;

GLint lightLoc[5][2];


glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix;

glm::vec3 eyePos(0, 0, 0);
//glm::vec3 lightIntesity(0, 0, 0);

int activeProgramIndex = 0;

struct Vertex
{
	Vertex() : x(0), y(0), z(0) {}
	Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }


	Vertex operator+(Vertex& v)
	{
		return Vertex(x + v.x, y + v.y, z + v.z);
	}
	Vertex operator*(float& f)
	{
		return Vertex(x * f, y * f, z * f);
	}
	Vertex operator*(Vertex v)
	{
		return Vertex(x * v.x, y * v.y, z * v.z);
	}

	glm::vec3 vertexToGlmVec3()
	{
		return glm::vec3(x, y, z);
	}
	GLfloat x, y, z;
};

struct Texture
{
	Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
	GLfloat u, v;
};

struct Normal
{
	Normal() : x(0), y(0), z(0) {}
	//Normal(Normal n) : x(n.x), y(n.y), z(n.z){}
	Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
	Normal(glm::vec3& vec) : x(vec.x), y(vec.y), z(vec.z) { }

	glm::vec3 normalToGlmVec3()
	{
		return glm::vec3(x, y, z);
	}


	Normal& operator+=(Normal& n)
	{
		x += n.x;
		y += n.y;
		z += n.z;
		return *this;
	}

	GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
	GLuint vIndex[3], tIndex[3], nIndex[3];
};

vector<Vertex> gVertices;
vector<Texture> gTextures;
vector<Normal> gNormals;
vector<Face> gFaces;

GLuint gVertexAttribBuffer, gIndexBuffer;
GLint gInVertexLoc, gInNormalLoc;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;

// new variables
int nSample = 10;
static float rotationAngle = -30.0f;
float coordMultiplier = 1.0;
// Store control points

struct Patch
{
	Vertex patchBezierControlPoints[4][4]; // 4x4 = 16 CPs
};

struct Light
{
	glm::vec3 lightPosition;
	glm::vec3 lightColor;
};

// default LightPoint count
int numberOfPointLights = 1;

Light lightSources[5];

// default CP counts 
int verticalCPCount = 4, horizontalCPCount = 4;

vector<Patch> bezierPatches;

vector<vector<float>> heightOfCPs;

vector<Vertex> bezierSampleVertices;
vector<Vertex> bezierNormalVertices;

bool parseInputFile(const string fileName)
{
	fstream inputFile;
	// @TODO: add assert for not reading a file
	inputFile.open(fileName.c_str());

	cerr << fileName + " file is read" << endl;

	inputFile >> numberOfPointLights;
	cerr << "Number of Light source is " + to_string(numberOfPointLights);
	for (int i = 0; i < numberOfPointLights; i++)
	{
		Light light;
		inputFile >> light.lightPosition.x;
		inputFile >> light.lightPosition.y;
		inputFile >> light.lightPosition.z;
		inputFile >> light.lightColor.r;
		inputFile >> light.lightColor.g;
		inputFile >> light.lightColor.b;
		cout << "Light :" << light.lightColor.r << " " << light.lightColor.g << " " << light.lightColor.b<<endl;
		cout << "Light :" << light.lightPosition.x << " " << light.lightPosition.y << " " << light.lightPosition.z <<endl;
		lightSources[i] = light;
	}

	inputFile >> verticalCPCount;
	inputFile >> horizontalCPCount;

	cerr << "vertical CP Count " + verticalCPCount << endl;
	cerr << "horizontal CP Count " + horizontalCPCount << endl;

	int numberOfPatch = ((verticalCPCount / 4) * (horizontalCPCount / 4));
	cerr << "Number of Patch " + numberOfPatch << endl;

	bezierPatches.resize(numberOfPatch);

	// get all height values of CPs
	for (int v = 0; v < verticalCPCount; v++)
	{
		vector<float> height;
		for (int h = 0; h < horizontalCPCount; h++)
		{
			float fHeight;
			inputFile >> fHeight;
			height.push_back(fHeight);
			//height.push_back(0);

		}
		heightOfCPs.push_back(height);

	}

	// get all z values and put them into bezierSurfaceVertices
	return true;
}



// Bezier Surface functions

// linSpace function like in octave 

vector<float> linSpace(float start, float end, int sampleNumber)
{
	vector<float> result;

	for (int i = 0; i < sampleNumber; i++)
	{
		float lineDiv = start + i * (end - start) / (float)(sampleNumber - 1);
		result.push_back(lineDiv);
	}
	return result;
}

int factorial(int n)
{
	// @TODO I found more efficient way on the internet, will be optimized later
	if (n < 2)
		return 1;
	return n * factorial(n - 1);
}

int nChoose(int n, int i)
{
	return factorial(n) / factorial(i) / factorial(n - i);
}

float bernstein(int i, int n, float t)
{
	return nChoose(n, i) * pow(t, i) * pow(1 - t, n - i);

}

float bernsteinDerivative(int i, int n, float t)
{
	float ret = nChoose(n, i) * pow(t, (i - 1)) * pow(1 - t, (n - i - 1)) * (-(n - i) * t + i * (1 - t));
	return (isnan(ret)) ? 1 : ret;
}


Vertex Q(float s, float t, Patch bezierPatch)
{
	Vertex tempVertex;
	Vertex tempnormalVert;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				tempVertex.x += bernstein(i, 3, s) * bernstein(j, 3, t) * bezierPatch.patchBezierControlPoints[i][j].x;
				tempVertex.y += bernstein(i, 3, s) * bernstein(j, 3, t) * bezierPatch.patchBezierControlPoints[i][j].y;
				tempVertex.z += bernstein(i, 3, s) * bernstein(j, 3, t) * bezierPatch.patchBezierControlPoints[i][j].z;
				
			}
		}
	
	return tempVertex;
}

Vertex Qder(float s, float t, Patch bezierPatch)
{
	Vertex tt;
	Vertex ss;
	Vertex tempnormalVert;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			ss.x += bernsteinDerivative(i, 3, s) * bezierPatch.patchBezierControlPoints[i][j].x;
			ss.y += bernsteinDerivative(i, 3, s) * bezierPatch.patchBezierControlPoints[i][j].y;
			ss.z += bernsteinDerivative(i, 3, s) * bezierPatch.patchBezierControlPoints[i][j].z;

			tt.x +=  bernsteinDerivative(j, 3, t) * bezierPatch.patchBezierControlPoints[i][j].x;
			tt.y +=  bernsteinDerivative(j, 3, t) * bezierPatch.patchBezierControlPoints[i][j].y;
			tt.z += bernsteinDerivative(j, 3, t) * bezierPatch.patchBezierControlPoints[i][j].z;

		}

		glm::vec3 surfaceNormal = glm::vec3(tt.y * ss.z - tt.z * ss.y, tt.z * ss.x - tt.x * ss.z, tt.x * ss.y - tt.y * ss.x);
		tempnormalVert.x = surfaceNormal.x;
		tempnormalVert.y = surfaceNormal.y;
		tempnormalVert.z = surfaceNormal.z;
	}

	return tempnormalVert;
}

void createControlPoints()
{
	int verticalPatch = verticalCPCount / 4;
	int horizontalPatch = horizontalCPCount / 4;
	int patchSize = bezierPatches.size();
	int b = 0;
	vector<float> vLin;
	vector<float> hLin;

	{
		

		if (horizontalPatch > verticalPatch)
		{
			vLin = linSpace(0.5* coordMultiplier, -0.5* coordMultiplier, verticalCPCount);
			hLin = linSpace(-0.5* 2* coordMultiplier, 0.5* 2* coordMultiplier, horizontalCPCount);

		}
		else if (horizontalPatch < verticalPatch)
		{
			vLin = linSpace(0.5* 2* coordMultiplier, -0.5* 2* coordMultiplier, verticalCPCount);
			hLin = linSpace(-0.5* coordMultiplier, 0.5* coordMultiplier, horizontalCPCount);

		}
		else if (horizontalPatch == verticalPatch)
		{
			vLin = linSpace(0.5 * coordMultiplier, -0.5 * coordMultiplier, verticalCPCount);
			hLin = linSpace(-0.5* coordMultiplier, 0.5* coordMultiplier, horizontalCPCount);
		}
			float* xl = vLin.data();
			float* yl = hLin.data();
			
			for (int y = 0; y < verticalPatch; y++)
			{
				for (int x = 0; x < horizontalPatch; x++)
				{
					for (int i = 0; i < 4; i++)
					{
						for (int j = 0; j < 4; j++)
						{
							// x y z 

							bezierPatches[b].patchBezierControlPoints[i][j].x = *(yl + j + (x * 3)); // i was multiply with 4 then there is a gap between patches so i choose 3
							bezierPatches[b].patchBezierControlPoints[i][j].y = *(xl + i + (y * 3));
							bezierPatches[b].patchBezierControlPoints[i][j].z = heightOfCPs[i + (y * 4)][j + (x * 4)]; // 16
						
						}
					}
					b++;

				}
			}
		

	}

}

void initBezierSampleVertices()
{
	vector<float> s = linSpace(0, 1, nSample);
	vector<float> t = linSpace(0, 1, nSample);

	for (int b = 0; b < bezierPatches.size(); b++)
	{
		for (int i = 0; i < s.size(); i++)
		{
			for (int j = 0; j < t.size(); j++)
			{
				bezierSampleVertices.push_back(Q(s[i], t[j], bezierPatches[b]));
			}
		}
	}
}


void generateBezierFaces() // + normals
{
	gNormals.resize(nSample*nSample*bezierPatches.size());

	for (int b = 0; b < bezierPatches.size(); b++)
	{
		for (int i = 0; i < nSample - 1; i++)
		{
			for (int j = 0; j < nSample - 1; j++)
			{


				// //face normals
				glm::vec3 v1 = bezierSampleVertices[i * nSample + j + b * nSample * nSample].vertexToGlmVec3(); // 0
				glm::vec3 v2 = bezierSampleVertices[(i + 1) * nSample + j + b * nSample * nSample].vertexToGlmVec3(); // 4
				glm::vec3 v3 = bezierSampleVertices[(i * nSample) + j + 1 + b * nSample * nSample].vertexToGlmVec3(); // 1
				glm::vec3 v4 = bezierSampleVertices[(i + 1) * nSample + j + 1 + b * nSample * nSample].vertexToGlmVec3(); //5


				glm::vec3 n1 = glm::triangleNormal(v1, v3, v2) * glm::vec3(-1);
				glm::vec3 n2 = glm::triangleNormal(v2, v1, v3) * glm::vec3(-1);
				glm::vec3 n3 = glm::triangleNormal(v3, v2, v1) * glm::vec3(-1);
				//

				Normal cross;
				cross.x = (n1.x + n2.x + n3.x);
				cross.y = (n1.y + n2.y + n3.y);
				cross.z = (n1.z + n2.z + n3.z);

				glm::vec3 n4 = glm::triangleNormal(v3, v2, v4) * glm::vec3(-1);
				glm::vec3 n5 = glm::triangleNormal(v4, v2, v3) * glm::vec3(-1);
				glm::vec3 n6 = glm::triangleNormal(v2, v3, v4) * glm::vec3(-1);

				Normal cross2;
				cross2.x = (n4.x + n5.x + n6.x);
				cross2.y = (n4.y + n5.y + n6.y);
				cross2.z = (n4.z + n5.z + n6.z);

				gNormals[i * nSample + j + b * nSample * nSample] += cross;
				gNormals[(i + 1) * nSample + j + b * nSample * nSample] += cross;
				gNormals[i * nSample + j + 1 + b * nSample * nSample] += cross;

				gNormals[i * nSample + j + 1 + b * nSample * nSample] += cross2;
				gNormals[(i + 1) * nSample + j + b * nSample * nSample] += cross2;
				gNormals[(i + 1) * nSample + j + 1 + b * nSample * nSample] += cross2;

				// construct faces
				int vInd[3];
				vInd[0] = (i * nSample) + j + b * nSample * nSample; //0
				vInd[1] = (i + 1) * nSample + j + b * nSample * nSample; // 4
				vInd[2] = (i * nSample) + j + 1 + b * nSample * nSample; // 1
				gFaces.push_back(Face(vInd, vInd, vInd));

				vInd[0] = (i + 1) * nSample + j + b * nSample * nSample; // 4
				vInd[1] = (i + 1) * nSample + j + 1 + b * nSample * nSample; // 5
				vInd[2] = (i * nSample) + j + 1 + b * nSample * nSample; // 1
				gFaces.push_back(Face(vInd, vInd, vInd));

			}

		}
	
	}

}

void calculateBezierNormals()
{
	// gNormals

	for (int i = 0; i < gFaces.size(); i += 3)
	{
		float v1 = bezierSampleVertices[gFaces[i].vIndex[0]].x;
		float v2 = bezierSampleVertices[gFaces[i].vIndex[1]].y;
		float v3 = bezierSampleVertices[gFaces[i].vIndex[2]].z;
	}
}

// parse obj file
bool ParseObj(const string& fileName)
{
	fstream myfile;

	// Open the input 
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			stringstream str(curLine);
			GLfloat c1, c2, c3;
			GLuint index[9];
			string tmp;

			if (curLine.length() >= 2)
			{
				if (curLine[0] == 'v')
				{
					if (curLine[1] == 't') // texture
					{
						str >> tmp; // consume "vt"
						str >> c1 >> c2;
						gTextures.push_back(Texture(c1, c2));
					}
					else if (curLine[1] == 'n') // normal
					{
						str >> tmp; // consume "vn"
						str >> c1 >> c2 >> c3;
						gNormals.push_back(Normal(c1, c2, c3));
					}
					else // vertex
					{
						str >> tmp; // consume "v"
						str >> c1 >> c2 >> c3;
						gVertices.push_back(Vertex(c1, c2, c3));
					}
				}
				else if (curLine[0] == 'f') // face
				{
					str >> tmp; // consume "f"
					char c;
					int vIndex[3], nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0];
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1];
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2];

					assert(vIndex[0] == nIndex[0] &&
						vIndex[1] == nIndex[1] &&
						vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

					gFaces.push_back(Face(vIndex, tIndex, nIndex));
				}
				else
				{
					cout << "Ignoring unidentified line in obj file: " << curLine << endl;
				}
			}

			//data += curLine;
			if (!myfile.eof())
			{
				//data += "\n";
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}

	assert(gVertices.size() == gNormals.size());

	return true;
}

// helps to read shader files
bool ReadDataFromFile(
	const string& fileName, ///< [in]  Name of the shader file
	string& data)     ///< [out] The contents of the file
{
	fstream myfile;

	// Open the input 
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			data += curLine;
			if (!myfile.eof())
			{
				data += "\n";
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}

	return true;
}

GLuint createVS(const char* shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar* shader = (const GLchar*)shaderSource.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &shader, &length);
	glCompileShader(vs);

	char output[1024] = { 0 };
	glGetShaderInfoLog(vs, 1024, &length, output);
	printf("VS compile log: %s\n", output);

	return vs;
}

GLuint createFS(const char* shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar* shader = (const GLchar*)shaderSource.c_str();

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &shader, &length);
	glCompileShader(fs);

	char output[1024] = { 0 };
	glGetShaderInfoLog(fs, 1024, &length, output);
	printf("FS compile log: %s\n", output);

	return fs;
}

void initShaders()
{
	// Create the programs

	gProgram[0] = glCreateProgram();
	gProgram[1] = glCreateProgram();

	// Create the shaders for both programs

	GLuint vs1 = createVS("Shaders/vert.glsl");
	GLuint fs1 = createFS("Shaders/frag.glsl");

	GLuint vs2 = createVS("Shaders/vert2.glsl");
	GLuint fs2 = createFS("Shaders/frag2.glsl");

	// Attach the shaders to the programs

	glAttachShader(gProgram[0], vs1);
	glAttachShader(gProgram[0], fs1);

	glAttachShader(gProgram[1], vs2);
	glAttachShader(gProgram[1], fs2);

	// Link the programs

	glLinkProgram(gProgram[0]);
	GLint status;
	glGetProgramiv(gProgram[0], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	glLinkProgram(gProgram[1]);
	glGetProgramiv(gProgram[1], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	// Get the locations of the uniform variables from both programs

	for (int i = 0; i < 2; ++i)
	{
		modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
		viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
		projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
		eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
		//lightIntesityLoc = glGetUniformLocation(gProgram[0], "lightIntesity");
		
		numberOfPointLightsLoc = glGetUniformLocation(gProgram[0], "numberOfPointLights");
		lightLoc[0][0] = glGetUniformLocation(gProgram[0], "lights[0].lightPosition");
		lightLoc[0][1] = glGetUniformLocation(gProgram[0], "lights[0].lightColor");


		lightLoc[1][0] = glGetUniformLocation(gProgram[0], "lights[1].lightPosition");
		lightLoc[1][1] = glGetUniformLocation(gProgram[0], "lights[1].lightColor");

		lightLoc[2][0] = glGetUniformLocation(gProgram[0], "lights[2].lightPosition");
		lightLoc[2][1] = glGetUniformLocation(gProgram[0], "lights[2].lightColor");

		lightLoc[3][0] = glGetUniformLocation(gProgram[0], "lights[3].lightPosition");
		lightLoc[3][1] = glGetUniformLocation(gProgram[0], "lights[3].lightColor");

		lightLoc[4][0] = glGetUniformLocation(gProgram[0], "lights[4].lightPosition");
		lightLoc[4][1] = glGetUniformLocation(gProgram[0], "lights[4].lightColor");
	}
}

void initBuffers()
{

	gVertexDataSizeInBytes = bezierSampleVertices.size() * 3 * sizeof(GLfloat);
	gNormalDataSizeInBytes = gNormals.size() * 3 * sizeof(GLfloat);
	int indexDataSizeInBytes = gFaces.size() * 3 * sizeof(GLuint);

	GLfloat* vertexData = new GLfloat[bezierSampleVertices.size() * 3];
	GLuint* indexData = new GLuint[gFaces.size() * 3];
	GLfloat* normalData = new GLfloat[gNormals.size() * 3];


	for (int i = 0; i < bezierSampleVertices.size(); i++)
	{
		vertexData[(3 * i)] = bezierSampleVertices[i].x;
		vertexData[(3 * i) + 1] = bezierSampleVertices[i].y;
		vertexData[(3 * i) + 2] = bezierSampleVertices[i].z;

		//cout << "x: " << vertexData[(3 * i)] << " y: " << vertexData[(3 * i) + 1] << " z: " << vertexData[(3 * i) + 2] << endl;
	}

	for (int i = 0; i < gFaces.size(); i++)
	{
		indexData[3 * i] = gFaces[i].vIndex[0];
		indexData[3 * i + 1] = gFaces[i].vIndex[1];
		indexData[3 * i + 2] = gFaces[i].vIndex[2];
	}

	for (int i = 0; i < gNormals.size(); i++)
	{
		normalData[3 * i] = gNormals[i].x;
		normalData[3 * i + 1] = gNormals[i].y;
		normalData[3 * i + 2] = gNormals[i].z;
	}

	

	glGenBuffers(1, &gVertexAttribBuffer);
	glGenBuffers(1, &gIndexBuffer);

	assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	
	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData);
	glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, normalData);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));


	delete[] vertexData;
	delete[] indexData;
	delete[] normalData;
}

void changeSampleSize()
{
	
	bezierPatches.clear();
	int numberOfPatch = ((verticalCPCount / 4) * (horizontalCPCount / 4));
	cerr << "Number of Patch " + numberOfPatch << endl;

	bezierPatches.resize(numberOfPatch);

	bezierSampleVertices.clear();
	gNormals.clear();
	gFaces.clear();

	createControlPoints();
	initBezierSampleVertices();
	generateBezierFaces();
	initBuffers();
}

void init(char** argv)
{
	//ParseObj("Models/armadillo.obj");
	//ParseObj("bunny.obj");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);

	//unsigned int VBO, VAO;
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	assert(VAO > 0);
	glBindVertexArray(VAO);
	cout << "vao = " << VAO << endl;

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	initShaders();

	if (!parseInputFile(argv[1]))
	{
		cout << "Failed to read input file" << endl;
		exit(-1);
	}

	/*vector<float> tempLine;
	tempLine = linSpace(0, 1, 10);*/

	createControlPoints();

	initBezierSampleVertices();
	generateBezierFaces();

	initBuffers();

}

void drawModel()
{
	glDrawElements(GL_TRIANGLES, gFaces.size() * 3, GL_UNSIGNED_INT, 0);
}

void display()
{
	glClearColor(0, 0, 0, 1);
	glClearDepth(1.0f);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//static float angle = -30;

	float angleRad = (float)(rotationAngle / 180.0) * M_PI;

	// Compute the modeling matrix
	glm::mat4 matT = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, 0.0f));
	//glm::mat4 matRx = glm::rotate<float>(glm::mat4(1.0), glm::radians(-30.0f), glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 matRx = glm::rotate<float>(glm::mat4(1.0), glm::radians(rotationAngle), glm::vec3(1.0, 0.0, 0.0));
	/*glm::mat4 matRy = glm::rotate<float>(glm::mat4(1.0), (-180. / 180.) * M_PI, glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 matRz = glm::rotate<float>(glm::mat4(1.0), angleRad, glm::vec3(0.0, 0.0, 1.0));*/

	//modelingMatrix = matT * matRz * matRy * matRx;
	modelingMatrix = matT * matRx;

	//modelingMatrix = matT;

	//viewingMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));


	// Set the active program and the values of its uniform variables
	glUseProgram(gProgram[0]);
	glUniformMatrix4fv(projectionMatrixLoc[activeProgramIndex], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(viewingMatrixLoc[activeProgramIndex], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
	glUniformMatrix4fv(modelingMatrixLoc[activeProgramIndex], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
	glUniform3fv(eyePosLoc[activeProgramIndex], 1, glm::value_ptr(eyePos));

	//glUniform3fv(lightIntesityLoc, 1, glm::value_ptr(lightSources[0].lightColor));
	glUniform3fv(lightLoc[0][0], 1, glm::value_ptr(lightSources[0].lightPosition));
	glUniform3fv(lightLoc[0][1], 1, glm::value_ptr(lightSources[0].lightColor));

	glUniform3fv(lightLoc[1][0], 1, glm::value_ptr(lightSources[1].lightPosition));
	glUniform3fv(lightLoc[1][1], 1, glm::value_ptr(lightSources[1].lightColor));

	glUniform3fv(lightLoc[2][0], 1, glm::value_ptr(lightSources[2].lightPosition));
	glUniform3fv(lightLoc[2][1], 1, glm::value_ptr(lightSources[2].lightColor));

	glUniform3fv(lightLoc[3][0], 1, glm::value_ptr(lightSources[3].lightPosition));
	glUniform3fv(lightLoc[3][1], 1, glm::value_ptr(lightSources[3].lightColor));

	glUniform3fv(lightLoc[4][0], 1, glm::value_ptr(lightSources[4].lightPosition));
	glUniform3fv(lightLoc[4][1], 1, glm::value_ptr(lightSources[4].lightColor));

	glUniform1i(numberOfPointLightsLoc, numberOfPointLights);

	//lightLoc[0][0] = glGetUniformLocation(gProgram[0], "lights[0].lightPosition");


	// Draw the scene
	drawModel();
	//rotationAngle += 10;

	/*if (rotationAngle >= 360.0f )
	{
		rotationAngle = -360.0f;
	}
	if (rotationAngle <= -360.0f)
	{
		rotationAngle = 360.0f;
	}*/

	//angle += .1;
	//cout << angle << endl;
}

void reshape(GLFWwindow* window, int w, int h)
{
	w = w < 1 ? 1 : w;
	h = h < 1 ? 1 : h;

	gWidth = w;
	gHeight = h;

	glViewport(0, 0, w, h);

	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();
	//glOrtho(-10, 10, -10, 10, -10, 10);
	//gluPerspective(45, 1, 1, 100);

	// Use perspective projection

	//float fovyRad = (float)(45.0 / 180.0) * M_PI;
	projectionMatrix = glm::perspective(glm::radians(45.0f), (float)w / (float)h, 1.0f, 100.0f);

	// Assume default camera position and orientation (camera is at
	// (0, 0, 0) with looking at -z direction and its up vector pointing
	// at +y direction)

	viewingMatrix = glm::mat4(1);
	// (position, direction, up vector)
	viewingMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	//viewingMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	//viewingMatrix = glm::translate(viewingMatrix, glm::vec3(0.0f, 0.0f, -2.0f));


	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		//glShadeModel(GL_SMOOTH);
		activeProgramIndex = 0;
	}
	else if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		//glShadeModel(GL_SMOOTH);
		activeProgramIndex = 1;
	}
	else if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		cout << "Wireframe" << endl;
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		//glShadeModel(GL_FLAT);
	}
	else if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		cout << "Solid" << endl;
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//glShadeModel(GL_FLAT);
	}
	// new added keys
	else if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		cout << "Pressed E, increase coordMultipler by 0.1 --> new multiplier "<< coordMultiplier << endl;
		coordMultiplier += 0.1;
		changeSampleSize();

	}
	else if (key == GLFW_KEY_D && action == GLFW_PRESS)
	{
		cout << "Pressed D, decrease coordMultipler by 0.1--> new multiplier " << coordMultiplier << endl;
		coordMultiplier -= 0.1;
		changeSampleSize();
	}
	else if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		cout << "Pressed R, increase rotationAngle by 10 --> new angle " << rotationAngle << endl;
		rotationAngle += 10;
	}
	else if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		cout << "Pressed F, decrease rotationAngle by 10--> new angle " << rotationAngle << endl;
		rotationAngle -= 10;
	}
	else if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		cout << "Pressed W, increase sampleSize by 2 --> new sampleSize" << nSample << endl;
		nSample += 2;
		changeSampleSize();
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		cout << "Pressed S, decrease sampleSize by 2--> new sampleSize " << nSample << endl;
		nSample -= 2;
		changeSampleSize();
	}
}

void mainLoop(GLFWwindow* window)
{
	double lastTime = glfwGetTime();
	int nbFrames = 0;

	while (!glfwWindowShouldClose(window))
	{
		double currentTime = glfwGetTime();
		nbFrames++;

		if (currentTime - lastTime >= 1.0) {
			string fr = " -- FPS: " + to_string(nbFrames) + " -- Sample: " + to_string(nSample) + " -- Patch: " + to_string(bezierPatches.size()) + " -- CoordM " + to_string(coordMultiplier);
			char rendererInfo[512] = { 0 };
			strcpy(rendererInfo, (const char*)glGetString(GL_RENDERER));
			(rendererInfo, (const char*)glGetString(GL_RENDERER));
			strcat(rendererInfo, " - ");
			strcat(rendererInfo, (const char*)glGetString(GL_VERSION));
			strcat(rendererInfo, fr.c_str());
			glfwSetWindowTitle(window, rendererInfo);
			nbFrames = 0;
			lastTime += 1.0;
		}

		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
	GLFWwindow* window;
	if (!glfwInit())
	{
		exit(-1);
	}

	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	int width = 800, height = 600;
	// @TODO: string title = add FPS sample and path number
	window = glfwCreateWindow(width, height, "CG HW1", NULL, NULL);


	if (!window)
	{
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	

	// Initialize GLEW to setup the OpenGL Function pointers
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	init(argv);

	// read input.txt file by arg


	glfwSetKeyCallback(window, keyboard);
	glfwSetWindowSizeCallback(window, reshape);

	reshape(window, width, height); // need to call this once ourselves
	mainLoop(window); // this does not return unless the window is closed

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
