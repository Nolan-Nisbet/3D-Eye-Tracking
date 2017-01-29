#include "Sprite_Renderer.h"


SpriteRenderer::SpriteRenderer(Shader &shader)
{
	this->shader = shader;
	this->camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f));
	this->initRenderData();
}

SpriteRenderer::~SpriteRenderer()
{
	glDeleteVertexArrays(1, &this->quadVAO);
}

void SpriteRenderer::DrawSprite(Texture2D &texture, glm::vec3 position, glm::vec2 size, GLfloat rotateX, GLfloat rotateY, GLuint side, glm::vec3 color)
{
	// Prepare transformations
	this->shader.Use();
	glm::mat4 model;

	
	model = glm::translate(model, position);

	
	//model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f));
	//model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));



	model = glm::rotate(model, glm::radians(rotateX), glm::vec3(1.0f, 0.0f, 0.0f));

	
	model = glm::rotate(model, glm::radians(rotateY), glm::vec3(0.0f, 1.0f, 0.0f));
	
	
	model = glm::scale(model, glm::vec3(size, 1.0f));

	this->shader.SetMatrix4("model", model);

	if (side == 0) {
		this->shader.SetMatrix4("view", this->camera.LeftViewMatrix());
		this->shader.SetMatrix4("projection", this->camera.LeftFrustum());
	}
	else {
		this->shader.SetMatrix4("view", this->camera.RightViewMatrix());
		this->shader.SetMatrix4("projection", this->camera.RightFrustum());
	}


	// Render textured quad
	this->shader.SetVector3f("spriteColor", color);

	glActiveTexture(GL_TEXTURE0);
	texture.Bind();

	glBindVertexArray(this->quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void SpriteRenderer::initRenderData()
{
	// Configure VAO/VBO
	GLuint VBO;
	
	GLfloat vertices[] = {
		// Pos			  // Tex
		0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

		0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f, 0.0f
	};


	//May be many problems here

	glGenVertexArrays(1, &this->quadVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(this->quadVAO);
	//Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	
	// TexCoord attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
