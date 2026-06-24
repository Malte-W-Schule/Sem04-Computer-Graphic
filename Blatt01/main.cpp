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


struct GeometryConfig {
	int subdivisions = 36;
	float radius = 1.0f;
};

struct TransformConfig {
	glm::vec3 position = glm::vec3(0.0f); 
	float initialDegree = 0.0f;
	float orbitAngle = 0.0f;
};

struct MaterialConfig {
	glm::vec3 color = glm::vec3(1.0f);
	glm::vec3 surfKa = glm::vec3(0.1f); // Ambient
	glm::vec3 surfKd = glm::vec3(0.6f); // Diffuse
	glm::vec3 surfKs = glm::vec3(0.3f); // Specular
};

struct RenderConfig {
	bool has_axis = false;
};


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

	
glm::mat4x4 view;
glm::mat4x4 projection;

float zNear = 0.1f;
float zFar  = 100.0f;

int xRotation = 0;
int yRotation = 0;
int zRotation = 0;

GLuint normalBuffer;

std::vector<GLushort> calcIndices(int n, std::vector<glm::vec3> subTriangles);
std::vector<glm::vec3> calcSphereVertices(int n, std::vector<glm::vec3> sphereVerticesWithoutSubdivision, std::vector<GLushort> sphereIndicesWithoutSubdivision, glm::vec3 center);
void renderSphere();
//void initSphere();
void renderNormales();
void renderKoords();
int indexCount = 0;
int indexCountNormals = 0;
float CZoom = 4.0f;

bool planetRotate = true;
float planetSpeed = 1.0f;
float currentPlanetSpeed = 1.0f;
bool isDepictionSolid = false;

class MySphere;

unsigned  lightIndex = 1;
glm::vec4 lights[2] = {
	glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), // Richtungslicht
	glm::vec4(0.0f, 0.0f, CZoom, 1.0f) // Punktlicht
};

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

/*
- surfKA = Ambient 
	- Die ambiente Reflexion (Umgebungslicht).

- surfKd = Diffuse 
	- Die diffuse Reflexion (Körperfarbe).

- surfKs = Specular
	- Die spekulare Reflexion (Glanzlicht / Spiegelung).
*/
void initShader(cg::GLSLProgram& program,glm::vec3 surfKa, glm::vec3 surfKd, glm::vec3 surfKs)
{
	program.use();
	program.setUniform("light", glm::vec3(0, 0, 0));
	program.setUniform("lightI", float(1.0f));

	// die farbe
	program.setUniform("surfKa", surfKa);
	program.setUniform("surfKd", surfKd);
	program.setUniform("surfKs", surfKs);

	program.setUniform("surfShininess", float(8.0f));
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
	GLuint normalBuffer;

	// OpenGL Puffer-IDs für die Achsen (NEU)
	GLuint axisVao;
	GLuint axisPositionBuffer;
	GLuint axisColorBuffer;
	GLuint axisIndexBuffer;

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
	glm::vec3 surfKa;
	glm::vec3 surfKd;
	glm::vec3 surfKs;

	cg::GLSLProgram program;

	// Konstruktor
	MySphere() : vao(0), positionBuffer(0), colorBuffer(0), indexBuffer(0), normalBuffer(0), indexCount(0), n(3) {}

//	============================== INIT ==============================	
	bool init(const GeometryConfig& geom,
		const TransformConfig& transform,
		const MaterialConfig& material,
		const RenderConfig& render) {


	this->surfKa = material.surfKa;
	this->surfKd = material.surfKd;
	this->surfKs = material.surfKs;
	this->n = geom.subdivisions;
	this->r = geom.radius;

	this->center = transform.position;

	this->degree = transform.initialDegree;
	this->orbitAngle = transform.orbitAngle;

	this->has_axis = render.has_axis;


	glm::vec3 color = material.color;

	float radius = geom.radius;



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

		



		// Create a shader program and set light direction.
		if (!program.compileShaderFromFile("shader/shadedPhong.vert", cg::GLSLShader::VERTEX)) {
			std::cerr << program.log();
			//	return false;
		}

		if (!program.compileShaderFromFile("shader/shadedPhong.frag", cg::GLSLShader::FRAGMENT)) {
			std::cerr << program.log();
			//	return false;
		}

		if (!program.link()) {
			std::cerr << program.log();
			//	return false;
		}

		GLuint programId = program.getHandle();

	// ==========================================
	// 1. KUGEL INITIALISIEREN
	// ==========================================

																															/*======================== Normalen ==========================*/
	std::vector<glm::vec3> normalenListe;
	std::vector<GLushort> normalenIndicesListe;

	for (auto& a : currentVertices)
	{
		//glm::vec3 normalePunkt = a + (a - center);
		glm::vec3 normalePunkt = glm::normalize(a);

		normalenListe.push_back(normalePunkt);
	}

	std::vector<glm::vec3> colorsNormales;
	// Für jeden generierten Punkt exakt einen Farbwert anlegen
	for (size_t i = 0; i < normalenListe.size(); i++) {
		colorsNormales.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // Alles grün
	}

	// ==========================================
	//  SHADER ZEUG! Kugel
	// ==========================================

	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &positionBuffer);
		glDeleteBuffers(1, &colorBuffer);
		glDeleteBuffers(1, &indexBuffer);
	}

	initShader(program, surfKa, surfKd, surfKs);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// === Position ===

	glGenBuffers(1, &positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, currentVertices.size() * sizeof(glm::vec3), currentVertices.data(), GL_STATIC_DRAW);
	GLuint pos = glGetAttribLocation(programId, "position");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// === Color ===

	glGenBuffers(1, &colorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
	GLuint col = glGetAttribLocation(programId, "color");
	glEnableVertexAttribArray(col);
	glVertexAttribPointer(col, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// === Normalen ===

	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, normalenListe.size() * sizeof(glm::vec3), normalenListe.data(), GL_STATIC_DRAW);
	GLuint nor = glGetAttribLocation(programId, "normal");
	glEnableVertexAttribArray(nor);
	glVertexAttribPointer(nor, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, currentIndices.size() * sizeof(GLushort), currentIndices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);





	// ==========================================
	// 2. ACHSEN INITIALISIEREN (EIGENE PUFFER!)
	// ==========================================
	if (has_axis) {
		std::vector<glm::vec3> axesVertices;
		std::vector<GLushort> axesIndices = { 0, 1 };

		// Y-Achse (oben und unten)
		glm::vec3 posAxes = (StartVertices[0] - center) * 2.0f + center;
		glm::vec3 negAxes = (StartVertices[1] - center) * 2.0f + center;

		axesVertices.push_back(posAxes);
		axesVertices.push_back(negAxes);

		std::vector<glm::vec3> colorAxes = { glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };


		// ==========================================
		// 3. SHADER ZEUG! Achsen
		// ==========================================

		if (axisVao != 0) {
			glDeleteVertexArrays(1, &axisVao);
			glDeleteBuffers(1, &axisPositionBuffer);
			glDeleteBuffers(1, &axisColorBuffer);
			glDeleteBuffers(1, &axisIndexBuffer);

		}


		glGenVertexArrays(1, &axisVao);
		glBindVertexArray(axisVao);

		glGenBuffers(1, &axisPositionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, axisPositionBuffer);
		glBufferData(GL_ARRAY_BUFFER, axesVertices.size() * sizeof(glm::vec3), axesVertices.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(pos);
		glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

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
		this->translationModel = glm::translate(glm::mat4(1.0f), transform.position);
		this->AxisInclinedModel = glm::rotate(glm::mat4(1.0f), glm::radians(degree), glm::vec3(0.0f, 0.0f, 1.0f));
		this->SphereModel = translationModel * AxisInclinedModel;

		return true;
	}

	//setze die kugel + achse auf die neue koordinate
    void setNewCoordinatesForCenter(float x, float y, float z) {					//////////////////////////////////////////////////////////////////////////////////////////////
       this->center = glm::vec3(x,y,z);

    }

	// Render-Funktion direkt in der Klasse
	void render(cg::GLSLProgram& program, const glm::mat4& projection, const glm::mat4& view) { //////////////////////////////////////////////////////////////////////////////////////////////

		glm::mat4 modelView = view * this->SphereModel;
		glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(modelView)));

		program.use();
		program.setUniform("modelviewMatrix", modelView);
		program.setUniform("projectionMatrix", projection);
		program.setUniform("normalMatrix", normalMat);

		program.setUniform("light", lights[lightIndex]);
		// Gemeinsame MVP Matrix für das Objekt
		/*
		glm::mat4 mvp = projection * view * this->SphereModel;

		program.use();
		program.setUniform("mvp", mvp);
		program.setUniform("light", lights[lightIndex]);
		*/
		// --- KUGEL ZEICHNEN ---
		glBindVertexArray(vao);
		if (isDepictionSolid) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);

		// --- ACHSEN ZEICHNEN ---
		if (this->has_axis) {
			glm::mat4 mvpAxis = projection * view * this->SphereModel;
			//program.setUniform("mvp", mvpAxis);
			
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
void rotateVectorFromSphere(MySphere& middle, MySphere& toRotate, float orbitLength, float speed) { //////////////////////////////////////////////////////////////////////////////////////////////

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


MySphere sun;
MySphere planet_right;
MySphere planet_left;
MySphere planet_tmp;
MySphere moon_right;
MySphere moon_left;

// ================================================================================= Ende MySPHERE =================================================================================


// ================================================================================= RENDER SPHERE =================================================================================
/*
void renderSphere() //////////////////////////////////////////////////////////////////////////////////////////////
{   // Create mvp.
    glm::mat4x4 mvp = projection * view * sphere.model;

    // Bind the shader program and set uniform(s).
    program.use();
	sun.program.use();    program.setUniform("mvp", mvp);

    // Bind vertex array object so we can render the 1 triangle.
    glBindVertexArray(sphere.vao);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}
*/

/*
void renderNormales() { //////////////////////////////////////////////////////////////////////////////////////////////
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
    
void renderKoords() { //////////////////////////////////////////////////////////////////////////////////////////////

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
*/


std::vector<GLushort> calcIndices(int n, std::vector<glm::vec3> subTriangles) { //////////////////////////////////////////////////////////////////////////////////////////////
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
	return sphereIndicesWithSubdivision;
}


//////////////////////////////////////////////////////////////////////////////////////////////
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


// ================================================================================= INIT =================================================================================


/*
 Initialization. Should return true if everything is ok and false if something went wrong.
 */
bool init()
{
  // OpenGL: Set "background" color and enable depth testing.
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
 
  glm::vec3 eye(0.0f, 0.0f, CZoom);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  
  view = glm::lookAt(eye, center, up);
  
  // =======================================================================INit sphre auf crack:
  //MySphere mySphere; // Dein neues globales Kugel-Objekt
  float currentRadius = 0.5f; // Optional: um Radius für die Tastatur zu speichern
  int currentSubdivisions = 3; // Optional: um n für die Tastatur zu speichern
  

  
// --- GEMEINSAME BASIS-WERTE ---
	GeometryConfig baseGeom;
baseGeom.subdivisions = currentSubdivisions;

RenderConfig baseRender;
//baseRender.programId = programId;

MaterialConfig baseMat;
baseMat.color  = glm::vec3(1.0f, 1.0f, 0.0f);
baseMat.surfKa = glm::vec3(0.1f, 0.1f, 0.1f);
baseMat.surfKs = glm::vec3(1.0f, 1.0f, 1.0f);


// ==========================================
// MATERIALIEN FÜR DIE VERSCHIEDENEN KÖRPER 
// ==========================================
MaterialConfig sunMat = baseMat;
sunMat.surfKd = glm::vec3(1.0f, 1.0f, 0.0f); // Gelb

MaterialConfig planetMat = baseMat;
planetMat.surfKd = glm::vec3(0.8f, 0.1f, 0.1f); // Rötlich

MaterialConfig moonMat = baseMat;
moonMat.surfKd = glm::vec3(0.2f, 0.2f, 0.2f); // Grau

// ==========================================
// SONNE
// ==========================================
GeometryConfig sunGeom = baseGeom;
sunGeom.radius = currentRadius;

TransformConfig sunTrans; // Standard ist glm::vec3(0.0f), passt also für die Mitte

RenderConfig sunRender = baseRender;
sunRender.has_axis = true;

sun.init(sunGeom, sunTrans, sunMat, sunRender);

// ==========================================
// PLANET RECHTS
// ==========================================
GeometryConfig planetGeom = baseGeom;
planetGeom.radius = 0.3f;

TransformConfig pRightTrans;
pRightTrans.position = glm::vec3(2.0f, 0.0f, 0.0f);
pRightTrans.initialDegree = 45.0f;

RenderConfig pRightRender = baseRender;
pRightRender.has_axis = true;

planet_right.init(planetGeom, pRightTrans, planetMat, pRightRender);

// ==========================================
// PLANET LINKS
// ==========================================
TransformConfig pLeftTrans;
pLeftTrans.position = glm::vec3(-2.0f, 0.0f, 0.0f);
pLeftTrans.orbitAngle = 180.0f;

RenderConfig pLeftRender = baseRender;
pLeftRender.has_axis = true;

// Nutzt dasselbe planetGeom wie der rechte Planet!
planet_left.init(planetGeom, pLeftTrans, planetMat, pLeftRender);

// ==========================================
// MOND RECHTS
// ==========================================
GeometryConfig moonGeom = baseGeom;
moonGeom.radius = 0.1f;

TransformConfig mRightTrans;
mRightTrans.position = glm::vec3(2.5f, 0.0f, 0.0f);

moon_right.init(moonGeom, mRightTrans, moonMat, baseRender);

// ==========================================
// MOND LINKS
// ==========================================
TransformConfig mLeftTrans;
mLeftTrans.position = glm::vec3(-2.1f, 0.0f, 0.0f);

// gleiche moon geom wie rechter moond
moon_left.init(moonGeom, mLeftTrans, moonMat, baseRender);

  return true;
}

bool start = true;

// ================================================================================= RENDER =================================================================================
/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// ==========================================
	// 1. Framerate / Geschwindigkeit berechnen
	// ==========================================
	static int lastTime = glutGet(GLUT_ELAPSED_TIME);

	int currentTime = glutGet(GLUT_ELAPSED_TIME);

	float deltaTime = (currentTime - lastTime) / 1000.0f;

	lastTime = currentTime;

	glm::vec3 eye(0.0f, 0.0f, CZoom);
	glm::vec3 center(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.0f, 1.0f, 0.0f);

	view = glm::lookAt(eye, center, up);

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
	sun.render(sun.program, projection, view);
    
	planet_right.render(planet_right.program, projection, view);
	planet_left.render(planet_left.program, projection, view);

	moon_right.render(moon_right.program, projection, view);
	moon_left.render(moon_left.program, projection, view);

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
  // In der glutResize Funktion:
  //projection = glm::perspective(glm::radians(45.0f), (float)width / height, zNear, zFar);
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
		if (CZoom > 1)
		{
			CZoom -= 0.5f;
		}
		break;
	case '-':
		if (CZoom < 6)
		{
			CZoom += 0.5f;
			
		}
		break;
	//langsamer
	case 'd':

		if(planetSpeed > 0 && planetRotate)
		{
			planetSpeed -= 0.1f; 
		}
		break;

	//schneller
	case 'f':
		if (planetSpeed < 2 && planetRotate)
		{
			planetSpeed += 0.1f;
		}
		break;

	//Stoppen und fortsetzen
	case 'g':

		if(planetRotate)
		{
			currentPlanetSpeed = planetSpeed;
			planetSpeed = 0.0f;

		}
		else{
			planetSpeed = currentPlanetSpeed;
		}

		planetRotate = !planetRotate;
		
		break;

	case 's':
		isDepictionSolid = !isDepictionSolid;
		break;
	case '1':
		lightIndex = 1 - lightIndex; // wechsel 0 -> 1 -> 0...
    
        
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


