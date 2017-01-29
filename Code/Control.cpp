#include "Control.h"
#include "Resource_Manager.h"
#include "Point_Renderer.h"
#include "Sprite_Renderer.h"
#include "Plane_Calibration.h"
#include "Depth_Calibration.h"
#include "Sandbox.h"
#include "GPClient.h"



PointRenderer *Renderer;
SpriteRenderer *SRenderer;
PlaneCalibration *Plane;
DepthCalibration *Depth;
Sandbox *Sand;




GPClient client;

Control::Control(GLuint width, GLuint height)
	: State(Calibration_2D), Keys(), Width(width), Height(height)
{

}

Control::~Control()
{
	delete Renderer;
}

void Control::Init(double start)
{
	// Load shaders
	ResourceManager::LoadShader("shaders/point.vs", "shaders/point.frag", nullptr, "point");
	ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");

	// Configure shaders
	ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);

	// Set render-specific controls
	Renderer = new PointRenderer(ResourceManager::GetShader("point"));


	//Intiate three main stages in program
	Plane = new PlaneCalibration(start, 0);
	Depth = new DepthCalibration(start, 0);
	Sand = new Sandbox(start);


	SRenderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));

	//Gazepoint connect
	client.client_connect();

	client.send_cmd("<SET ID=\"ENABLE_SEND_POG_LEFT\" STATE=\"1\" />"); //pulls point of Gaze for left eye
	client.send_cmd("<SET ID=\"ENABLE_SEND_POG_RIGHT\" STATE=\"1\" />"); //pulls point of gaze for right eye
	client.send_cmd("<SET ID=\"ENABLE_SEND_DATA\" STATE=\"1\" />");


	// Load textures
	ResourceManager::LoadTexture("textures/bgrid.jpg", GL_TRUE, "bgrid"); //used to create box 
	ResourceManager::LoadTexture("textures/lrgrid.jpg", GL_TRUE, "lrgrid");
	ResourceManager::LoadTexture("textures/tbgrid.jpg", GL_TRUE, "tbgrid");

	ResourceManager::LoadTexture("textures/depth.jpg", GL_TRUE, "depth"); 

	ResourceManager::LoadTexture("textures/city.jpg", GL_TRUE, "city");
	ResourceManager::LoadTexture("textures/stones2.jpg", GL_TRUE, "stones");
	ResourceManager::LoadTexture("textures/space.jpg", GL_TRUE, "space");

}


/*Used to send the latest gazepoint data to the current stage of the program as well as update states when calibrations have been completed*/
void Control::Update(double ct)
{
	GLfloat xl = 0.0f;
	GLfloat yl = 0.0f;
	GLfloat xr = 0.0f;
	GLfloat yr = 0.0f;

	
	if (Plane->complete) { //2d calibration is completed
		Plane->Clean(); //runs various algorithms to clean the raw data
		Plane->Linear_Fit(); //creates a linear fit function to map each individual eye
		this->State = Calibration_2D_Result; 
		dt = ct; 
		Plane->complete = 0;
		PlaneComplete = 1; //lets other stages know that linear fit can be used
	}
	else if (Depth->complete) { //3d calibration is completed

		Depth->Clean(); //runs various algorithms to clean the raw data
		Depth->start = ct; //resets the start time in case multiple data sets are goign to be collected
		Depth->setnum = Depth->setnum + 1; //increases the data set number
		dt = ct;
		if (Depth->setnum > (Depth->maxSets-1)) { //if max amount of data sest acquired
			this->State = Calibration_3D_Result;
			Depth->Print_Test(); //print data from 3d calibration
		}
		Depth->complete = 0;
		DepthComplete = 1; //lets other stages know that linear regression values can be used
	}


	//acquires most recent eye track data and sends it to the current stage of the program
	if (this->State == Calibration_2D) {
		Plane->Update(ct);
		if (Gaze_Location(xl, yl, xr, yr)) {
			Plane->Gaze_Data(xl, yl, xr, yr, ct);
		}
	}
	else if (this->State == Calibration_3D) {

		Depth->Update(ct);
		if (Gaze_Location(xl, yl, xr, yr)) {
			Depth->Gaze_Data(xl, yl, xr, yr, ct);
		}
	}
	else if (this->State == SB){
		
		if (Gaze_Location(xl, yl, xr, yr)) {
			Sand->Gaze_Data(xl, yl, xr, yr, ct);
		}
		Sand->Update(ct);
	}
	else if (this->State == VolRen) {
		
	}
}

/*Handles keyboard input from user*/
void Control::ProcessInput(double ct)
{
	if ((ct - lastpress) > 300) { 
		if (this->State == Calibration_2D_Result) {
			if (this->Keys[GLFW_KEY_Q]) { //indicates user have fisnihed viewing calibration 2d results
				this->State = Calibration_3D;

				
				//Passed lienar fit data to next two stages
				Depth->xl_Intercept = Plane->xl_Intercept;
				Depth->xl_Slope = Plane->xl_Slope;
				Depth->xr_Intercept = Plane->xr_Intercept;
				Depth->xr_Slope = Plane->xr_Slope;
				Depth->yl_Intercept = Plane->yl_Intercept;
				Depth->yl_Slope = Plane->yl_Slope;
				Depth->yr_Intercept = Plane->yr_Intercept;
				Depth->yr_Slope = Plane->yr_Slope;


				Sand->xl_Intercept = Plane->xl_Intercept;
				Sand->xl_Slope = Plane->xl_Slope;
				Sand->xr_Intercept = Plane->xr_Intercept;
				Sand->xr_Slope = Plane->xr_Slope;
				Sand->yl_Intercept = Plane->yl_Intercept;
				Sand->yl_Slope = Plane->yl_Slope;
				Sand->yr_Intercept = Plane->yr_Intercept;
				Sand->yr_Slope = Plane->yr_Slope;

				
				Depth->start = ct;
				lastpress = ct;
			}
			//toggles display of various data
			else if (this->Keys[GLFW_KEY_R]) {
				RawR = !(RawR); 
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_L]) {
				RawL = !(RawL);
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_E]) {
				AverageR = !(AverageR);
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_W]) {
				AverageL = !(AverageL);
				lastpress = ct;
			}
			//toggles lienar fit adjustment to data
			else if (this->Keys[GLFW_KEY_T]) {
				LinFit = !(LinFit);
				lastpress = ct;
			}
		}
		else if (this->State == Calibration_2D) { 
			if (this->Keys[GLFW_KEY_Q]) { //used to skip 2d calibation
				this->State = Calibration_3D;
				Depth->start = ct;
				lastpress = ct;
			}
		}
		else if (this->State == Calibration_3D_Result) {
			if (this->Keys[GLFW_KEY_Q]) {

				
					//Runs a multivariate linear regression to find coefficients
					
					/*
					To change which fixation method is used for the lin regression change the number beside m:
					m1->average
					m2->clean average
					m3->weighted average
					m4->median
					m5->dti
					*/

					for (int i = 0; i < 36; i++) {
						X[0][i] = (Depth->set[i].set[0].m5.xr + Depth->set[i].set[0].m5.xl) / 2.0; //POG between both eyes x
						X[1][i] = (Depth->set[i].set[0].m5.yr + Depth->set[i].set[0].m5.yl) / 2.0; //POG between both eyes y
						X[2][i] = Depth->set[i].set[0].m5.xr - Depth->set[i].set[0].m5.xl; // seperation between eyes

						Y[i] = Depth->positions[i + 1].z;
					}
					LinReg();
						
					

					for (int i = 0; i < 3; i++) {
						Sand->R1[i] = C[i];
					}


					//Runs lin regression based on (x,y,z) for x, y and z
					for (int i = 0; i < 36; i++) {
						X[0][i] = Depth->set[i].set[0].m5.gx; //3 space location x
						X[1][i] = Depth->set[i].set[0].m5.gy; //3 space location y
						X[2][i] = Depth->set[i].set[0].m5.gz; //3 space location z

						Y[i] = Depth->positions[i + 1].x;
					}
					LinReg();

					for (int i = 0; i < 3; i++) {
						Sand->R2x[i] = C[i];
					}

					for (int i = 0; i < 36; i++) {
						X[0][i] = Depth->set[i].set[0].m5.gx; //3 space location x
						X[1][i] = Depth->set[i].set[0].m5.gy; //3 space location y
						X[2][i] = Depth->set[i].set[0].m5.gz; //3 space location z

						Y[i] = Depth->positions[i + 1].y;
					}
					LinReg();

					for (int i = 0; i < 3; i++) {
						Sand->R2y[i] = C[i];
					}

					for (int i = 0; i < 36; i++) {
						X[0][i] = Depth->set[i].set[0].m5.gx; //3 space location x
						X[1][i] = Depth->set[i].set[0].m5.gy; //3 space location y
						X[2][i] = Depth->set[i].set[0].m5.gz; //3 space location z

						Y[i] = Depth->positions[i + 1].z;
					}
					LinReg();

					for (int i = 0; i < 3; i++) {
						Sand->R2z[i] = C[i];
					}

					if (Depth->maxSets >= 2) {
						PrintDepthResults();
					}
			

				this->State = SB; //update state to sandbox
			}
			//toggle various data
			else if (this->Keys[GLFW_KEY_R]) {
				RawR = !(RawR);
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_L]) {
				RawL = !(RawL);
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_E]) {
				AverageR = !(AverageR);
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_W]) {
				AverageL = !(AverageL);
				lastpress = ct;
			}
			//used to indicate which planes data points to show
			else if (this->Keys[GLFW_KEY_S]) {
				plane = 0;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_D]) {
				plane = 1;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_F]) {
				plane = 2;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_G]) {
				plane = 3;
				lastpress = ct;
			}
			//indicate the type of adjustment raw and clean data with recieve
			else if (this->Keys[GLFW_KEY_T]) {
				if (PlaneComplete) {
					LinFit = !(LinFit);
				}
				lastpress = ct;
			}
			//indicates which clean method will be shown as average
			else if (this->Keys[GLFW_KEY_Y]) {
				average = 1;
				clean_average = 0;
				weighted_average = 0;
				median = 0;
				dti = 0;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_U]) {
				average = 0;
				clean_average = 1;
				weighted_average = 0;
				median = 0;
				dti = 0;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_I]) {
				average = 0;
				clean_average = 0;
				weighted_average = 1;
				median = 0;
				dti = 0;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_O]) {
				average = 0;
				clean_average = 0;
				weighted_average = 0;
				median = 1;
				dti = 0;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_P]) {
				average = 0;
				clean_average = 0;
				weighted_average = 0;
				median = 0;
				dti = 1;
				lastpress = ct;
			}
			//indicates which calibration set is shown
			else if (this->Keys[GLFW_KEY_H]) {
				sets=0;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_J]) {
				if(Depth->setnum > 0)
					sets=1;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_K]) {
				if(Depth->setnum > 1)
					sets=2;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_L]) {
				if(Depth->setnum > 2)
					sets=3;
				lastpress = ct;
			}		
		}
		else if (this->State == Calibration_3D) {
			if (this->Keys[GLFW_KEY_Q]) { //indicates user have finished viewing calibration 3d results
				this->State = SB;
				lastpress = ct;
			}
		}
		else if (this->State == SB) {
			//Selects which mini game to display
			if (this->Keys[GLFW_KEY_I]) {
				points = 1; 
				imageView = 0;
				staticPoints = 0;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_O]) {
				points = 0;
				imageView = 1;
				staticPoints = 0;
				lastpress = ct;
			}
			else if (this->Keys[GLFW_KEY_P]) {
				points = 0;
				imageView = 0;
				staticPoints = 1;
				lastpress = ct;
			}
			//increases or decreases the depth of the image shown within a rnage of 0-1m from the convergence plane
			else if (this->Keys[GLFW_KEY_S]) { 
				lastpress = ct;
				if (Sand->depth >= 1.0) {
					Sand->depth = 1.0;
				}
				else {
					Sand->depth += 0.02;
				}
			}
			else if (this->Keys[GLFW_KEY_D]) {
				lastpress = ct;
				if (Sand->depth <= 0) {
					Sand->depth = 0;
				}
				else {
					Sand->depth -= 0.02;
				}
			}
			//chooses which method is used to calculate 3d gaze position
			else if (this->Keys[GLFW_KEY_E]) {
					Sand->NoFit = 1;
					Sand->LinFit = 0;
					Sand->LinR = 0;
					Sand->LinR2 = 0;
			}
			else if (this->Keys[GLFW_KEY_R]) {
				if (PlaneComplete) {
					Sand->NoFit = 0;
					Sand->LinFit = 1;
					Sand->LinR = 0;
					Sand->LinR2 = 0;
				}
			}
			else if (this->Keys[GLFW_KEY_T]) {
				if (DepthComplete) {
					Sand->NoFit = 0;
					Sand->LinFit = 0;
					Sand->LinR = 1;
					Sand->LinR2 = 0;
				}
			}
			else if (this->Keys[GLFW_KEY_Y]) {
				if (DepthComplete) {
					Sand->NoFit = 0;
					Sand->LinFit = 0;
					Sand->LinR = 0;
					Sand->LinR2 = 1;
				}
			}
			else if (this->Keys[GLFW_KEY_F]) { //lets user select the excact dpeth they would like the image shown at
				lastpress = ct;
				Sand->depth = 0;
			}
			else if (this->Keys[GLFW_KEY_G]) {
				lastpress = ct;
				Sand->depth = 0.1;
			}
			else if (this->Keys[GLFW_KEY_H]) {
				lastpress = ct;
				Sand->depth = 0.2;
			}
			else if (this->Keys[GLFW_KEY_J]) {
				lastpress = ct;
				Sand->depth = 0.3;
			}
			else if (this->Keys[GLFW_KEY_K]) {
				lastpress = ct;
				Sand->depth = 0.4;
			}
			else if (this->Keys[GLFW_KEY_L]) {
				lastpress = ct;
				Sand->depth = 0.5;
			}
			else if (this->Keys[GLFW_KEY_Z]) {
				lastpress = ct;
				Sand->depth = 0.6;
			}
			else if (this->Keys[GLFW_KEY_X]) {
				lastpress = ct;
				Sand->depth = 0.7;
			}
			else if (this->Keys[GLFW_KEY_C]) {
				lastpress = ct;
				Sand->depth = 0.8;
			}
			else if (this->Keys[GLFW_KEY_V]) {
				lastpress = ct;
				Sand->depth = 0.9;
			}
			else if (this->Keys[GLFW_KEY_B]) {
				lastpress = ct;
				Sand->depth = 1.0;
			}
		}
	}
}

void Control::RenderLeft()
{
	Draw(0); //calls draw function 
}

void Control::RenderRight()
{
	Draw(1); //calls draw function 
}


void Control::Draw(GLuint side) {
	GLfloat offset = -0.0000001f; 

	if (this->State == Calibration_2D) {
		if (Plane->transition) { //draws calibration circle as it moves between calibration points
			Renderer->DrawPoint(Plane->position, glm::vec2(Plane->scale, Plane->scale), glm::vec3(180.0f, 180.0f, 180.0f), side);
		}
		else { //draws calibration circle and dot when at calibration point 
			Renderer->DrawPoint(Plane->position, glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), side);
			Renderer->DrawPoint(glm::vec3(Plane->position.x, Plane->position.y, Plane->position.z + offset), glm::vec2(Plane->scale, Plane->scale), glm::vec3(180.0f, 180.0f, 180.0f), side);
		}

	}
	else if (this->State == Calibration_2D_Result) {

		if (LinFit) { //draws points with linear fit
			for (int i = 1; i <= 9; i++) { //for each of the 9 2d calibration points
				Renderer->DrawPoint(Plane->positions[i], glm::vec2(1.0f, 1.0f), glm::vec3(244.0f, 0.0f, 0.0f), side);

				GLuint size = Plane->set[i - 1].set[0].point.size();
				for (int j = 0; j < size; j++) {
					GLfloat colour = (((float)j + 1.0) / (float)size); //sets raw data point colour based on how late in the calibration it was recorded (brighter = more recent)
					if (RawR) 
						Renderer->DrawPoint(glm::vec3((Plane->set[i - 1].set[0].point.at(j).xR)*Plane->xr_Slope + Plane->xr_Intercept, (Plane->set[i - 1].set[0].point.at(j).yR)*Plane->yr_Slope + Plane->yr_Intercept, -0.92f), glm::vec2(0.3f, 0.3f), glm::vec3(0.0f, 0.0f, colour), side);
					if (RawL) 
						Renderer->DrawPoint(glm::vec3((Plane->set[i - 1].set[0].point.at(j).xL)*Plane->xl_Slope + Plane->xl_Intercept, (Plane->set[i - 1].set[0].point.at(j).yL)*Plane->yl_Slope + Plane->yl_Intercept, -0.92f), glm::vec2(0.3f, 0.3f), glm::vec3(0.0f, colour, 0.0f), side);		
				}

				if (AverageL) 
					Renderer->DrawPoint(glm::vec3((Plane->set[i - 1].set[0].m1.xl)*Plane->xl_Slope + Plane->xl_Intercept, (Plane->set[i - 1].set[0].m1.yl)*Plane->yl_Slope + Plane->yl_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
				if (AverageR) 
					Renderer->DrawPoint(glm::vec3((Plane->set[i - 1].set[0].m1.xr)*Plane->xr_Slope + Plane->xr_Intercept, (Plane->set[i - 1].set[0].m1.yr)*Plane->yr_Slope + Plane->yr_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
			}
			
		}
		else { 
			for (int i = 1; i <= 9; i++) { 
				Renderer->DrawPoint(Plane->positions[i], glm::vec2(1.0f, 1.0f), glm::vec3(244.0f, 0.0f, 0.0f), side);

				GLuint size = Plane->set[i - 1].set[0].point.size();
				for (int j = 0; j < size; j++) {	
					GLfloat colour =(((float)j + 1.0) / (float)size);
			    	if (RawR) 
						Renderer->DrawPoint(glm::vec3(Plane->set[i - 1].set[0].point.at(j).xR, Plane->set[i - 1].set[0].point.at(j).yR, -0.92f), glm::vec2(0.3f, 0.3f), glm::vec3(0.0f, 0.0f, colour), side);
					if (RawL) 
						Renderer->DrawPoint(glm::vec3(Plane->set[i - 1].set[0].point.at(j).xL, Plane->set[i - 1].set[0].point.at(j).yL, -0.92f), glm::vec2(0.3f, 0.3f), glm::vec3(0.0f, colour, 0.0f), side);		
				}

				if (AverageL) 
					Renderer->DrawPoint(glm::vec3(Plane->set[i - 1].set[0].m1.xl, Plane->set[i - 1].set[0].m1.yl, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 244.0f, 0.0f), side);
				if (AverageR) 
					Renderer->DrawPoint(glm::vec3(Plane->set[i - 1].set[0].m1.xr, Plane->set[i - 1].set[0].m1.yr, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 244.0f), side);
			}
		}
	}
	else if (this->State == Calibration_3D) { //similar to 2d calibration
		if (Depth->transition) {
			Renderer->DrawPoint(Depth->position, glm::vec2(Depth->scale, Depth->scale), glm::vec3(180.0f, 180.0f, 180.0f), side);
		}
		else {
			Renderer->DrawPoint(Depth->position, glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), side);
			Renderer->DrawPoint(glm::vec3(Depth->position.x, Depth->position.y, Depth->position.z + offset), glm::vec2(Depth->scale, Depth->scale), glm::vec3(180.0f, 180.0f, 180.0f), side);
		}

		SRenderer->DrawSprite(ResourceManager::GetTexture("lrgrid"), glm::vec3(-1 * (ScreenWidth / 2.0), -1 * (ScreenHeight / 2.0), -.92), glm::vec2(1.0, ScreenHeight), 0.0f, 90.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //left wall
		SRenderer->DrawSprite(ResourceManager::GetTexture("lrgrid"), glm::vec3((ScreenWidth / 2.0), -1 * (ScreenHeight / 2.0), -1.92), glm::vec2(1.0, ScreenHeight), 0.0f, -90.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //right wall
		SRenderer->DrawSprite(ResourceManager::GetTexture("tbgrid"), glm::vec3(-1 * (ScreenWidth / 2.0), -1 * (ScreenHeight / 2.0), -.92), glm::vec2(ScreenWidth, 1.0), -90.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //bottom wall
		SRenderer->DrawSprite(ResourceManager::GetTexture("tbgrid"), glm::vec3(-1 * (ScreenWidth / 2.0), (ScreenHeight / 2.0), -1.92), glm::vec2(ScreenWidth, 1.0), 90.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //top wall
		SRenderer->DrawSprite(ResourceManager::GetTexture("bgrid"), glm::vec3(-1 * (ScreenWidth / 2.0), -1 * (ScreenHeight / 2.0), -1.92), glm::vec2(ScreenWidth, ScreenHeight), 0.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //back wall
	}
	else if (this->State == Calibration_3D_Result) {
		for (int i = 1; i <= 9; i++) {
			GLuint location = plane * 9 + i;
			
			Renderer->DrawPoint(glm::vec3(Depth->set[location -1].rCoord.xl, Depth->set[location - 1].rCoord.yl, -0.92f), glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), side); //draws the location of each calibration each for each eye at the same time to allow the user to look at theyre results without the shutter glasses on
			Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].rCoord.xr, Depth->set[location - 1].rCoord.yr, -0.92f), glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), side);

			if (LinFit) {
				GLuint size = Depth->set[location - 1].set[sets].point.size();
				for (int j = 0; j < size; j++) {

					GLfloat colour = (((float)j + 1.0) / (float)size);
					if (RawR) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].point.at(j).xR)*Plane->xr_Slope + Plane->xr_Intercept, (Depth->set[location - 1].set[sets].point.at(j).yR)*Plane->yr_Slope + Plane->yr_Intercept, -0.92f), glm::vec2(0.3f, 0.3f), glm::vec3(0.0f, 0.0f, colour), side);
					if (RawL) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].point.at(j).xL)*Plane->xl_Slope + Plane->xl_Intercept, (Depth->set[location - 1].set[sets].point.at(j).yL)*Plane->yl_Slope + Plane->yl_Intercept, -0.92f), glm::vec2(0.3f, 0.3f), glm::vec3(0.0f, colour, 0.0f), side);
				}

				if (AverageL) { //draws theparticular cleaning method selected by user
					if (average) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m1.xl)*Plane->xl_Slope + Plane->xl_Intercept, (Depth->set[location - 1].set[sets].m1.yl)*Plane->yl_Slope + Plane->yl_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);	
					else if (clean_average) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m2.xl)*Plane->xl_Slope + Plane->xl_Intercept, (Depth->set[location - 1].set[sets].m2.yl)*Plane->yl_Slope + Plane->yl_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
					else if (weighted_average) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m3.xl)*Plane->xl_Slope + Plane->xl_Intercept, (Depth->set[location - 1].set[sets].m3.yl)*Plane->yl_Slope + Plane->yl_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
					else if (median) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m4.xl)*Plane->xl_Slope + Plane->xl_Intercept, (Depth->set[location - 1].set[sets].m4.yl)*Plane->yl_Slope + Plane->yl_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
					else if (dti) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m5.xl)*Plane->xl_Slope + Plane->xl_Intercept, (Depth->set[location - 1].set[sets].m5.yl)*Plane->yl_Slope + Plane->yl_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
				}

				if (AverageR) {
					if (average) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m1.xr)*Plane->xr_Slope + Plane->xr_Intercept, (Depth->set[location - 1].set[sets].m1.yr)*Plane->yr_Slope + Plane->yr_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
					else if (clean_average) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m2.xr)*Plane->xr_Slope + Plane->xr_Intercept, (Depth->set[location - 1].set[sets].m2.yr)*Plane->yr_Slope + Plane->yr_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
					else if (weighted_average) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m3.xr)*Plane->xr_Slope + Plane->xr_Intercept, (Depth->set[location - 1].set[sets].m3.yr)*Plane->yr_Slope + Plane->yr_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
					else if (median) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m4.xr)*Plane->xr_Slope + Plane->xr_Intercept, (Depth->set[location - 1].set[sets].m4.yr)*Plane->yr_Slope + Plane->yr_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
					else if (dti) 
						Renderer->DrawPoint(glm::vec3((Depth->set[location - 1].set[sets].m5.xr)*Plane->xr_Slope + Plane->xr_Intercept, (Depth->set[location - 1].set[sets].m5.yr)*Plane->yr_Slope + Plane->yr_Intercept, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
				}
			}
			else {
				GLuint size = Depth->set[location - 1].set[sets].point.size();
				for (int j = 0; j < size; j++) {
					GLfloat colour = (((float)j + 1.0) / (float)size);

					if (RawR) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].point.at(j).xR, Depth->set[location - 1].set[sets].point.at(j).yR, -0.92f), glm::vec2(0.3f, 0.3f), glm::vec3(0.0f, 0.0f, colour), side);
					if (RawL) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].point.at(j).xL, Depth->set[location - 1].set[sets].point.at(j).yL, -0.92f), glm::vec2(0.3f, 0.3f), glm::vec3(0.0f, colour, 0.0f), side);
				}

				if (AverageL) {
					if (average) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m1.xl, Depth->set[location - 1].set[sets].m1.yl, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
					else if (clean_average) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m2.xl, Depth->set[location - 1].set[sets].m2.yl, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
					else if (weighted_average) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m3.xl, Depth->set[location - 1].set[sets].m3.yl, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
					else if (median) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m4.xl, Depth->set[location - 1].set[sets].m4.yl, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
					else if (dti) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m5.xl, Depth->set[location - 1].set[sets].m5.yl, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), side);
				}

				if (AverageR) {
					if (average) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m1.xr, Depth->set[location - 1].set[sets].m1.yr, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
					else if (clean_average) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m2.xr, Depth->set[location - 1].set[sets].m2.yr, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
					else if (weighted_average) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m3.xr, Depth->set[location - 1].set[sets].m3.yr, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
					else if (median) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m4.xr, Depth->set[location - 1].set[sets].m4.yr, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
					else if (dti) 
						Renderer->DrawPoint(glm::vec3(Depth->set[location - 1].set[sets].m5.xr, Depth->set[location - 1].set[sets].m5.yr, -0.92f), glm::vec2(0.8f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f), side);
				}
			}
		}
	}
	else if (this->State == SB) {
		if (points) { //target practice game
			for (GLuint i = 0; i < Sand->positionCount; i++) { //draws each of the targets
				Renderer->DrawPoint(Sand->position[i], glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), side);
				Renderer->DrawPoint(glm::vec3(Sand->position[i].x, Sand->position[i].y, Sand->position[i].z + offset), glm::vec2(Sand->scale, Sand->scale), glm::vec3(180.0f, 180.0f, 180.0f), side);
			}

			//draws the depth meter and gaze depths on the right hand side of screen
			SRenderer->DrawSprite(ResourceManager::GetTexture("depth"), glm::vec3((ScreenWidth / 2.0) - 0.018, -1 * ScreenHeight / 2.0, -.921), glm::vec2(0.018, ScreenHeight), 0.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f));
			Renderer->DrawPoint(glm::vec3((ScreenWidth / 2.0) - 0.008, (-ScreenHeight * (Sand->gazepos.z + 0.92)) - (ScreenHeight / 2.0), -0.9205), glm::vec2(2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), side); //depth poitn of raw data average (black)
			
	
			Renderer->DrawPoint(Sand->gazepos, glm::vec2(2.0f, 2.0f), glm::vec3(254.0f, 153.0f, 0.0f), side);  //draws the actual gaze position

			//draws the walls of box
			SRenderer->DrawSprite(ResourceManager::GetTexture("lrgrid"), glm::vec3(-1 * (ScreenWidth / 2.0), -1 * (ScreenHeight / 2.0), -.92), glm::vec2(1.0, ScreenHeight), 0.0f, 90.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //left wall
			SRenderer->DrawSprite(ResourceManager::GetTexture("lrgrid"), glm::vec3((ScreenWidth / 2.0), -1 * (ScreenHeight / 2.0), -1.92), glm::vec2(1.0, ScreenHeight), 0.0f, -90.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //right wall
			SRenderer->DrawSprite(ResourceManager::GetTexture("tbgrid"), glm::vec3(-1 * (ScreenWidth / 2.0), -1 * (ScreenHeight / 2.0), -.92), glm::vec2(ScreenWidth, 1.0), -90.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //bottom wall
			SRenderer->DrawSprite(ResourceManager::GetTexture("tbgrid"), glm::vec3(-1 * (ScreenWidth / 2.0), (ScreenHeight / 2.0), -1.92), glm::vec2(ScreenWidth, 1.0), 90.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //top wall
			SRenderer->DrawSprite(ResourceManager::GetTexture("bgrid"), glm::vec3(-1 * (ScreenWidth / 2.0), -1 * (ScreenHeight / 2.0), -1.92), glm::vec2(ScreenWidth, ScreenHeight), 0.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); //back wall
		}
		else if(imageView){ //image viewing
			SRenderer->DrawSprite(ResourceManager::GetTexture("stones"), glm::vec3(-2.2 * (ScreenWidth / 2.0), -2.2 * (ScreenHeight / 2.0), -1.0*(Sand->depth) - 0.92), glm::vec2(2.2*ScreenWidth, 2.2*ScreenHeight), 0.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); 

			//draws the depth meter and gaze depths on the right hand side of screen
			SRenderer->DrawSprite(ResourceManager::GetTexture("depth"), glm::vec3((ScreenWidth / 2.0) - 0.018, -1 * ScreenHeight / 2.0, -.919), glm::vec2(0.018, ScreenHeight), 0.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f)); 
			Renderer->DrawPoint(glm::vec3((ScreenWidth / 2.0) - 0.008, (-ScreenHeight * (Sand->gazepos.z + 0.92)) - (ScreenHeight / 2.0), -0.9189), glm::vec2(2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), side);
			
		}
		else if (staticPoints) { //set of nine static targets
			for (int i = 0; i < 9; i++) {
				Renderer->DrawPoint(Sand->staticPos[i], glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), side);
				Renderer->DrawPoint(glm::vec3(Sand->staticPos[i].x, Sand->staticPos[i].y, Sand->staticPos[i].z + offset), glm::vec2(Sand->scale, Sand->scale), glm::vec3(180.0f, 180.0f, 180.0f), side);
			}

			//draws the depth meter and gaze depths on the right hand side of screen
			SRenderer->DrawSprite(ResourceManager::GetTexture("depth"), glm::vec3((ScreenWidth / 2.0) - 0.018, -1 * ScreenHeight / 2.0, -.921), glm::vec2(0.018, ScreenHeight), 0.0f, 0.0f, side, glm::vec3(1.0f, 1.0f, 1.0f));
			Renderer->DrawPoint(glm::vec3((ScreenWidth / 2.0) - 0.008, (-ScreenHeight * (Sand->gazepos.z + 0.92)) - (ScreenHeight / 2.0), -0.9205), glm::vec2(2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), side); //depth poitn of raw data average (black)


			Renderer->DrawPoint(Sand->gazepos, glm::vec2(2.0f, 2.0f), glm::vec3(254.0f, 153.0f, 0.0f), side);  //draws the actual gaze position

		}
	}
	else if (this->State == VolRen) {
			//volume rendering code
	}
	

	
}

//reads latest gaze location from the gaze poitn API
int Control::Gaze_Location(GLfloat &xl, GLfloat &yl, GLfloat &xr, GLfloat &yr)
{
	
	string rxstr = client.get_rx_latest();

	string validR = "";
	string validL = "";
	float tempX = 0.0f;
	float tempY = 0.0f;

	int start_index = 0;

	int leftValid = 0;
	int rightValid = 0;

	if (rxstr != "") 
	{
		if (rxstr.substr(1, 3) == "REC") { //checks to see if the curretn pulled string is gaze data

			start_index = rxstr.find("LPOGV=", 0);
			validL = rxstr.at(start_index + 7);

			if (validL == "1") { //makes sure the left eye data is valid

				start_index = rxstr.find("LPOGX=", 0);
				tempX = std::stof(rxstr.substr(start_index + 7, 7));

				start_index = rxstr.find("LPOGY=", 0);
				tempY = std::stof(rxstr.substr(start_index + 7, 7));

				if (tempX > 0.0 && tempY > 0.0 && tempX < 1.0 && tempY < 1.0) {
					leftValid = 1;
					xl = ScreenWidth * (tempX - 0.5); // gives a value -1 to 1
					yl = ScreenHeight * (-1.0 * (tempY - 0.5)); // gives a value -1 to 1
				}
			}


			start_index = rxstr.find("RPOGV=", 0);
			validR = rxstr.at(start_index + 7);

			if (validR == "1") { //makes sure the right eye data is valid

				start_index = rxstr.find("RPOGX=", 0);
				tempX = std::stof(rxstr.substr(start_index + 7, 7));

				start_index = rxstr.find("RPOGY=", 0);
				tempY = std::stof(rxstr.substr(start_index + 7, 7));

				if (tempX > 0.0 && tempY > 0.0 && tempX < 1.0 && tempY < 1.0) {
					rightValid = 1;
					xr = ScreenWidth *(tempX - 0.5); // gives a value -1 to 1
					yr = ScreenHeight *(-1.0 * (tempY - 0.5)); // gives a value -1 to 1
				}
			}



			if (leftValid && rightValid) { //if both eyesgaze positions are valid the data is stored (this ensures that each set of gaze positions were acquired at the same time)
				return 1;	
			}

			return 0;

		}
	}


}

/*
	Code included in LinReg and SymmetricMatrixInvert modified from existing algorithm for weighted linear regression written
	by: Walt Fair, Jr 
	Original code can be found at : http://www.codeproject.com/Articles/25335/An-Algorithm-for-Weighted-Linear-Regression
*/



bool Control::LinReg() { //used to perform multivariate lienar regression 

	int N = 3;
	double B[3];

	// Clear matrices
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			V[i][j] = 0;

	// Form Least Squares Matrix
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			V[i][j] = 0;
			for (int k = 0; k < 36; k++)
				V[i][j] = V[i][j] + X[i][k] * X[j][k];
		}
		B[i] = 0;
		for (int k = 0; k < 36; k++)
			B[i] = B[i] + X[i][k] * Y[k];
	}

	if (!SymmetricMatrixInvert())
	{
		return false;
	}

	for (int i = 0; i < N; i++) {
		C[i] = 0;
		for (int j = 0; j < N; j++)
			C[i] = C[i] + V[i][j] * B[j];
	}

	// Calculate statistics
	double TSS = 0;
	double RSS = 0;
	double YBAR = 0;
	double WSUM = 0;

	for (int k = 0; k < 36; k++)
	{
		YBAR = YBAR + Y[k];
		WSUM = WSUM + 1;
	}
	YBAR = YBAR / WSUM;
	for (int k = 0; k < 36; k++)
	{
		Ycalc[k] = 0;
		for (int i = 0; i < N; i++)
			Ycalc[k] = Ycalc[k] + C[i] * X[i][k];
		DY[k] = Ycalc[k] - Y[k];
		TSS = TSS + (Y[k] - YBAR) * (Y[k] - YBAR);
		RSS = RSS + DY[k] * DY[k];
	}
	double SSQ = RSS / (36-N);
	RYSQ = 1 - RSS / TSS;
	FReg = 9999999;
	if (RYSQ < 0.9999999)
		FReg = RYSQ / (1 - RYSQ) * (36-N) / (N - 1);
	SDV = sqrt(SSQ);

	// Calculate var-covar matrix and std error of coefficients
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
			V[i][j] = V[i][j] * SSQ;
		SEC[i] = sqrt(V[i][i]);
	}
	return true;
}




bool Control::SymmetricMatrixInvert() {
	int N = (int)sqrt(9);

	double t[3];
	double Q[3];
	double R[3];
	double AB;
	int K, L, M;

	// Invert a symetric matrix in V
	for (M = 0; M < N; M++)
		R[M] = 1;
	K = 0;
	for (M = 0; M < N; M++)
	{
		double Big = 0;
		for (L = 0; L < N; L++)
		{
			AB = abs(V[L][L]);
			if ((AB > Big) && (R[L] != 0))
			{
				Big = AB;
				K = L;
			}
		}
		if (Big == 0)
		{
			return false;
		}
		R[K] = 0;
		Q[K] = 1 / V[K][K];
		t[K] = 1;
		V[K][K] = 0;
		if (K != 0)
		{
			for (L = 0; L < K; L++)
			{
				t[L] = V[L][K];
				if (R[L] == 0)
					Q[L] = V[L][K] * Q[K];
				else
					Q[L] = -V[L][K] * Q[K];
				V[L][K] = 0;
			}
		}
		if ((K + 1) < N)
		{
			for (L = K + 1; L < N; L++)
			{
				if (R[L] != 0)
					t[L] = V[K][L];
				else
					t[L] = -V[K][L];
				Q[L] = -V[K][L] * Q[K];
				V[K][L] = 0;
			}
		}
		for (L = 0; L < N; L++)
			for (K = L; K < N; K++)
				V[L][K] = V[L][K] + t[L] * Q[K];
	}
	M = N;
	L = N - 1;
	for (K = 1; K < N; K++)
	{
		M = M - 1;
		L = L - 1;
		for (int J = 0; J <= L; J++)
			V[M][J] = V[J][M];
	}
	return true;
}



//Used to print 3D gaze positions calculated by three different calibration methods
void Control::PrintDepthResults() {
	std::string filename = "Depth_Test_V2.txt";


	GLfloat avglX = 0.0f;
	GLfloat avglY = 0.0f;
	GLfloat avgrX = 0.0f;
	GLfloat avgrY = 0.0f;

	GLfloat gX = 0.0f;
	GLfloat gY = 0.0f;
	GLfloat gZ = 0.0f;
	

	std::ofstream dataFile;
	dataFile.open(filename);




	dataFile << "Depth Test" << "\n";
	dataFile << "Time per calibration point: " << std::to_string(Depth->HoldTime) << " ms" << "\n";
	dataFile << "All values in meters. Origin at screen centre. +Z towards user. +X to the user right. +Y users up" << "\n";
	dataFile << "Plane depths: ( " << Depth->planedepths[0] << ", " << Depth->planedepths[1] << ", " << Depth->planedepths[2] << ", " << Depth->planedepths[3] << " )" << "\n" << "\n";



	for (int i = 0; i < 9; i++) {
		dataFile << "\n" << "Position " + std::to_string(i + 1) << "\n" << "\n";

		dataFile << "Depth 1" << "\n";
		dataFile << "3D Position" << "( " << Depth->set[i].position.x << " , " << Depth->set[i].position.y << " , " << Depth->set[i].position.z + 0.92 << " )" << "\n" << "\n";
		for (int j = 0; j < Depth->setnum; j++) {
			dataFile << "Test " << std::to_string(j + 1) << "\n";

			avglX = Depth->set[i].set[j].m1.xl;
			avgrX = Depth->set[i].set[j].m1.xr;
			avglY = Depth->set[i].set[j].m1.yl;
			avgrY = Depth->set[i].set[j].m1.yr;

			dataFile << "No Fit ( " << Depth->set[i].set[j].m1.gx << " , " << Depth->set[j].set[j].m1.gy << " , " << Depth->set[j].set[j].m1.gz + 0.92 << " )" << "\n";

			avglX = avglX*Depth->xl_Slope + Depth->xl_Intercept;
			avgrX = avgrX*Depth->xr_Slope + Depth->xr_Intercept;
			avglY = avglY*Depth->yl_Slope + Depth->yl_Intercept;
			avgrY = avgrY*Depth->yr_Slope + Depth->yr_Intercept;

			double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);
			gZ = t * (-1.0 * Convergence); //meters towards user from screen

			dataFile << "Linear Fit ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n";

			avglX = Depth->set[i].set[j].m1.xl;
			avgrX = Depth->set[i].set[j].m1.xr;
			avglY = Depth->set[i].set[j].m1.yl;
			avgrY = Depth->set[i].set[j].m1.yr;

			gZ = Sand->R1[0] * ((avglX + avgrX) / 2.0) + Sand->R1[1] * ((avglY + avgrY) / 2.0) + Sand->R1[2] * (avgrX - avglX);
			t = gZ / (-1.0 * Convergence);
			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);

			dataFile << "Lin Reg 1 ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n";

			t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);
			gZ = t * (-1.0 * Convergence); //meters towards user from screen

			gX = Sand->R2x[0] * gX + Sand->R2x[1] * gY + Sand->R2x[2] * gZ;
			gY = Sand->R2y[0] * gX + Sand->R2y[1] * gY + Sand->R2y[2] * gZ;
			gZ = Sand->R2z[0] * gX + Sand->R2z[1] * gY + Sand->R2z[2] * gZ;

			dataFile << "Lin Reg 2 ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n" << "\n";
		}
		dataFile << "Depth 2" << "\n";
		dataFile << "3D Position" << "( " << Depth->set[i + 9].position.x << " , " << Depth->set[i + 9].position.y << " , " << Depth->set[i + 9].position.z + 0.92 << " )" << "\n" << "\n";
		for (int j = 0; j < Depth->setnum; j++) {
			dataFile << "Test " << std::to_string(j + 1) << "\n";

			avglX = Depth->set[i + 9].set[j].m1.xl;
			avgrX = Depth->set[i + 9].set[j].m1.xr;
			avglY = Depth->set[i + 9].set[j].m1.yl;
			avgrY = Depth->set[i + 9].set[j].m1.yr;

			dataFile << "No Fit ( " << Depth->set[i].set[j].m1.gx << " , " << Depth->set[j].set[j].m1.gy << " , " << Depth->set[j].set[j].m1.gz + 0.92 << " )" << "\n";

			avglX = avglX*Depth->xl_Slope + Depth->xl_Intercept;
			avgrX = avgrX*Depth->xr_Slope + Depth->xr_Intercept;
			avglY = avglY*Depth->yl_Slope + Depth->yl_Intercept;
			avgrY = avgrY*Depth->yr_Slope + Depth->yr_Intercept;

			double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);
			gZ = t * (-1.0 * Convergence); //meters towards user from screen

			dataFile << "Linear Fit ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n";

			avglX = Depth->set[i + 9].set[j].m1.xl;
			avgrX = Depth->set[i + 9].set[j].m1.xr;
			avglY = Depth->set[i + 9].set[j].m1.yl;
			avgrY = Depth->set[i + 9].set[j].m1.yr;

			gZ = Sand->R1[0] * ((avglX + avgrX) / 2.0) + Sand->R1[1] * ((avglY + avgrY) / 2.0) + Sand->R1[2] * (avgrX - avglX);
			t = gZ / (-1.0 * Convergence);
			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);

			dataFile << "Lin Reg 1 ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n";

			t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);
			gZ = t * (-1.0 * Convergence); //meters towards user from screen

			gX = Sand->R2x[0] * gX + Sand->R2x[1] * gY + Sand->R2x[2] * gZ;
			gY = Sand->R2y[0] * gX + Sand->R2y[1] * gY + Sand->R2y[2] * gZ;
			gZ = Sand->R2z[0] * gX + Sand->R2z[1] * gY + Sand->R2z[2] * gZ;

			dataFile << "Lin Reg 2 ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n" << "\n";
		}
		dataFile << "Depth 3" << "\n";
		dataFile << "3D Position" << "( " << Depth->set[i + 18].position.x << " , " << Depth->set[i + 18].position.y << " , " << Depth->set[i + 18].position.z + 0.92 << " )" << "\n" << "\n";
		for (int j = 0; j < Depth->setnum; j++) {
			dataFile << "Test " << std::to_string(j + 1) << "\n";

			avglX = Depth->set[i + 18].set[j].m1.xl;
			avgrX = Depth->set[i + 18].set[j].m1.xr;
			avglY = Depth->set[i + 18].set[j].m1.yl;
			avgrY = Depth->set[i + 18].set[j].m1.yr;

			dataFile << "No Fit ( " << Depth->set[i].set[j].m1.gx << " , " << Depth->set[j].set[j].m1.gy << " , " << Depth->set[j].set[j].m1.gz + 0.92 << " )" << "\n";

			avglX = avglX*Depth->xl_Slope + Depth->xl_Intercept;
			avgrX = avgrX*Depth->xr_Slope + Depth->xr_Intercept;
			avglY = avglY*Depth->yl_Slope + Depth->yl_Intercept;
			avgrY = avgrY*Depth->yr_Slope + Depth->yr_Intercept;

			double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);
			gZ = t * (-1.0 * Convergence); //meters towards user from screen

			dataFile << "Linear Fit ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n";

			avglX = Depth->set[i + 18].set[j].m1.xl;
			avgrX = Depth->set[i + 18].set[j].m1.xr;
			avglY = Depth->set[i + 18].set[j].m1.yl;
			avgrY = Depth->set[i + 18].set[j].m1.yr;

			gZ = Sand->R1[0] * ((avglX + avgrX) / 2.0) + Sand->R1[1] * ((avglY + avgrY) / 2.0) + Sand->R1[2] * (avgrX - avglX);
			t = gZ / (-1.0 * Convergence);
			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);

			dataFile << "Lin Reg 1 ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n";

			t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);
			gZ = t * (-1.0 * Convergence); //meters towards user from screen

			gX = Sand->R2x[0] * gX + Sand->R2x[1] * gY + Sand->R2x[2] * gZ;
			gY = Sand->R2y[0] * gX + Sand->R2y[1] * gY + Sand->R2y[2] * gZ;
			gZ = Sand->R2z[0] * gX + Sand->R2z[1] * gY + Sand->R2z[2] * gZ;

			dataFile << "Lin Reg 2 ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n" << "\n";
		}
		dataFile << "Depth 4" << "\n";
		dataFile << "3D Position" << "( " << Depth->set[i + 27].position.x << " , " << Depth->set[i + 27].position.y << " , " << Depth->set[i + 27].position.z + 0.92 << " )" << "\n" << "\n";
		for (int j = 0; j < Depth->setnum; j++) {
			dataFile << "Test " << std::to_string(j + 1) << "\n";

			avglX = Depth->set[i + 27].set[j].m1.xl;
			avgrX = Depth->set[i + 27].set[j].m1.xr;
			avglY = Depth->set[i + 27].set[j].m1.yl;
			avgrY = Depth->set[i + 27].set[j].m1.yr;

			dataFile << "No Fit ( " << Depth->set[i].set[j].m1.gx << " , " << Depth->set[j].set[j].m1.gy << " , " << Depth->set[j].set[j].m1.gz + 0.92 << " )" << "\n";

			avglX = avglX*Depth->xl_Slope + Depth->xl_Intercept;
			avgrX = avgrX*Depth->xr_Slope + Depth->xr_Intercept;
			avglY = avglY*Depth->yl_Slope + Depth->yl_Intercept;
			avgrY = avgrY*Depth->yr_Slope + Depth->yr_Intercept;

			double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);
			gZ = t * (-1.0 * Convergence); //meters towards user from screen

			dataFile << "Linear Fit ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n";

			avglX = Depth->set[i + 27].set[j].m1.xl;
			avgrX = Depth->set[i + 27].set[j].m1.xr;
			avglY = Depth->set[i + 27].set[j].m1.yl;
			avgrY = Depth->set[i + 27].set[j].m1.yr;

			gZ = Sand->R1[0] * ((avglX + avgrX) / 2.0) + Sand->R1[1] * ((avglY + avgrY) / 2.0) + Sand->R1[2] * (avgrX - avglX);
			t = gZ / (-1.0 * Convergence);
			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);

			dataFile << "Lin Reg 1 ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n";

			t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

			gX = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gY = t*((avglY + avgrY) / 2.0);
			gZ = t * (-1.0 * Convergence); //meters towards user from screen

			gX = Sand->R2x[0] * gX + Sand->R2x[1] * gY + Sand->R2x[2] * gZ;
			gY = Sand->R2y[0] * gX + Sand->R2y[1] * gY + Sand->R2y[2] * gZ;
			gZ = Sand->R2z[0] * gX + Sand->R2z[1] * gY + Sand->R2z[2] * gZ;

			dataFile << "Lin Reg 2 ( " << gX << " , " << gY << " , " << gZ + 0.92 << " )" << "\n" << "\n";
		}
	}
}
