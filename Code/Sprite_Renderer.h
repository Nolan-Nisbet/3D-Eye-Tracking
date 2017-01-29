#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include "Texture.h"
#include "Shader.h"
#include "Camera.h"


class SpriteRenderer
{
public:
	// Constructor (inits shaders/shapes)
	SpriteRenderer(Shader &shader);
	// Destructor
	~SpriteRenderer();
	// Renders a defined quad textured with given sprite
	void DrawSprite(Texture2D &texture, glm::vec3 position, glm::vec2 size, GLfloat rotateX, GLfloat rotateY, GLuint side, glm::vec3 color);
private:
	// Render state
	Shader shader;
	GLuint quadVAO;
	Camera camera;
	// Initializes and configures the quad's buffer and vertex attributes
	void initRenderData();
};

