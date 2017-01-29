#include "Point_Renderer.h"


PointRenderer::PointRenderer(Shader &shader)
{
	this->shader = shader;
	this->camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f));
	this->initRenderData();
}

PointRenderer::~PointRenderer()
{
	glDeleteVertexArrays(1, &this->pointVAO);
}

void PointRenderer::DrawPoint(glm::vec3 position, glm::vec2 size, glm::vec3 color, GLuint side)
{
	// Prepare transformations
	this->shader.Use();
	glm::mat4 model;
	model = glm::translate(model, position);  
	model = glm::scale(model, glm::vec3(size, 1.0f)); 

	this->shader.SetMatrix4("model", model);

	//picks view matrix and frustrum based on which eye is being rendered
	if (side == 0) {
		this->shader.SetMatrix4("view", this->camera.LeftViewMatrix());
		this->shader.SetMatrix4("projection", this->camera.LeftFrustum());
	}
	else {
		this->shader.SetMatrix4("view", this->camera.RightViewMatrix());
		this->shader.SetMatrix4("projection", this->camera.RightFrustum());
	}


	// Render textured quad
	this->shader.SetVector3f("pointColor", color);


	glBindVertexArray(this->pointVAO);
	glDrawArrays(GL_TRIANGLES, 0, 270);
	glBindVertexArray(0);
}

void PointRenderer::initRenderData()
{
	// Configure VAO/VBO
	GLuint VBO;

	GLfloat vertices[810];
	GLuint segments = 90;
	GLfloat width = 0.001f;
	GLfloat height = 0.001f;

	GLuint count = 0;

	//creating vertices for a circle
	for (GLfloat i = 0; i < 360.0f; i += (360.0f / segments)) {
		vertices[count++] = 0.0f;
		vertices[count++] = 0.0f;
		vertices[count++] = 0.0f;
		vertices[count++] = (cos(glm::radians(i)) * width);
		vertices[count++] = (sin(glm::radians(i)) * height);
		vertices[count++] = 0.0f;
		vertices[count++] = (cos(glm::radians(i + (360.0f / segments))) * width);
		vertices[count++] = (sin(glm::radians(i + (360.0f / segments))) * height);
		vertices[count++] = 0.0f;
	}


	glGenVertexArrays(1, &this->pointVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(this->pointVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
