
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

// Default camera values
const GLfloat YAW = -90.0f;
const GLfloat PITCH = 0.0f;

const GLfloat EyeSeparation = 0.0647;
const GLfloat Convergence = 0.92;
const GLfloat NCD = 0.3;
const GLfloat FCD = 3.0;

const GLfloat ScreenWidth = 0.5313124;
const GLfloat ScreenHeight = 0.2988632;

// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// Camera Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	// Eular Angles
	GLfloat Yaw;
	GLfloat Pitch;



	// Constructor with vectors
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), GLfloat yaw = YAW, GLfloat pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f))
	{
		this->Position = position;
		this->WorldUp = up;
		this->Yaw = yaw;
		this->Pitch = pitch;
		this->updateCameraVectors();
	}
	// Constructor with scalar values
	Camera(GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f))
	{
		this->Position = glm::vec3(posX, posY, posZ);
		this->WorldUp = glm::vec3(upX, upY, upZ);
		this->Yaw = yaw;
		this->Pitch = pitch;
		this->updateCameraVectors();
	}

	// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(this->Position, this->Position + this->Front, this->Up);
	}


	/* Code included for created Left and Rigth view matrices and frustrums modified from existing project for Rendering a 3D Anaglyph  written
	by: Animesh Mishra 
	Original code can be found at : http://www.animesh.me/2011/05/rendering-3d-anaglyph-in-opengl.html
	
	*/



	glm::mat4 LeftViewMatrix()
	{
		this->Position -= this->Right * HES;
		glm::mat4 returnVal = glm::lookAt(this->Position, this->Position + this->Front, this->Up);
		this->Position += this->Right * HES;
		return returnVal;
	}

	glm::mat4 LeftFrustum()
	{

		float a = 0.0f;
		float b = 0.0f;
		float c = 0.0f;
		float left = 0.0f;
		float right = 0.0f;

		a = AspectRatio * tan(FOVRad / 2) * Convergence;
		b = a - EyeSeparation / 2;
		c = a + EyeSeparation / 2;

		left = -b * NCD / Convergence;
		right = c * NCD / Convergence;

		return glm::frustum(left, right, bottom, top, (float)NCD, (float)FCD);
	}

	glm::mat4 RightViewMatrix()
	{
		this->Position += this->Right * HES;
		glm::mat4 returnVal = glm::lookAt(this->Position, this->Position + this->Front, this->Up);
		this->Position -= this->Right * HES;
		return returnVal;
	}

	glm::mat4 RightFrustum()
	{

		float a = 0.0f;
		float b = 0.0f;
		float c = 0.0f;
		float left = 0.0f;
		float right = 0.0f;

		a = AspectRatio * tan(FOVRad / 2) * Convergence;
		b = a - EyeSeparation / 2;
		c = a + EyeSeparation / 2;

		left = -c * NCD / Convergence;
		right = b* NCD / Convergence;


		return glm::frustum(left, right, bottom, top, (float)NCD, (float)FCD);
	}

private:
	// Calculates the front vector from the Camera's (updated) Eular Angles
	void updateCameraVectors()
	{
		// Calculate the new Front vector
		glm::vec3 front;
		front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
		front.y = sin(glm::radians(this->Pitch));
		front.z = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
		this->Front = glm::normalize(front);
		// Also re-calculate the Right and Up vector
		this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		this->Up = glm::normalize(glm::cross(this->Right, this->Front));
	}

	GLfloat HES = EyeSeparation / 2.0;
	GLfloat AspectRatio = 16 / 9.0;
	GLfloat FOVRad = 2.0 * atan((ScreenHeight / 2.0) / Convergence);

	GLfloat top = NCD * tan(FOVRad / 2);
	GLfloat bottom = -top;

	float a = 0.0f;
	float b = 0.0f;
	float c = 0.0f;
};

