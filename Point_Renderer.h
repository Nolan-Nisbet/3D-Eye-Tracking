#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Texture.h"
#include "Shader.h"
#include "Camera.h"


class PointRenderer
{
public:
	// Constructor (inits shaders/shapes)
	PointRenderer(Shader &shader);
	// Destructor
	~PointRenderer();
	// Renders a defined quad textured with given sprite
	void DrawPoint(glm::vec3 position, glm::vec2 size, glm::vec3 color, GLuint side);
private:
	// Render state
	Shader shader;
	GLuint pointVAO;
	Camera camera;
	// Initializes and configures the quad's buffer and vertex attributes
	void initRenderData();
};
