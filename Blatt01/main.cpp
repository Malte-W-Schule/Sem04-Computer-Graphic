#include <iostream>
#include <vector>

#include <GL/glew.h>
//#include <GL/gl.h> // OpenGL header not necessary, included by GLEW
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "GLSLProgram.h"
#include "GLTools.h"
#include <glm/gtx/rotate_vector.hpp>


// Standard window width
const int WINDOW_WIDTH  = 640;
// Standard window height
const int WINDOW_HEIGHT = 480;
// GLUT window id/handle
int glutID = 0;

glm::vec3 CMYtoRGB(glm::vec3 input);
glm::vec3 CMYtoHSV(glm::vec3 input);
glm::vec3 RGBtoHSV(glm::vec3 input);
glm::vec3 RGBtoCMY(glm::vec3 input);
glm::vec3 HSVtoRGB(glm::vec3 input);
glm::vec3 HSVtoCMY(glm::vec3 input);
void readInLoop();

float min(glm::vec3 input);
float max(glm::vec3 input);

cg::GLSLProgram program;

glm::mat4x4 view;
glm::mat4x4 projection;

float zNear = 0.1f;
float zFar  = 100.0f;

std::vector<GLushort> calcIndices(std::vector<glm::vec3> subTriangles);
std::vector<glm::vec3> calcSubDivideTriangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3);
std::vector<glm::vec3> calcSphereVertices(std::vector<glm::vec3> sphereVerticesWithoutSubdivision, std::vector<GLushort> sphereIndicesWithoutSubdivision, glm::vec3 center);
void renderSphere();
void initSphere(float size);
glm::vec3 spherePoint(float radius, float betaDegree, float lambdaDegree, glm::vec3 center);
int indexCount = 0;


// ================================================================================= Size =================================================================================
float size = 1;
int n = 2;

/*
Struct to hold data for object rendering.
*/
class Object
{
public:
  inline Object ()
    : vao(0),
      positionBuffer(0),
      colorBuffer(0),
      indexBuffer(0)
  {}

  inline ~Object () { // GL context must exist on destruction
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1, &colorBuffer);
    glDeleteBuffers(1, &positionBuffer);
  }

  GLuint vao;        // vertex-array-object ID
  
  GLuint positionBuffer; // ID of vertex-buffer: position
  GLuint colorBuffer;    // ID of vertex-buffer: color
  
  GLuint indexBuffer;    // ID of index-buffer
  
  glm::mat4x4 model; // model matrix
};

Object triangle;
Object quad;
Object sphere;

// ================================================================================= RENDER SPHERE =================================================================================
void renderSphere()
{   // Create mvp.
    glm::mat4x4 mvp = projection * view * sphere.model;

    // Bind the shader program and set uniform(s).
    program.use();
    program.setUniform("mvp", mvp);

    // Bind vertex array object so we can render the 1 triangle.
    glBindVertexArray(sphere.vao);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}
    

// ================================================================================= RENDER TRIANGLE =================================================================================
void renderTriangle()
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * triangle.model;
  
  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);
  
  // Bind vertex array object so we can render the 1 triangle.
  glBindVertexArray(triangle.vao);
  glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void renderQuad()
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * quad.model;
  
  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);
  
  // Bind vertex array object so we can render the 2 triangles.
  glBindVertexArray(quad.vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

// ================================================================================= INIT SPHERE =================================================================================
void initSphere(float size) {
    // start (0,0,0)
       //ecken, 5 | 4 mitte | 1 oben | 1 unten (start)

       // 0 0 0 | 1 1 1 | 1 1 -1 | -1 1 1 | -1 1 -1 | 0 2 0

    std::vector<glm::vec3> Skalierungsmatrix;

    // Die 6 Eckpunkte
    std::vector<glm::vec3> sphereVerticesold = { {
        { 0.0f,  1.0f,  0.0f}, // 0: Oben
        { 0.0f,  -1.0f,  0.0f}, // 1: Unten
        { -1.0f,  -0.1f,  1.0f}, // 2: Rechts
        {1.0f,  0.1f,  -1.0f}, // 3: Links
        { 1.0f,  -0.1f,  1.0f}, // 4: Vorne
        { -1.0f,  0.1f, -1.0f}  // 5: Hinten
    } };


    // === Startpunkt rotiert um 50°===

    std::vector<glm::vec3> sphereVerticesWithoutSubdivision = { 
    {  0.0f,       1.0f,   0.0f },       // 0: Oben (bleibt gleich, da auf der Drehachse)
    {  0.0f,      -1.0f,   0.0f },       // 1: Unten (bleibt gleich, da auf der Drehachse)
    {  0.245576f, -0.1f,   1.392728f },  // 2: Rechts
    { -0.245576f,  0.1f,  -1.392728f },  // 3: Links
    {  1.392728f, 0.0f,  -0.245576f },   // 4: Vorne
    { -1.392728f,  0.0f,   0.245576f }   // 5: Hinten
   };


    // 24 Indizes für die 8 Dreiecke 
    std::vector<GLushort> sphereIndicesWithoutSubdivision = {
        // Obere Hälfte
        0, 4, 2,
        0, 2, 5,
        0, 5, 3,
        0, 3, 4,

        // Untere Hälfte
        1, 2, 4,
        1, 5, 2,
        1, 3, 5,
        1, 4, 3 };

    std::cout << sphereIndicesWithoutSubdivision.size() << std::endl;
    std::vector<glm::vec3> sphereVertices;
    std::vector<GLushort> sphereIndices;

    sphereVertices = calcSphereVertices(sphereVerticesWithoutSubdivision, sphereIndicesWithoutSubdivision, glm::vec3 (0.0f, 0.0f, 0.0f));
    sphereIndices = calcIndices(sphereVertices);
    
    if (n == 0) {
        sphereVertices = sphereVerticesWithoutSubdivision;
        sphereIndices = sphereIndicesWithoutSubdivision;
    }
    
    indexCount = sphereIndices.size();
    
    for (glm::vec3& v : sphereVertices) v *= size;

    //alles Rot
    const std::vector<glm::vec3> colors = { {
        { 1.0f,  1.0f,  0.0f},
        { 1.0f,  0.0f,  0.0f}, 
        { 1.0f,  0.0f,  0.0f}, 
        { 1.0f,  0.0f,  0.0f}, 
        { 1.0f,  1.0f,  0.0f}, 
        { 0.0f,  1.0f,  0.0f}  
    } };


    GLuint programId = program.getHandle();
    GLuint pos;

    // Step 0: Create vertex array object.
    glGenVertexArrays(1, &sphere.vao);
    glBindVertexArray(sphere.vao);

    // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
    glGenBuffers(1, &sphere.positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere.positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(glm::vec3), sphereVertices.data(), GL_STATIC_DRAW);

    // Bind it to position.
    pos = glGetAttribLocation(programId, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Step 2: Create vertex buffer object for color attribute and bind it to...
    glGenBuffers(1, &sphere.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere.colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);

    // Bind it to color.
    pos = glGetAttribLocation(programId, "color");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Step 3: Create vertex buffer object for indices. No binding needed here.
    glGenBuffers(1, &sphere.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(GLushort), sphereIndices.data(), GL_STATIC_DRAW);

    // Unbind vertex array object (back to default).
    glBindVertexArray(0);

    // Modify model matrix.Kugel liegt jetzt im Mittelpunkt
    sphere.model = glm::mat4(1.0f);
}

std::vector<GLushort> calcIndices(std::vector<glm::vec3> subTriangles) {
    std::vector<GLushort> sphereIndicesWithSubdivision;

   int weirdIndexCounter = 0;

    for (int y = 0; y <= n; y++)
    {
        int countRow = (n + 2 - y);

        for (int x = 0; x < countRow - 1; x++)
        {
            int V1 = weirdIndexCounter + x;
            int V2 = V1 + countRow;
            int V3 = V1 + 1;

            // Dreieck nach unten
            sphereIndicesWithSubdivision.push_back(V1);
            sphereIndicesWithSubdivision.push_back(V2);
            sphereIndicesWithSubdivision.push_back(V3);

            // Dreieck nach oben
            if (x != 0)
            {
                int Vu1 = V1;
                int Vu2 = V2 - 1;
                int Vu3 = V2;

                sphereIndicesWithSubdivision.push_back(Vu1);
                sphereIndicesWithSubdivision.push_back(Vu2);
                sphereIndicesWithSubdivision.push_back(Vu3);
            }
        }

        weirdIndexCounter += countRow;
    }

   /* if (n == 1) {
        sphereIndicesWithSubdivision = {
            // Obere Hälfte
            0,1,4,
            0,4,3,
            3,4,6,
            4,5,6,
            1,5,4,
            1,2,5
            
            
         };
    
    }
    
    if (n == 2) {
        sphereIndicesWithSubdivision = {
        // Obere Hälfte
        0, 4, 1,
        1, 5, 2,
        2, 5, 6,
        2, 6, 3,

        // Untere Hälfte
        4, 7, 5,
        5, 7, 8,
        5, 8, 6,
        7, 9, 8 }; 
    }
    */
    return sphereIndicesWithSubdivision;
}

glm::vec3 spherePoint(float radius, float betaDegree, float lambdaDegree, glm::vec3 center)
{
    float beta = glm::radians(betaDegree);
    float lambda = glm::radians(lambdaDegree);

    float x = radius * sin(beta) * cos(lambda);
    float y = radius * cos(beta);
    float z = radius * sin(beta) * sin(lambda);

    return center + glm::vec3(x, y, z);
}

std::vector<glm::vec3> calcSphereVertices(std::vector<glm::vec3> sphereVerticesWithoutSubdivision, std::vector<GLushort> sphereIndicesWithoutSubdivision, glm::vec3 center)
{
    std::vector<glm::vec3> subTriangles;

    float degree = 90.0f / (n + 1);

        
   
    for (int k = 0; k < sphereIndicesWithoutSubdivision.size(); k += 3) {
        glm::vec3 v1 =
            sphereVerticesWithoutSubdivision[
                sphereIndicesWithoutSubdivision[k]
            ];

        glm::vec3 v2 =
            sphereVerticesWithoutSubdivision[
                sphereIndicesWithoutSubdivision[k + 1]
            ];

        glm::vec3 v3 =
            sphereVerticesWithoutSubdivision[
                sphereIndicesWithoutSubdivision[k + 2]
            ];
        float radius = glm::length(v1 - center);
       

        for (int i = 0; i <= n + 1; i++)
        {
            // von oben nach unten
            float beta = 90.0f - (i * degree);

            int triangleCountInRow = n + 2 - i;

            for (int j = 0; j < triangleCountInRow; j++)
            {
                // seitliche Bewegung
                float lambda = j * degree;

                glm::vec3 point =
                    spherePoint(radius, beta, lambda, center);

                subTriangles.push_back(point);
            }
        }
    }
    
    return subTriangles;
}

std::vector<glm::vec3> calcSubDivideTriangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3)
{
	std::vector<glm::vec3> subTriangles;

	// d ist die Anzahl der Segmente pro Kante (z.B. n=1 bedeutet d=2 Segmente)
	float d = static_cast<float>(n + 1);

	// richtungsvektoren ausgehend von V1
	glm::vec3 toTopLeftV    = (v2 - v1) / d; 
	glm::vec3 toLeftV       = (v3 - v1) / d; 
    glm::vec3 start;
    

	for (int i = 0; i <= n + 1 ; i++)
    {
        start = v1 + (float(i) * toTopLeftV);

		glm::vec3 rowStartV1 = v1 + (static_cast<float>(i) * toTopLeftV);
        
        int triangleCountInRow = n + 2 - i;

		for (int y = 0; y < triangleCountInRow; y++)
		{
            glm::vec3 currentPoint = start + (float(y) * toLeftV);
            subTriangles.push_back(currentPoint);
		}
        
	}

	return subTriangles;
}


// ================================================================================= INIT TRIANGLE =================================================================================
void initTriangle()  
{
    glm::vec3 cmy(0.0f, 1.0f, 1.0f);

  // Construct triangle. These vectors can go out of scope after we have send all data to the graphics card.
  const std::vector<glm::vec3> vertices = { glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f) };
  const std::vector<glm::vec3> colors   = { CMYtoRGB(cmy), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)}; // change float numbers between 0-1 to change color, Color Order is: Red/Green/Blue
  const std::vector<GLushort>  indices  = { 0, 1, 2 };

  GLuint programId = program.getHandle();
  GLuint pos;

  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &triangle.vao);
  glBindVertexArray(triangle.vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &triangle.positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triangle.positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &triangle.colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triangle.colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &triangle.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangle.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  // Modify model matrix.
  triangle.model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.25f, 0.0f, 0.0f));
}



// ================================================================================= INIT QUAD =================================================================================
void initQuad(std::vector<glm::vec3>& colors)
{
  // Construct triangle. These vectors can go out of scope after we have send all data to the graphics card.
  const std::vector<glm::vec3> vertices = { { -1.0f, 1.0f, 0.0f }, { -1.0, -1.0, 0.0 }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } };
  //const std::vector<glm::vec3> colors = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
  //const std::vector<glm::vec3> colors   = { { 1.0f, 0.0f, 0.0f }, CMYtoRGB(cmy), { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
  const std::vector<GLushort>  indices  = { 0, 1, 2, 0, 2, 3 };

  GLuint programId = program.getHandle();
  GLuint pos;
  
  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &quad.vao);
  glBindVertexArray(quad.vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &quad.positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad.positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &quad.colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad.colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &quad.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  // Modify model matrix.
  quad.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.25f, 0.0f, 0.0f));
}


// ================================================================================= INIT =================================================================================
/*
 Initialization. Should return true if everything is ok and false if something went wrong.
 */
bool init()
{
  // OpenGL: Set "background" color and enable depth testing.
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  
  // Construct view matrix.
  glm::vec3 eye(0.0f, 0.0f, 4.0f);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  
  view = glm::lookAt(eye, center, up);
  
  // Create a shader program and set light direction.
  if (!program.compileShaderFromFile("shader/simple.vert", cg::GLSLShader::VERTEX)) {
    std::cerr << program.log();
    return false;
  }
  
  if (!program.compileShaderFromFile("shader/simple.frag", cg::GLSLShader::FRAGMENT)) {
    std::cerr << program.log();
    return false;
  }
  
  if (!program.link()) {
    std::cerr << program.log();
    return false;
  }

  /*
  std::cout << "Gib drei Farbwerte ein (immer einen + enter):" << std::endl;
  // Create all objects.
  float a;
  std::cin >> a;
  float b;
  std::cin >> b;
  float c;
  std::cin >> c;
  glm::vec3 color(a, b, c);
  
  std::cout << "1 RGB, 2 CMY, 3 HSV, as input" << std::endl;
  int d;
  std::cin >> d;
  if (d == 1){
      // rgb
  }
  else if (d == 2) {
	  // cym
      color = CMYtoRGB(color);
  }
  else if (d == 3) {
	  // hsv
      color = HSVtoRGB(color);
  }
  */


  //std::vector<glm::vec3> colors = { color,color,color,color };
  //initQuad(colors);
  //initTriangle();

  initSphere(1.0f);
  return true;
}


// ================================================================================= RENDER =================================================================================
/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//renderTriangle();
	//renderQuad();
    renderSphere();
}

void glutDisplay ()
{
   render();
   glutSwapBuffers();
}


// ================================================================================= GLUT =================================================================================
/*
 Resize callback.
 */
void glutResize (int width, int height)
{
  // Division by zero is bad...
  height = height < 1 ? 1 : height;
  glViewport(0, 0, width, height);
  
  // Construct projection matrix.
  projection = glm::perspective(45.0f, (float) width / height, zNear, zFar);
}


// ================================================================================= GLUT Keyboard =================================================================================
/*
 Callback for char input.
 */
void glutKeyboard (unsigned char keycode, int x, int y)
{
  switch (keycode) {
  case 27: // ESC
    glutDestroyWindow ( glutID );
    return;
    
  case '+':
    if (n == 4) { break; }
    n += 1;
    //initSphere(1.0f); //Größe nicht verändern
    
    break;
  case '-':
	if (n == 0) { break; }
    n -= 1;
	//initSphere(1.0f); //Größe nicht verändern
    
    break;
  case 'x':
    // do something
    break;
  case 'y':
    // do something
    break;
  case 'z':
    // do something
    break;
  case 'r':
      if (size <= 0.5) {
          break;
      }
      size = size - 0.1;
	  initSphere(size);
	  break;
  case 'R':
      if(size>1.5){
          break;
      }
	  size = size + 0.1;
	  initSphere(size);
	  break;
  }
  glutPostRedisplay();
}



// ================================================================================= MAIN =================================================================================

int main(int argc, char** argv)
{
  // GLUT: Initialize freeglut library (window toolkit).
  glutInitWindowSize    (WINDOW_WIDTH, WINDOW_HEIGHT);
  glutInitWindowPosition(40,40);
  glutInit(&argc, argv);
  
  // GLUT: Create a window and opengl context (version 4.3 core profile).
  glutInitContextVersion(4, 3);
  glutInitContextFlags  (GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
  glutInitDisplayMode   (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  
  glutCreateWindow("Aufgabenblatt 01");
  glutID = glutGetWindow();
  
  // GLEW: Load opengl extensions
  //glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    return -1;
  }
#if _DEBUG
  if (glDebugMessageCallback) {
    std::cout << "Register OpenGL debug callback " << std::endl;
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(cg::glErrorVerboseCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE,
			  GL_DONT_CARE,
			  GL_DONT_CARE,
			  0,
			  nullptr,
			  true); // get all debug messages
  } else {
    std::cout << "glDebugMessageCallback not available" << std::endl;
  }
#endif

  // GLUT: Set callbacks for events.
  glutReshapeFunc(glutResize);
  glutDisplayFunc(glutDisplay);
  glutIdleFunc   (glutDisplay); // redisplay when idle
  
  glutKeyboardFunc(glutKeyboard);
  
  //readInLoop();

  //Werte abfragen und in eine Matrix speichern
  // init vertex-array-objects.
  bool result = init();
  if (!result) {
    return -2;
  }

  // GLUT: Loop until the user closes the window
  // rendering & event handling
  glutMainLoop ();
  
  // Cleanup in destructors:
  // Objects will be released in ~Object
  // Shader program will be released in ~GLSLProgram
  
  return 0;
}


// ======================================================================= Blatt01 Start =======================================================================
void readInLoop() {
	std::string command;
	do {
		std::cout << "CMY, HSV für eine Umrechnung oder exit, um zum render Loop der Formen zu kommen" << std::endl;
		std::cin >> command;
		if (command == "CMY") {
			std::cout << "Gib die Werte für c, m, y ein" << std::endl;
			float c;
			std::cin >> c;
			float m;
			std::cin >> m;
			float y;
			std::cin >> y;

			glm::vec3 input(c, m, y);

			//CMYtoRGB(input);
			CMYtoHSV(input);
		}
		if (command == "HSV") {
			std::cout << "Gib die Werte für r, g, b ein" << std::endl;
			float r;
			std::cin >> r;
			float g;
			std::cin >> g;
			float b;
			std::cin >> b;

			glm::vec3 input(r, g, b);
			//HSVtoRGB(input);
			HSVtoCMY(input);
		}
	} while (command != "exit");

}


glm::vec3 CMYtoRGB(glm::vec3 input) {
    float c = input.x;
    float m = input.y;
    float y = input.z;

    float r = 1 - c;
    float g = 1 - m;
    float b = 1 - y;

    std::cout << "========= Ausgabe (CMYtoRGB) =========" << std::endl;
    std::cout << r << "," << g << "," << b << std::endl;


    return glm::vec3(r, g, b);
}


glm::vec3 CMYtoHSV(glm::vec3 input) {
    glm::vec3 rgb = CMYtoRGB(input);
    return RGBtoHSV(rgb);
}

glm::vec3 RGBtoHSV(glm::vec3 input) {
    float r = input.x;
    float g = input.y;
    float b = input.z;

    float minV = min(input);
    float maxV = max(input);

    float v = maxV;

    float delta = (maxV - minV);
    float s = delta / maxV;

    float h = 0;

    if (delta == 0) {
        h = 0;
        s = 0;
    }
    else {
        if (v == r) {
            h = 60 * ((g - b) / delta);
        }
        else if (v==g) {
            h = 60 * (2 + (b - r) / delta);
        }
        else {
            h = 60 * (4 + (r - g) / delta);
        }
        if (h < 0) {        //h is a minus value
            h = h + 360.0f;  
        }
    }

    
    std::cout << "========== Ausgabe HSV (RGBtoHSV)===========" << std::endl;
    std::cout << h << "," << s << "," << v << std::endl;

    return glm::vec3(h, s, v);
}

glm::vec3 HSVtoRGB(glm::vec3 input) {

  
	//H, S and V input range = 0 ÷ 1.0
	//R, G and B output range = 0 ÷ 255
    // h s v
    float h = input.x;
    float s = input.y;
    float v = input.z;


	float r = 0;
	float g = 0;
	float b = 0;

    float var_h = 0;

	int var_i = 0;
	float var_r = 0;
	float var_g = 0;
	float var_b = 0;
	float var_1 = 0;
	float var_2 = 0;
	float var_3 = 0;

	if (s == 0)
	{
         r = v;
         g = v;
         b = v;
	}
	else
	{
        
       
        float var_h = (h/360.0f) * 6.0f;    //dividing hue in 6 circle segments
        if (var_h == 6.0f) {    //360 degree = 0 degree
            var_h = 0;
        }      //H must be < 1

        var_i = int(var_h);             //Or ... var_i = floor( var_h ); cuts of the decimal numbers
        var_1 = v * (1.0f - s);                             //-> p dominant color
        var_2 = v * (1.0f - s * (var_h - var_i));           //-> q ; f = var_h - var_i: extracts the decimal number, fading color
        var_3 = v * (1.0f - s * (1.0f - (var_h - var_i)));  //-> t, intensifying color
        
        if (var_i == 0) { var_r = v; var_g = var_3; var_b = var_1; }        //red -> yellow
        else if (var_i == 1) { var_r = var_2; var_g = v; var_b = var_1; }   //yellow -> green
        else if (var_i == 2) { var_r = var_1; var_g = v; var_b = var_3; }   //green -> cyan
        else if (var_i == 3) { var_r = var_1; var_g = var_2; var_b = v; }   //cyan -> blue
        else if (var_i == 4) { var_r = var_3; var_g = var_1; var_b = v; }   //blue -> magenta
		else { var_r = v; var_g = var_1; var_b = var_2; }                   //magenta -> red

        r = var_r;
        g = var_g;
        b = var_b;
	}

    std::cout << "=========== Ausgabe (HSVtoRGB) ===========" << std::endl;
    std::cout << r << "," << g << "," << b << std::endl;

    return glm::vec3(r, g, b);
}

glm::vec3 HSVtoCMY(glm::vec3 input){
    glm::vec3 rgb = HSVtoRGB(input);
    return RGBtoCMY(rgb);
}

glm::vec3 RGBtoCMY(glm::vec3 input) {
	
    float r = input.x;
	float g = input.y;
	float b = input.z;

	float c = 1 - r;
	float m = 1 - g;
	float y = 1 - b;

	std::cout << "========= Ausgabe (RGBtoCMY) =========" << std::endl;
	std::cout << c << "," << m << "," << y << std::endl;

	return glm::vec3(c, m, y);
}

float min(glm::vec3 input) {
    if (input.x <= input.y && input.x <= input.z) {
        return input.x;
    }
    else if (input.y <= input.x && input.y <= input.z) {
        return input.y;
    }
    else {
        return input.z;
    }
}

float max(glm::vec3 input) {
    if (input.x >= input.y && input.x >= input.z) {
        return input.x;
    }
    else if (input.y >= input.x && input.y >= input.z) {
        return input.y;
    }
    else {
        return input.z;
    }
}

// ======================================================================= Blatt01 End =======================================================================


// ======================================================================= Blatt0 Start =======================================================================


