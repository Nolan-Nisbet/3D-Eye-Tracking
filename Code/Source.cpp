
/*Code included in Poitn_Render.h, Resource Manager.h, Shader.h, Sprite_Renderer.h, Texture.h, Point Renderer.cpp, Resource_Manager.cpp, Shader.cpp, Sprite Renderer.cpp, texture.cpp has been heavily modified from code
provided by Joey de Vries located at : http://learnopengl.com/#!About
The code he provides is under Public Domain Dedicationwhich waves all copyrights
*/


#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Control.h"
#include "Resource_Manager.h"
#include <ctime>


// GLFW function declerations
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// The Width of the screen in pixels
const GLuint SCREEN_WIDTH = 1920;
// The height of the screen in pixels
const GLuint SCREEN_HEIGHT = 1080;

Control Calibration(SCREEN_WIDTH, SCREEN_HEIGHT);

int main(int argc, char *argv[])
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	glfwWindowHint(GLFW_STEREO, GL_TRUE); //turn on stereo (quad buffering)

	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Calibration", glfwGetPrimaryMonitor(), nullptr); //make sure that you set your primary monitor to one capable of 3d
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	glewInit();
	glGetError(); // Call it once to catch glewInit() bug, all other errors are now from our application.

	glfwSetKeyCallback(window, key_callback);

	// Define the viewport dimensions
	glViewport(0, 0, 1920, 1080);

	// Setup OpenGL options
	glEnable(GL_DEPTH_TEST);

	// Initialize calibration
	Calibration.Init(clock() / (CLOCKS_PER_SEC / 1000));

	double currentTime = 0.0f;


	// Start Game within Menu State
	Calibration.State = Calibration_2D;

	while (!glfwWindowShouldClose(window))
	{
		// Calculate delta time
		double currentTime = clock() / (CLOCKS_PER_SEC / 1000);
		glfwPollEvents();

		//deltaTime = 0.001f;
		// Manage user input
		Calibration.ProcessInput(currentTime);

		// Update Calibration state
		Calibration.Update(currentTime);

		// Render for left eye
		glDrawBuffer(GL_BACK_LEFT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Calibration.RenderLeft();

		//Render for right eye
		glDrawBuffer(GL_BACK_RIGHT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Calibration.RenderRight();

		glfwSwapBuffers(window);


	}

	// Delete all resources as loaded using the resource manager
	ResourceManager::Clear();

	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	//Escape key used to exit program
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			Calibration.Keys[key] = GL_TRUE;
		else if (action == GLFW_RELEASE)
			Calibration.Keys[key] = GL_FALSE;
	}
}
