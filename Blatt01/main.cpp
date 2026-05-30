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

bool normalen = false;

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

int xRotation = 0;
int yRotation = 0;
int zRotation = 0;

std::vector<GLushort> calcIndices(int n, std::vector<glm::vec3> subTriangles);
std::vector<glm::vec3> calcSphereVertices(int n, std::vector<glm::vec3> sphereVerticesWithoutSubdivision, std::vector<GLushort> sphereIndicesWithoutSubdivision, glm::vec3 center);
void renderSphere();
//void initSphere();
void renderNormales();
void renderKoords();
int indexCount = 0;
int indexCountNormals = 0;
float CZoom = 4.0f;

class MySphere;


// ================================================================================= Size =================================================================================

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
Object normales;
Object koords;



// ================================================================================= SPHERE Klasse =================================================================================


class MySphere {
private:
	int n; // subdivisions
	int indexCount;

	// OpenGL Puffer-IDs
	GLuint vao;
	GLuint positionBuffer;
	GLuint colorBuffer;
	GLuint indexBuffer;

public:
	// Die Modellmatrix gehört zum Objekt
	glm::mat4 model;

	// Konstruktor
	MySphere() : vao(0), positionBuffer(0), colorBuffer(0), indexBuffer(0), indexCount(0), n(3), model(glm::mat4(1.0f)) {}

	// Initialisierung
	void init(int subdivisions, float radius, float x, float y, float z, glm::vec3 color, GLuint programId) {
		this->n = subdivisions;

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
			0, 4, 2,
			0, 2, 5,
			0, 5, 3,
			0, 3, 4,

			// Obere hälfte
			1, 2, 4,
			1, 5, 2,
			1, 3, 5,
			1, 4, 3
		};

		glm::vec3 center(0.0f, 0.0f, 0.0f);
		std::vector<glm::vec3> currentVertices = calcSphereVertices(this->n, StartVertices, StartIndices, center);
		std::vector<GLushort> currentIndices = calcIndices(this->n, currentVertices);

		this->indexCount = currentIndices.size();

		// Falls Radius in calcSphereVertices nicht verrechnet wird:
		for (glm::vec3& v : currentVertices) v *= radius;

		std::vector<glm::vec3> colors(currentVertices.size(), color);

		// Alte Puffer löschen, falls init() neu aufgerufen wird (z.B. bei n++ durch Tastatur)
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

		this->model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
	}

	// Render-Funktion direkt in der Klasse
	void render(cg::GLSLProgram& program, const glm::mat4& projection, const glm::mat4& view) {

		glm::mat4 mvp = projection * view * this->model;

		program.use();
		program.setUniform("mvp", mvp);

		glBindVertexArray(vao);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
		glBindVertexArray(0);
	}
};


// ================================================================================= Ende MySPHERE =================================================================================


MySphere sun;


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

void renderNormales() {
	glm::mat4x4 mvp = projection * view * normales.model;

	// Bind the shader program and set uniform(s).
	program.use();
	program.setUniform("mvp", mvp);

	// Bind vertex array object so we can render the 1 triangle.
	glBindVertexArray(normales.vao);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_LINES, indexCountNormals, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}
    
void renderKoords() {

    glm::mat4x4 mvp = projection * view * koords.model;

    // Bind the shader program and set uniform(s).
    program.use();
    program.setUniform("mvp", mvp);

    // Bind vertex array object so we can render the 1 triangle.
    glBindVertexArray(koords.vao);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);
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
/*void initSphere() {
   
   /* std::vector<glm::vec3> Skalierungsmatrix;

   // Die 6 Eckpunkte
    std::vector<glm::vec3> sphereVerticesold = { {
        { 0.0f,  1.0f,  0.0f}, // 0: Oben
        { 0.0f,  -1.0f,  0.0f}, // 1: Unten
        { -1.0f,  0.0f,  1.0f}, // 2: VORNE
        {1.0f,  0.0f,  -1.0f}, // 3: Hinten
        { 1.0f,  0.0f,  1.0f}, // 4: Rechts
        { -1.0f,  0.0f, -1.0f}  // 5: Links
    } };


    // === Startpunkt rotiert um 50°===

    std::vector<glm::vec3> sphereVerticesWithoutSubdivision = {
    {  0.0f,       1.0f,   0.0f },       // 0: Oben (bleibt gleich, da auf der Drehachse)
    {  0.0f,      -1.0f,   0.0f },       // 1: Unten (bleibt gleich, da auf der Drehachse)
    {  0.245576f, -0.f,   1.392728f },  // 2: VORNE
    { -0.245576f,  0.1f,  -1.392728f },  // 3: Hinten
    {  1.392728f, 0.0f,  -0.245576f },   // 4: Rechts
    { -1.392728f,  0.0f,   0.245576f }   // 5: Links
    };
    
    // 
    sphereVerticesWithoutSubdivision = sphereVerticesold;

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


    std::vector<glm::vec3> sphereVertices;
    std::vector<GLushort> sphereIndices;

    sphereVertices = calcSphereVertices(sphereVerticesWithoutSubdivision, sphereIndicesWithoutSubdivision, glm::vec3(0.0f, 0.0f, 0.0f));
    sphereIndices = calcIndices(sphereVertices);

    if (n == 0) {
        sphereVertices = sphereVerticesWithoutSubdivision;
        sphereIndices = sphereIndicesWithoutSubdivision;
    }

    indexCount = sphereIndices.size();
    indexCountNormals = sphereVertices.size() * 2;

    for (glm::vec3& v : sphereVertices) v *= size;


    std::vector<glm::vec3> colors;
    // Für jeden generierten Punkt exakt einen Farbwert anlegen
    for (size_t i = 0; i < sphereVertices.size(); i++) {
        colors.push_back(glm::vec3(1.0f, 1.0f, 0.0f)); // Alles Gelb
    }*/




    //======================================================================================================================================================
    /*
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
   

    // ============================================================================== Koordinaten ==============================================================================

   // X rot (um -5 Grad um Y gedreht)
	//glm::vec3 minusXline(0.0f, 0.0f, 0.0f);
	//glm::vec3 plusXline(1.0f, 0.0f, 0.0f);
     
	//Y blau (bleibt bei Y-Rotation völlig unverändert)
	glm::vec3 minusYline(0.0f, -1.0f, 0.0f);
	glm::vec3 plusYline(0.0f, 1.0f, 0.0f);

	// Z lila/magenta (um -5 Grad um Y gedreht)
	//glm::vec3 minusZline(0.0f, 0.0f, 0.0f);
	//glm::vec3 plusZline(0.0f, 0.0f, 1.0f);

    std::vector<glm::vec3> koordList;

	//koordList.push_back(minusXline);//0
	//koordList.push_back(plusXline);//1

	koordList.push_back(minusYline);//2
	koordList.push_back(plusYline);

    //koordList.push_back(minusZline);
    //koordList.push_back(plusZline);

    std::vector<GLushort> koordIndiceList = { 0, 1, 2, 3, 4, 5 };


    std::vector<glm::vec3> koordColors = {
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 1.0f, 0.0f) ,
        glm::vec3(0.0f, 1.0f, 0.0f)};



    // Step 0: Create vertex array object.
    glGenVertexArrays(1, &koords.vao);
    glBindVertexArray(koords.vao);

    // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
    glGenBuffers(1, &koords.positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, koords.positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, koordList.size() * sizeof(glm::vec3), koordList.data(), GL_STATIC_DRAW);

    // Bind it to position.
    pos = glGetAttribLocation(programId, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Step 2: Create vertex buffer object for color attribute and bind it to...
    glGenBuffers(1, &koords.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, koords.colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, koordColors.size() * sizeof(glm::vec3), koordColors.data(), GL_STATIC_DRAW);

    // Bind it to color.
    pos = glGetAttribLocation(programId, "color");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Step 3: Create vertex buffer object for indices. No binding needed here.
    glGenBuffers(1, &koords.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, koords.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, koordIndiceList.size() * sizeof(GLushort), koordIndiceList.data(), GL_STATIC_DRAW);

    // Unbind vertex array object (back to default).
    glBindVertexArray(0);

    // Modify model matrix.Kugel liegt jetzt im Mittelpunkt
	koords.model = glm::mat4(1.0f);
    
        
    // ============================================================================== normalen ==============================================================================
    
	std::vector<glm::vec3> normalenListe;
	std::vector<GLushort> normalenIndicesListe;
        for (auto& a : sphereVertices)
        {
        //center

            glm::vec3 center(0.0f, 0.0f, 0.0f);
            // richtungsvektor berechnen
            glm::vec3 normalePunkt = a + (a - center);

            // linie malen von a zu normalePunkt
            int lenght = normalenIndicesListe.size(); // bei leer = 0, bei 1 = 1 


            normalenListe.push_back(a);
            normalenListe.push_back(normalePunkt);
            // indizes hinzufügen
            normalenIndicesListe.push_back(lenght);
            normalenIndicesListe.push_back(lenght + 1);
        }

    std::vector<glm::vec3> colorsNormales;
    // Für jeden generierten Punkt exakt einen Farbwert anlegen
    for (size_t i = 0; i < normalenListe.size(); i++) {
        colorsNormales.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // Alles grün
    }

    // noralenliste und normalenindicesliste drawn.
    // Step 0: Create vertex array object.
    glGenVertexArrays(1, &normales.vao);
    glBindVertexArray(normales.vao);

    // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
    glGenBuffers(1, &normales.positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normales.positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, normalenListe.size() * sizeof(glm::vec3), normalenListe.data(), GL_STATIC_DRAW);

    // Bind it to position.
    pos = glGetAttribLocation(programId, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Step 2: Create vertex buffer object for color attribute and bind it to...
    glGenBuffers(1, &normales.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normales.colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, colorsNormales.size() * sizeof(glm::vec3), colorsNormales.data(), GL_STATIC_DRAW);

    // Bind it to color.
    pos = glGetAttribLocation(programId, "color");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Step 3: Create vertex buffer object for indices. No binding needed here.
    glGenBuffers(1, &normales.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, normales.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, normalenIndicesListe.size() * sizeof(GLushort), normalenIndicesListe.data(), GL_STATIC_DRAW);

    // Unbind vertex array object (back to default).
    glBindVertexArray(0);

    normales.model = glm::mat4(1.0f);

    // ==================================================================== ROtation faken ===================================================================================
	
	for (int y = 0; y < yRotation; y++) {

		// Rotationsmatrix für X-Achse erzeugen und auf alle Objekte anwenden
		sphere.model = glm::rotate(sphere.model, glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		normales.model = glm::rotate(normales.model, glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		koords.model = glm::rotate(koords.model, glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	for (int z = 0; z < zRotation; z++) {

		// Rotationsmatrix für X-Achse erzeugen und auf alle Objekte anwenden
		sphere.model = glm::rotate(sphere.model, glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		normales.model = glm::rotate(normales.model, glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		koords.model = glm::rotate(koords.model, glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	}
	for (int x = 0; x < xRotation; x++) {

		// Rotationsmatrix für X-Achse erzeugen und auf alle Objekte anwenden
		sphere.model = glm::rotate(sphere.model, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		normales.model = glm::rotate(normales.model, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		koords.model = glm::rotate(koords.model, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	}
    
}*/




std::vector<GLushort> calcIndices(int n, std::vector<glm::vec3> subTriangles) {
	std::vector<GLushort> sphereIndicesWithSubdivision;

	int numFaces = 8; // Unser Basis-Oktaeder hat 8 Flächen
	// Mathematische Summenformel für die Anzahl der Punkte in einem unterteilten Dreieck
	int pointsPerFace = ((n + 2) * (n + 3)) / 2;

	// Für jede der 8 Flächen die Indizes berechnen
	for (int face = 0; face < numFaces; face++) {
		// Offset: Bei welcher Index-Nummer beginnt die aktuelle Fläche im Array?
		int weirdIndexCounter = face * pointsPerFace;

		for (int y = 0; y <= n; y++) {
			int countRow = (n + 2 - y);

			for (int x = 0; x < countRow - 1; x++) {
				int V1 = weirdIndexCounter + x;
				int V2 = V1 + countRow;
				int V3 = V1 + 1;

				// Dreieck nach unten
				sphereIndicesWithSubdivision.push_back(V1);
				sphereIndicesWithSubdivision.push_back(V2);
				sphereIndicesWithSubdivision.push_back(V3);

				// Dreieck nach oben
				if (x != 0) {
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
	}
    for (int i = 0; i < sphereIndicesWithSubdivision.size(); i += 3)
    {
        std::cout
            << sphereIndicesWithSubdivision[i] << ", "
            << sphereIndicesWithSubdivision[i + 1] << ", "
            << sphereIndicesWithSubdivision[i + 2]
            << std::endl;
    }
	return sphereIndicesWithSubdivision;
}



std::vector<glm::vec3> calcSphereVertices(int n, std::vector<glm::vec3> sphereVerticesWithoutSubdivision, std::vector<GLushort> sphereIndicesWithoutSubdivision, glm::vec3 center)
{
	std::vector<glm::vec3> subTriangles; 

	// Wir gehen durch alle 8 Flächen des Basis-Oktaeders
	for (int k = 0; k < sphereIndicesWithoutSubdivision.size(); k += 3) {
		glm::vec3 v1 = sphereVerticesWithoutSubdivision[sphereIndicesWithoutSubdivision[k]];     // Spitze
		glm::vec3 v2 = sphereVerticesWithoutSubdivision[sphereIndicesWithoutSubdivision[k + 1]]; // Basis links
		glm::vec3 v3 = sphereVerticesWithoutSubdivision[sphereIndicesWithoutSubdivision[k + 2]]; // Basis rechts

		for (int y = 0; y <= n + 1; y++) {
            //äußere For wandert von unten nach oben durch die reihen
			// t geht von 0.0 (unten an der Basis v2-v3) bis 1.0 (oben an der Spitze v1)
			float t = (float)y / (n + 1);

			// Start- und Endpunkt der aktuellen Reihe berechnen
			glm::vec3 rowStart = v2 + t * (v1 - v2);
			glm::vec3 rowEnd = v3 + t * (v1 - v3);

			int pointsInRow = n + 2 - y; // Wird nach oben hin immer schmaler

			for (int x = 0; x < pointsInRow; x++) {
				glm::vec3 point;
				if (pointsInRow == 1) {
					point = rowStart; // Ganz oben gibt es nur noch die Spitze (v1)
				}
				else {
					// Auf der Reihe von links nach rechts interpolieren
					float u = (float)x / (pointsInRow - 1);
					point = rowStart + u * (rowEnd - rowStart);
				}

				// WICHTIG: Den Punkt auf die Kugeloberfläche zwingen (Radius = 1.0)
                //vllt müssen wir noch den Radius draufmultiplizieren? Wenn Radius mal nicht 1 ist?
				point = center + glm::normalize(point - center);
				subTriangles.push_back(point);
			}
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
  
  // ================================================================================= Construct view matrix =================================================================================
  // Construct view matrix.

 
  glm::vec3 eye(0.0f, 0.0f, CZoom);
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


  // =======================================================================INit sphre auf crack:
  //MySphere mySphere; // Dein neues globales Kugel-Objekt
  float currentRadius = 0.5f; // Optional: um Radius für die Tastatur zu speichern
  int currentSubdivisions = 3; // Optional: um n für die Tastatur zu speichern
  GLuint programId = program.getHandle();

  // Klasse aufrufen: n=3, radius=0.5, x=0, y=0, z=0, color=gelb
  sun.init(currentSubdivisions, currentRadius, 0.0f, 0.0f, 0.0f, glm::vec3(1.0f, 1.0f, 0.0f), programId);


  return true;
}


// ================================================================================= RENDER =================================================================================
/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	sun.render(program, projection, view);

	//renderTriangle();
	//renderQuad();

                            // unsicher ob das passt

    // fjdslkfklasdjf



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
        glutDestroyWindow(glutID);
        return;

   case '+':
        //if (currentSubdivisions == 4) break;
        //currentSubdivisions++;
        // Init neu aufrufen, damit die Geometrie neu berechnet wird
        //sun.init(currentSubdivisions, currentRadius, 0.0f, 0.0f, 0.0f, glm::vec3(1.0f, 1.0f, 0.0f), program.getHandle());
    break;
    case '-':
        //if (n == 0) { break; }
        //n -= 1;
        //initSphere(); //Größe nicht verändern

        break;
    case 'x':
        // Rotationsmatrix für X-Achse erzeugen und auf alle Objekte anwenden
        sphere.model = glm::rotate(sphere.model, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        normales.model = glm::rotate(normales.model, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        koords.model = glm::rotate(koords.model, glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        xRotation = (xRotation + 1) % 72; // bei 360 wieder auf 0
        //initSphere();
        break;

    case 'y':
        // Rotationsmatrix für Y-Achse erzeugen und auf alle Objekte anwenden
        sphere.model = glm::rotate(sphere.model, glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        normales.model = glm::rotate(normales.model, glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        koords.model = glm::rotate(koords.model, glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        yRotation = (yRotation + 1) % 72; // bei 360 wieder auf 0
        //initSphere();
        break;

    case 'z':
        // Rotationsmatrix für Z-Achse erzeugen und auf alle Objekte anwenden
        sphere.model = glm::rotate(sphere.model, glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        normales.model = glm::rotate(normales.model, glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        koords.model = glm::rotate(koords.model, glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        zRotation = (zRotation + 1) % 72; // bei 360 wieder auf 0
        //initSphere();
        break;
    
    //case 'r': // Kleiner machen (z.B. um den Faktor 0.9
        //if (size > 0.1)
        //{
        //    //sphere.model = glm::scale(sphere.model, glm::vec3(0.9f));
            //normales.model = glm::scale(normales.model, glm::vec3(0.9f));
        //    size -= 0.1f;
        //    initSphere();
        //}

        // koords.model weglassen, wenn das Achsenkreuz seine feste Größe behalten soll!
        //break;

    //case 'R': // Größer machen (z.B. um den Faktor 1.1)

        // if (size < 2)
        //{

            //sphere.model = glm::scale(sphere.model, glm::vec3(1.1f));
            //normales.model = glm::scale(normales.model, glm::vec3(1.1f));
           // size += 0.1;
          //  initSphere();
      //  }
	//    break;
		//  case 'l':
        // true false "switch" für normale
	//    normalen = !normalen;
		//     break;

		// case 'a':
        
		//  CZoom += 0.1;
		//   init();
		//   break;

		//  case 's':
        
       // CZoom -= 0.1f;
		// init();
	 //   break;

		//case 'n':
    // true false "switch" für normale
       // xRotation = 0;
	//	yRotation = 0;
		//zRotation = 0;
		//initSphere();
	  //  break;
        
    glutPostRedisplay();

    }
}



// ================================================================================= MAIN =================================================================================

int main(int argc, char** argv){
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


