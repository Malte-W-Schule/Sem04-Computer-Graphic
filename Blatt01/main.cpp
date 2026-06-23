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


// ================================================================================= Size =================================================================================
float size = 1;
int n = 10;

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
<<<<<<< Updated upstream
=======
Object normales;
Object koords;



// ================================================================================= SPHERE Klasse =================================================================================


static void initShader(cg::GLSLProgram* program, const std::string& vertPath, const std::string& fragPath)
{
	if (!program->compileShaderFromFile(vertPath.c_str(), cg::GLSLShader::VERTEX))
	{
		throw std::runtime_error("COMPILE VERTEX: " + program->log());
	}

	if (!program->compileShaderFromFile(fragPath.c_str(), cg::GLSLShader::FRAGMENT))
	{
		throw std::runtime_error("COMPILE FRAGMENT: " + program->log());
	}

	if (!program->link())
	{
		throw std::runtime_error("LINK: " + program->log());
	}
}

class MySphere {
private:
	int n; // subdivisions
	int indexCount;

	// OpenGL Puffer-IDs für die Kugel
	GLuint vao;
	GLuint positionBuffer;
	GLuint colorBuffer;
	GLuint indexBuffer;

	// OpenGL Puffer-IDs für die Achsen (NEU)
	GLuint axisVao;
	GLuint axisPositionBuffer;
	GLuint axisColorBuffer;
	GLuint axisIndexBuffer;

	cg::GLSLProgram program;
	cg::GLSLProgram programAxis;

public:

    glm::vec3 center;
	// Die Modellmatrix gehört zum Objekt

    glm::mat4 translationModel; // wo ist planet position
   
    glm::mat4 AxisInclinedModel;    // wie ist achse gedreht
    
    glm::mat4 SphereRotationModel; // 

    glm::mat4 SphereModel;      // alles zsm

    bool has_axis;
    float degree;
    float r;
    float orbitAngle;
    
   
	// Konstruktor
	MySphere() : vao(0), positionBuffer(0), colorBuffer(0), indexBuffer(0), indexCount(0), n(3) { }

	void init(int subdivisions, float radius, float x, float y, float z, glm::vec3 color, float initialDegree, bool has_axis = false, float orbitAngle = 0.0f) {
		this->n = subdivisions;
		this->has_axis = has_axis;
        this->degree = initialDegree;
        this->r = radius;
        center = glm::vec3(x, y, z);
        this->orbitAngle = orbitAngle;

		initShader(&program, "shader/shadedPhong.vert", "shader/shadedPhong.frag");
		initShader(&programAxis, "shader/simple.vert", "shader/simple.frag");
		program.use();
		program.setUniform("light", lights[lightIndex]);
		program.setUniform("lightI", float(1.0f));
		program.setUniform("surfKa", glm::vec3(0.1f, 0.1f, 0.1f));
		program.setUniform("surfKd", glm::vec3(0.7f, 0.1f, 0.1f));
		program.setUniform("surfKs", glm::vec3(1, 1, 1));
		program.setUniform("surfShininess", float(8.0f));
		
		GLuint programId = programAxis.getHandle();


		std::vector<glm::vec3> StartVertices = {
			{ 0.0f,  radius,  0.0f}, // Oben
			{ 0.0f, -radius,  0.0f}, // Unten
			{ radius,  0.0f,  0.0f}, // Rechts
			{-radius,  0.0f,  0.0f}, // Links
			{ 0.0f,  0.0f,  radius}, // Vorne
			{ 0.0f,  0.0f, -radius}  // Hinten
		};

		std::vector<GLushort> StartIndices = {
			// untere hälfte
			0, 4, 2,  0, 2, 5,  0, 5, 3,  0, 3, 4,
			// Obere hälfte
			1, 2, 4,  1, 5, 2,  1, 3, 5,  1, 4, 3
		};

		glm::vec3 center(0.0f, 0.0f, 0.0f);
		std::vector<glm::vec3> currentVertices = calcSphereVertices(this->n, StartVertices, StartIndices, center);
		std::vector<GLushort> currentIndices = calcIndices(this->n, currentVertices);

		this->indexCount = currentIndices.size();

		for (glm::vec3& v : currentVertices) v *= radius;
		std::vector<glm::vec3> colors(currentVertices.size(), color);


		// ==========================================
		// 1. KUGEL INITIALISIEREN
		// ==========================================
		if (vao != 0) {
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &positionBuffer);
			glDeleteBuffers(1, &colorBuffer);
			glDeleteBuffers(1, &indexBuffer);
		}

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &positionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glBufferData(GL_ARRAY_BUFFER, currentVertices.size() * sizeof(glm::vec3), currentVertices.data(), GL_STATIC_DRAW);

		GLuint pos = glGetAttribLocation(programId, "position");
		glEnableVertexAttribArray(pos);
		glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &colorBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
		GLuint col = glGetAttribLocation(programId, "color");
		glEnableVertexAttribArray(col);
		glVertexAttribPointer(col, 3, GL_FLOAT, GL_FALSE, 0, 0);
		
		glGenBuffers(1, &indexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, currentIndices.size() * sizeof(GLushort), currentIndices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);

		// ==========================================
		// 2. ACHSEN INITIALISIEREN (EIGENE PUFFER!)
		// ==========================================
		if (has_axis) {

			GLuint programAxisId = programAxis.getHandle();
			std::vector<glm::vec3> axesVertices;
			std::vector<GLushort> axesIndices = { 0, 1 };

			// Y-Achse (oben und unten)
			glm::vec3 posAxes = (StartVertices[0] - center) * 2.0f + center;
			glm::vec3 negAxes = (StartVertices[1] - center) * 2.0f + center;

			axesVertices.push_back(posAxes);
			axesVertices.push_back(negAxes);

			std::vector<glm::vec3> colorAxes = { glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };

			if (axisVao != 0) {
				glDeleteVertexArrays(1, &axisVao);
				glDeleteBuffers(1, &axisPositionBuffer);
				glDeleteBuffers(1, &axisColorBuffer);
				glDeleteBuffers(1, &axisIndexBuffer);
			}

			glGenVertexArrays(1, &axisVao);
			glBindVertexArray(axisVao);


			GLuint pos = glGetAttribLocation(programAxisId, "position");
			glGenBuffers(1, &axisPositionBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, axisPositionBuffer);
			glBufferData(GL_ARRAY_BUFFER, axesVertices.size() * sizeof(glm::vec3), axesVertices.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(pos);
			glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

			GLuint col = glGetAttribLocation(programAxisId, "color");
			glGenBuffers(1, &axisColorBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, axisColorBuffer);
			glBufferData(GL_ARRAY_BUFFER, colorAxes.size() * sizeof(glm::vec3), colorAxes.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(col);
			glVertexAttribPointer(col, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glGenBuffers(1, &axisIndexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisIndexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, axesIndices.size() * sizeof(GLushort), axesIndices.data(), GL_STATIC_DRAW);

			glBindVertexArray(0);

		}
        
		// Modell-Matrix setzen (wird für Kugel und Achse gemeinsam genutzt)
		this->translationModel = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
        this->AxisInclinedModel = glm::rotate(glm::mat4(1.0f), glm::radians(initialDegree), glm::vec3(0.0f, 0.0f, 1.0f));
        this->SphereModel = translationModel * AxisInclinedModel;
	}

	//setze die kugel + achse auf die neue koordinate
    void setNewCoordinatesForCenter(float x, float y, float z) {
       this->center = glm::vec3(x,y,z);

    }

	// Render-Funktion direkt in der Klasse
	void render(const glm::mat4& projection, const glm::mat4& view) {

		// Gemeinsame MVP Matrix für das Objekt
		glm::mat4 mvp = projection * view * this->SphereModel;
		glm::mat3 nm = glm::inverseTranspose(glm::mat3(this->SphereModel));

		program.use();
		program.setUniform("modelviewMatrix", mvp);
		program.setUniform("projectionMatrix", projection);
		program.setUniform("normalMatrix", nm);
		//program.setUniform("light", lights[lightIndex]);

		// --- KUGEL ZEICHNEN ---
		glBindVertexArray(vao);
		if (isDepictionSolid) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

		// --- ACHSEN ZEICHNEN ---
		if (this->has_axis) {
			glm::mat4 mvpAxis = projection * view * this->SphereModel;
			programAxis.use();
			programAxis.setUniform("mvp", mvpAxis);
			
			// HIER BINDEN WIR DAS EIGENE VAO DER ACHSE
			glBindVertexArray(axisVao);
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			
			glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, 0);
		}

		// Clean-up
		glBindVertexArray(0);
	}

	
};


/*
    //berechne den kreis auf dem bewegt wird

    Params:
    - middle        (MySphere object which gets rotated around)
    - toRotate      (MySphere object which gets rotated)
    - orbitLengh    (distance between both objects)
    - speed:        (rotation speed)
*/
void rotateVectorFromSphere(MySphere& middle, MySphere& toRotate, float orbitLength, float speed) {

	// 1. Den absoluten Winkel des Mondes (auf seiner Umlaufbahn) aktualisieren
	toRotate.orbitAngle += speed;
	if (toRotate.orbitAngle >= 360.0f) {
		toRotate.orbitAngle -= 360.0f;
	}

	// 2. Basis-Vektor erstellen (Abstand auf der X-Achse im lokalen Raum)
	glm::vec3 baseVector(orbitLength, 0.0f, 0.0f);

	// degree (axe) winkel holen um vector darum zu drehen
	// 3. Auf der flachen Umlaufbahn rotieren (um die lokale Y-Achse)
	glm::vec3 localOrbitVector = glm::rotate(baseVector, glm::radians(toRotate.orbitAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    
	// 4. Die Achse des 'middle'-Objekts übernehmen
	// multiplizieren den flachen Orbit-Vektor mit der Achsen-Rotationsmatrix des Planeten.
	// Wichtig: Wir nutzen als 4. Komponente '0.0f', da es sich um einen Richtungsvektor und keinen Punkt handelt!
	glm::vec3 worldOrbitVector = glm::vec3(middle.AxisInclinedModel * glm::vec4(localOrbitVector, 0.0f));

	// 5. Den finalen, gekippten Vektor auf die aktuelle Welt-Position des Planeten addieren
	glm::vec3 newCenter = middle.center + worldOrbitVector;

	// 6. Position im Mond-Objekt speichern
	toRotate.setNewCoordinatesForCenter(newCenter.x, newCenter.y, newCenter.z);

	// 7. Matrizen aktualisieren
	toRotate.translationModel = glm::translate(glm::mat4(1.0f), toRotate.center);
	toRotate.SphereModel = toRotate.translationModel * toRotate.SphereRotationModel * toRotate.AxisInclinedModel;
}




// ================================================================================= Ende MySPHERE =================================================================================

>>>>>>> Stashed changes

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
    glDrawElements(GL_TRIANGLES, (n+1)*(n+1)*3 , GL_UNSIGNED_SHORT, 0);
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


    std::cout << "hallo" << std::endl;
    
    std::vector<glm::vec3> sphereVertices = calcSubDivideTriangle(sphereVerticesold[0], sphereVerticesold[2], sphereVerticesold[4]);
    std::vector<GLushort> sphereIndices = calcIndices(sphereVertices);
    
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
    std::vector<GLushort> sphereIndicesWithoutSubdivision;

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
            sphereIndicesWithoutSubdivision.push_back(V1);
            sphereIndicesWithoutSubdivision.push_back(V2);
            sphereIndicesWithoutSubdivision.push_back(V3);

            // Dreieck nach oben
            if (x != 0)
            {
                int Vu1 = V1;
                int Vu2 = V2 - 1;
                int Vu3 = V2;

                sphereIndicesWithoutSubdivision.push_back(Vu1);
                sphereIndicesWithoutSubdivision.push_back(Vu2);
                sphereIndicesWithoutSubdivision.push_back(Vu3);
            }
        }

        weirdIndexCounter += countRow;
    }

    /*
    if (n == 2) {
        sphereIndicesWithoutSubdivision = {
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
    }*/

    return sphereIndicesWithoutSubdivision;
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

MySphere sun;
MySphere planet_right;
MySphere planet_left;
MySphere planet_tmp;
MySphere moon_right;
MySphere moon_left;

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
  glDepthFunc(GL_LEQUAL);
  
  // Construct view matrix.
  glm::vec3 eye(0.0f, 0.0f, 4.0f);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  
  view = glm::lookAt(eye, center, up);
<<<<<<< Updated upstream
  
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
=======


  // =======================================================================INit sphre auf crack:
  //MySphere mySphere; // Dein neues globales Kugel-Objekt
  float currentRadius = 0.5f; // Optional: um Radius für die Tastatur zu speichern
  int currentSubdivisions = 3; // Optional: um n für die Tastatur zu speichern

  // Klasse aufrufen: n=3, radius=0.5, x=0, y=0, z=0, color=gelb
  sun.init(currentSubdivisions, currentRadius, 0.0f, 0.0f, 0.0f, glm::vec3(1.0f, 1.0f, 0.0f), 0.0f, true);

  planet_right.init(currentSubdivisions, 0.3f, 2.0f, 0.0f, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f), 45.0f, true);
 
  planet_left.init(currentSubdivisions, 0.3f, -2.0f, 0.0f, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, true,180.0f);
  
  moon_right.init(currentSubdivisions, 0.1f, 2.5f, 0.0f, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f), 0.0f, false);
 
  moon_left.init(currentSubdivisions, 0.1f, -2.1f, 0.0f, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f), 0.0f, false);


>>>>>>> Stashed changes

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

<<<<<<< Updated upstream
	//renderTriangle();
	//renderQuad();
    renderSphere();
=======
	int currentTime = glutGet(GLUT_ELAPSED_TIME);

	float deltaTime = (currentTime - lastTime) / 1000.0f;

	lastTime = currentTime;

	// ==========================================
	// 1. ANIMATION / UPDATE PHASE
	// ==========================================

    planet_left.AxisInclinedModel = glm::rotate(planet_left.AxisInclinedModel, glm::radians((deltaTime * planetSpeed * 60.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
    planet_right.AxisInclinedModel = glm::rotate(planet_right.AxisInclinedModel, glm::radians((deltaTime * planetSpeed * 60.0f)), glm::vec3(0.0f, 1.0f, 0.0f));

    rotateVectorFromSphere(sun, planet_right, 2.0f, (deltaTime * planetSpeed * 10.0f));
    rotateVectorFromSphere(sun, planet_left, 2.0f, (deltaTime * planetSpeed * 10.0f));

    rotateVectorFromSphere(planet_left, moon_left, 0.5f, -2*(deltaTime * planetSpeed * 60.0f));
    rotateVectorFromSphere(planet_right, moon_right, 0.5f, (deltaTime * planetSpeed * 60.0f));

    //planet_right.axisRotationModel = glm::rotate(planet_right.axisRotationModel, glm::radians(-0.05f), glm::vec3(0.0f, 1.0f, 0.0f));
    
	// ==========================================
	// 2. ZEICHNEN PHASE
	// ==========================================
	sun.render(projection, view);
    
	planet_right.render(projection, view);
	planet_left.render(projection, view);

	// (Monde erstmal weggelassen, siehe Frage unten)
	moon_right.render(projection, view);
	moon_left.render(projection, view);
>>>>>>> Stashed changes
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


