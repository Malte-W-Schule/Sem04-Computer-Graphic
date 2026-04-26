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

float min(glm::vec3 input);
float max(glm::vec3 input);

cg::GLSLProgram program;

glm::mat4x4 view;
glm::mat4x4 projection;

float zNear = 0.1f;
float zFar  = 100.0f;

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


  std::vector<glm::vec3> colors = { color,color,color,color };
  initQuad(colors);
  initTriangle();
  return true;
}

/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderTriangle();
	renderQuad();
}

void glutDisplay ()
{
   render();
   glutSwapBuffers();
}

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
    // do something
    break;
  case '-':
    // do something
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
  }
  glutPostRedisplay();
}

void readInLoop() {
	std::string command;
	do {
		std::cout << "CMY, HSV für eine Umrechnung oder exit um zum render Loop der Formen zu kommen" << std::endl;
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
  //glutIdleFunc   (glutDisplay); // redisplay when idle
  
  glutKeyboardFunc(glutKeyboard);
  
  readInLoop();

  //Werte abfragen und in eine Matrix speichern
  // init vertex-array-objects.
  bool result = init();
  if (!result) {
    return -2;
  }

  // ======================================================================= TEST Start =======================================================================
  //RGBtoHSV(glm::vec3(0.4f, 0.09f, 0.1f));
  //CMYtoHSV(glm::vec3(0.6f, 0.77f, 0.75f));


  glm::vec3 rgb1 = glm::vec3(0.4f, 0.3f, 0.1f);
  std::cout << rgb1.x << "," << rgb1.y << "," << rgb1.z << std::endl;

  glm::vec3 cmy1 = RGBtoCMY(rgb1);
  glm::vec3 hsv1 = CMYtoHSV(cmy1);
  HSVtoRGB(hsv1);

  // ======================================================================= TEST End =======================================================================

  

  // GLUT: Loop until the user closes the window
  // rendering & event handling
  glutMainLoop ();
  
  // Cleanup in destructors:
  // Objects will be released in ~Object
  // Shader program will be released in ~GLSLProgram
  
  return 0;
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

    if (delta == 0)                     //This is a gray, no chroma...
    {
        h = 0;
        s = 0;
    }
    else                                    //Chromatic data...
    {
        float del_R = (((v - r) / 6.0f) + (delta / 2.0f)) / delta;
        float del_G = (((v - g) / 6.0f) + (delta / 2.0f)) / delta;
        float del_B = (((v - b) / 6.0f) + (delta / 2.0f)) / delta;

        if (r == v) {
            h = del_B - del_G;
        }
        else if (g == v) {
            h = (1.0f / 3.0f) + del_R - del_B;
        }
        else if (b == v) {
            h = (2.0f / 3.0f) + del_G - del_R;
        }

        if (h < 0.0f) {
            h += 1;
        }
        if (h > 1.0f) {
            h -= 1;
        }
    }

    h = h * 360;

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
        
       
        float var_h = (h/360.0f) * 6.0f;
        if (var_h == 6.0f) {
            var_h = 0;
        }      //H must be < 1

        var_i = int(var_h);             //Or ... var_i = floor( var_h )
        var_1 = v * (1.0f - s);
        var_2 = v * (1.0f - s * (var_h - var_i));
        var_3 = v * (1.0f - s * (1.0f - (var_h - var_i)));
        
        if (var_i == 0) { var_r = v; var_g = var_3; var_b = var_1; }
        else if (var_i == 1) { var_r = var_2; var_g = v; var_b = var_1; }
        else if (var_i == 2) { var_r = var_1; var_g = v; var_b = var_3; }
        else if (var_i == 3) { var_r = var_1; var_g = var_2; var_b = v; }
        else if (var_i == 4) { var_r = var_3; var_g = var_1; var_b = v; }
		else { var_r = v; var_g = var_1; var_b = var_2; }

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
