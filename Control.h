#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>


enum State {
	Calibration_2D, //Runs calibration through 9 points located at the convergenc epoint (not 3d) and stores and organzies all eye track data
	Calibration_2D_Result, //Dispalys eye track data fornm 2d calibration on monitor 
	Calibration_3D, //Runs claibration through 36 points located at 4 depths while storing and roganizing all eye track date
	Calibration_3D_Result, //Displays eye track data from 3d calibration on monitor
	SB,
	VolRen //not implemented
};


class Control
{
public:
	State			State;
	GLboolean		Keys[1024];
	GLuint			Width, Height;


	GLboolean PlaneComplete = 0; //indicates that the 2d calibration has been completed
	GLboolean DepthComplete = 0; //indicates that the 3d calibration has been completed



	GLboolean RawL = 1; //indicates whether to draw the raw left eye track data during 2D_Result and 3D_Result
	GLboolean AverageL = 0; //indicates whether to draw cleans left eye track data during 2D_Result and 3D_Result
	GLboolean RawR = 1; //indicates whether to draw the raw right eye track data during 2D_Result and 3D_Result
	GLboolean AverageR = 0; //indicates whether to draw the clean right eye track data during 2D_Result and 3D_Result

	GLboolean LinFit = 0; //adjust both raw and clean data based on a linear fit
	
	//Indicates which method is used to calculate the clean data forn raw
	GLboolean average = 1;
	GLboolean clean_average = 0;
	GLboolean weighted_average = 0;
	GLboolean median = 0;
	GLboolean dti = 0;

	
	GLboolean linF = 0; //used to indicate if linear fit was performed after the 2d calibration and it was successful
	GLboolean linR = 0; // used to indicate if linear regression was performed after 3d claibration and it was successsful


	GLuint plane = 0; //idicates current plane shown in 3d result
	
	//Used to select which mini game is displayed during Sanbox stage
	GLboolean points = 1; 
	GLboolean imageView = 0;
	GLboolean staticPoints = 0;

	GLuint sets = 0; //indicates which set from the 3d calibration is shown (3d calibration can be run a maximum of 4 times in a row in order to identify any varianc ein the data 

	//Linear Regression Information
	double V[3][3]; // Least squares and var / covar matrix
	double C[3]; // Coefficients
	double SEC[3];    // Std Error of coefficients
	double RYSQ;            // Multiple correlation coefficient
	double SDV;             // Standard deviation of errors
	double FReg;            // Fisher F statistic for regression
	double Ycalc[36];         // Calculated values of Y
	double DY[36];            // Residual values of Y

	double X[3][36]; // Holds X, Y, Eye Seperation for all 36 calibration points   
	double Y[36]; // Holds Depth at each calibration point

	double R1[3]; //holds coefficients from first regression

	double R2x[3]; //holds coeffients from second regression on x
	double R2y[3]; //holds coeffients from second regression on y
	double R2z[3]; //holds coeffients from second regression on z
	


	unsigned long dt; //delta time
	unsigned long lastpress = 0;


	Control(GLuint width, GLuint height);
	~Control();

	void Init(double start);
	void ProcessInput(double ct);
	void Update(double ct);
	void RenderLeft();
	void RenderRight();
	bool LinReg();
	bool SymmetricMatrixInvert();
	int Gaze_Location(GLfloat &xl, GLfloat &yl, GLfloat &xr, GLfloat &yr);

	void Draw(GLuint side);
	void PrintDepthResults();
	 
};

