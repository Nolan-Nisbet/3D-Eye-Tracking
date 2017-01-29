#pragma once
#include <deque>


#include <vector>
#include <math.h> 
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>

struct depth{
	std::deque<GLfloat> z;
};

class DepthCalibration
{
	public:
		DepthCalibration(double start, GLuint setnum);
		~DepthCalibration();
		void Update(double ct);
		void Gaze_Data(GLfloat xl, GLfloat yl, GLfloat xr, GLfloat yr, double ct);
		void Clean();
		void Average();
		void Clean_Average();
		void Weighted_Average();
		void Median();
		void DTI();
		void Print();
		void Print_Test();
		void Linear_Fit();

		//Linear fit values passded in from 2d claibration
		GLfloat xl_Slope = 0.0f;
		GLfloat xr_Slope = 0.0f;
		GLfloat xl_Intercept = 0.0f;
		GLfloat xr_Intercept = 0.0f;

		GLfloat yl_Slope = 0.0f;
		GLfloat yr_Slope = 0.0f;
		GLfloat yl_Intercept = 0.0f;
		GLfloat yr_Intercept = 0.0f;

		//used for a linear fit of depths
		GLfloat depth_Slope = 0.0f;
		GLfloat depth_Intercept = 0.0f;
	

		glm::vec3 positions[37]; //positions of all 36 calibration points as well as the starting location of the target 

		float planedepths[4] = { //plane depths in respect to convergence plane
			0.0f, -0.15f, -0.30f, -0.45f
		};

		GLuint maxSets = 1;
		GLuint setnum = 0;
		GLuint setc = 0;
		GLuint positionC = 0;

		GLfloat avglX = 0.0f;
		GLfloat avglY = 0.0f;
		GLfloat avgrX = 0.0f;
		GLfloat avgrY = 0.0f;

		std::deque<Datapoint> isoSet; //used to take sections of the eye track data over the cours eof the calibration and run various algorithms on it
		
		CalibrationPoints set[36]; //Array of calibration poitns which hold all relevent information about each calibration point
		double timeStamps[73];
		const double TransitionTime = 700; //time between calibration points
		const double HoldTime = 1500; //time spent at each calibration points
		double start; //start time

		GLfloat scale;
		GLfloat scaleFactor = 6.0;
		glm::vec3 position; //current poistion of target
		GLuint calibrationPoint; //indicates which calibration point we are currently at or transitioning to
		bool transition; //indicates 
		GLuint currentStamp;

		bool complete = 0;

	private:
		float DepthCalibration::transitionP(unsigned long dTime, float pTime, float nTime, float pCoord, float nCoord);
		float DepthCalibration::scaleF(unsigned long dTime, float pTime, float nTime);
		
		
};

DepthCalibration::DepthCalibration(double start, GLuint setnum)
{

	this->setnum = setnum; 

	float thetaW = atan(((ScreenWidth / 2.0) + (EyeSeparation / 2.0)) / Convergence);
	float thetaH = atan(((ScreenHeight / 2.0)) / Convergence);

	int positionCount = 0;

	//3D position set
	positions[positionCount++] = glm::vec3(-1.2 * ((Convergence - planedepths[0]) * tan(thetaW) - (EyeSeparation / 2.0)), 0.8 * ((Convergence - planedepths[0]) * tan(thetaH)), -1.0 * (Convergence - planedepths[0])); //should start off the screen

	//sets the location of each point in relation to where it appears on the screen (this allows points at various depths to be displays in the same spot)
	for (int i = 0; i < 4; i++) {
		positions[positionCount++] = glm::vec3(-0.6 * ((Convergence - planedepths[i]) * tan(thetaW) - (EyeSeparation / 2.0)), 0.6 * ((Convergence - planedepths[i]) * tan(thetaH)), -1.0 *(Convergence - planedepths[i]));
		positions[positionCount++] = glm::vec3(0.0f, 0.6 * ((Convergence - planedepths[i]) * tan(thetaH)), -1.0 * (Convergence - planedepths[i]));
		positions[positionCount++] = glm::vec3(0.6 * ((Convergence - planedepths[i]) * tan(thetaW) - (EyeSeparation / 2.0)), 0.6 * ((Convergence - planedepths[i]) * tan(thetaH)), -1.0 * (Convergence - planedepths[i]));

		positions[positionCount++] = glm::vec3(0.6 * ((Convergence - planedepths[i]) * tan(thetaW) - (EyeSeparation / 2.0)), 0.0f, -1.0 * (Convergence - planedepths[i]));
		positions[positionCount++] = glm::vec3(0.0f, 0.0f, -1.0 * (Convergence - planedepths[i]));
		positions[positionCount++] = glm::vec3(-0.6 * ((Convergence - planedepths[i]) * tan(thetaW) - (EyeSeparation / 2.0)), 0.0f, -1.0 * (Convergence - planedepths[i]));

		positions[positionCount++] = glm::vec3(-0.6 * ((Convergence - planedepths[i]) * tan(thetaW) - (EyeSeparation / 2.0)), -0.6 * ((Convergence - planedepths[i]) * tan(thetaH)), -1.0 * (Convergence - planedepths[i]));
		positions[positionCount++] = glm::vec3(0.0f, -0.6 * ((Convergence - planedepths[i]) * tan(thetaH)), -1.0 * (Convergence - planedepths[i]));
		positions[positionCount++] = glm::vec3(0.6 * ((Convergence - planedepths[i]) * tan(thetaW) - (EyeSeparation / 2.0)), -0.6 * ((Convergence - planedepths[i]) * tan(thetaH)), -1.0 * (Convergence - planedepths[i]));


	}
	
	for (int i = 1; i < 37; i++) {
		set[i - 1].position.x = positions[i].x;
		set[i - 1].position.y = positions[i].y;
		set[i - 1].position.z = positions[i].z;

	}

	
	int coordCount = 0;


	//calculates the excact position each point is displayed ont eh screen for each eye
	for (int i = 0; i < 4; i++) {

		float outerX = 0.6 * (ScreenWidth / 2.0);
		float innerX = (((positions[(i * 9) + 3].x - (EyeSeparation / 2.0)) / (Convergence - planedepths[i]))*Convergence) + (EyeSeparation / 2.0);
		float bothY = 0.6 * (ScreenHeight / 2.0);

		float centreXl = (((EyeSeparation / 2.0) / (Convergence - planedepths[i])) *Convergence) - (EyeSeparation / 2.0);
		float centreXr = (-1.0 * (((EyeSeparation / 2.0) / (Convergence - planedepths[i])) * Convergence)) + (EyeSeparation / 2.0);


		set[coordCount].rCoord.xr = -1.0 * outerX; //Position 1, 10, 19, 28
		set[coordCount].rCoord.yr = bothY;
		set[coordCount].rCoord.xl = -1.0 * innerX;
		set[coordCount].rCoord.yl = bothY;

		coordCount++;

		set[coordCount].rCoord.xr = centreXr; //Position 2, 11, 20, 29
		set[coordCount].rCoord.yr = bothY;
		set[coordCount].rCoord.xl = centreXl;
		set[coordCount].rCoord.yl = bothY;

		coordCount++;

		set[coordCount].rCoord.xr = innerX; //Position 3, 12, 21, 30
		set[coordCount].rCoord.yr = bothY;
		set[coordCount].rCoord.xl = outerX;
		set[coordCount].rCoord.yl = bothY;

		coordCount++;

		set[coordCount].rCoord.xr = innerX; //Position 4, 13, 22, 31
		set[coordCount].rCoord.yr = 0.0f;
		set[coordCount].rCoord.xl = outerX;
		set[coordCount].rCoord.yl = 0.0f;

		coordCount++;

		set[coordCount].rCoord.xr = centreXr; //Position 5, 14, 23, 32
		set[coordCount].rCoord.yr = 0.0f;
		set[coordCount].rCoord.xl = centreXl;
		set[coordCount].rCoord.yl = 0.0f;

		coordCount++;

		set[coordCount].rCoord.xr = -1.0 * outerX; //Position 6, 15, 24, 33
		set[coordCount].rCoord.yr = 0.0f;
		set[coordCount].rCoord.xl = -1.0 * innerX;
		set[coordCount].rCoord.yl = 0.0f;

		coordCount++;

		set[coordCount].rCoord.xr = -1.0 * outerX; //Position 7, 16, 25, 34
		set[coordCount].rCoord.yr = -1.0 * bothY;
		set[coordCount].rCoord.xl = -1.0 * innerX;
		set[coordCount].rCoord.yl = -1.0 * bothY;

		coordCount++;

		set[coordCount].rCoord.xr = centreXr; //Position 8, 17, 26, 35
		set[coordCount].rCoord.yr = -1.0 * bothY;
		set[coordCount].rCoord.xl = centreXl;
		set[coordCount].rCoord.yl = -1.0 * bothY;

		coordCount++;

		set[coordCount].rCoord.xr = innerX; //Position 9, 18, 27, 36
		set[coordCount].rCoord.yr = -1.0 * bothY;
		set[coordCount].rCoord.xl = outerX;
		set[coordCount].rCoord.yl = -1.0 * bothY;

		coordCount++;
	}

	this->start = start;
	timeStamps[0] = start + 1500;
	int stampCount = 1;

	//generates time stamps used to move through the calibration
	for (int i = 0; i < 36; i++) {
		timeStamps[stampCount] = timeStamps[stampCount - 1] + TransitionTime;
		stampCount++;
		timeStamps[stampCount] = timeStamps[stampCount - 1] + HoldTime;
		stampCount++;
	}

}

DepthCalibration::~DepthCalibration()
{
}

//based on the current time we evaluate the position and size of the target
void DepthCalibration::Update(double ct) {
	double dt = ct - start;



	if (dt < timeStamps[0]) {
		calibrationPoint = 0;
		currentStamp = 0;
		transition = 0;
	}
	else if ((dt >= timeStamps[0]) && (dt < timeStamps[1])) { // transition to 1
		calibrationPoint = 1;
		currentStamp = 0;
		transition = 1;
	}
	else if ((dt >= timeStamps[1]) && (dt < timeStamps[2])) { // hold at 1
		calibrationPoint = 1;
		currentStamp = 1;
		transition = 0;
	}
	else if ((dt >= timeStamps[2]) && (dt < timeStamps[3])) { // transition to 2
		calibrationPoint = 2;
		currentStamp = 2;
		transition = 1;
	}
	else if ((dt >= timeStamps[3]) && (dt < timeStamps[4])) { // hold at 2
		calibrationPoint = 2;
		currentStamp = 3;
		transition = 0;
	}
	else if ((dt >= timeStamps[4]) && (dt < timeStamps[5])) { // transition to 3
		calibrationPoint = 3;
		currentStamp = 4;
		transition = 1;
	}
	else if ((dt >= timeStamps[5]) && (dt < timeStamps[6])) { // hold at 3
		calibrationPoint = 3;
		currentStamp = 5;
		transition = 0;
	}
	else if ((dt >= timeStamps[6]) && (dt < timeStamps[7])) { // transition to 4
		calibrationPoint = 4;
		currentStamp = 6;
		transition = 1;
	}
	else if ((dt >= timeStamps[7]) && (dt < timeStamps[8])) { // hold at 4
		calibrationPoint = 4;
		currentStamp = 7;
		transition = 0;
	}
	else if ((dt >= timeStamps[8]) && (dt < timeStamps[9])) { // transition to 5
		calibrationPoint = 5;
		currentStamp = 8;
		transition = 1;
	}
	else if ((dt >= timeStamps[9]) && (dt < timeStamps[10])) { // hold at 5
		calibrationPoint = 5;
		currentStamp = 9;
		transition = 0;
	}
	else if ((dt >= timeStamps[10]) && (dt < timeStamps[11])) { // transition to 6
		calibrationPoint = 6;
		currentStamp = 10;
		transition = 1;
	}
	else if ((dt >= timeStamps[11]) && (dt < timeStamps[12])) { // hold at 6
		calibrationPoint = 6;
		currentStamp = 11;
		transition = 0;
	}
	else if ((dt >= timeStamps[12]) && (dt < timeStamps[13])) { // transition to 7
		calibrationPoint = 7;
		currentStamp = 12;
		transition = 1;
	}
	else if ((dt >= timeStamps[13]) && (dt < timeStamps[14])) { // hold at 7
		calibrationPoint = 7;
		currentStamp = 13;
		transition = 0;
	}
	else if ((dt >= timeStamps[14]) && (dt < timeStamps[15])) { // transition to 8
		calibrationPoint = 8;
		currentStamp = 14;
		transition = 1;
	}
	else if ((dt >= timeStamps[15]) && (dt < timeStamps[16])) { // hold at 8
		calibrationPoint = 8;
		currentStamp = 15;
		transition = 0;
	}
	else if ((dt >= timeStamps[16]) && (dt < timeStamps[17])) { // transition to 9
		calibrationPoint = 9;
		currentStamp = 16;
		transition = 1;
	}
	else if ((dt >= timeStamps[17]) && (dt < timeStamps[18])) { // hold at 9
		calibrationPoint = 9;
		currentStamp = 17;
		transition = 0;
	}
	else if ((dt >= timeStamps[18]) && (dt < timeStamps[19])) { // transition to 10
		calibrationPoint = 10;
		currentStamp = 18;
		transition = 1;
	}
	else if ((dt >= timeStamps[19]) && (dt < timeStamps[20])) { // hold at 10
		calibrationPoint = 10;
		currentStamp = 19;
		transition = 0;
	}
	else if ((dt >= timeStamps[20]) && (dt < timeStamps[21])) { // transition to 11
		calibrationPoint = 11;
		currentStamp = 20;
		transition = 1;
	}
	else if ((dt >= timeStamps[21]) && (dt < timeStamps[22])) { // hold at 11
		calibrationPoint = 11;
		currentStamp = 21;
		transition = 0;
	}
	else if ((dt >= timeStamps[22]) && (dt < timeStamps[23])) { // transition to 12
		calibrationPoint = 12;
		currentStamp = 22;
		transition = 1;
	}
	else if ((dt >= timeStamps[23]) && (dt < timeStamps[24])) { // hold at 12
		calibrationPoint = 12;
		currentStamp = 23;
		transition = 0;
	}
	else if ((dt >= timeStamps[24]) && (dt < timeStamps[25])) { // transition to 13
		calibrationPoint = 13;
		currentStamp = 24;
		transition = 1;
	}
	else if ((dt >= timeStamps[25]) && (dt < timeStamps[26])) { // hold at 13
		calibrationPoint = 13;
		currentStamp = 25;
		transition = 0;
	}
	else if ((dt >= timeStamps[26]) && (dt < timeStamps[27])) { // transition to 14
		calibrationPoint = 14;
		currentStamp = 26;
		transition = 1;
	}
	else if ((dt >= timeStamps[27]) && (dt < timeStamps[28])) { // hold at 14
		calibrationPoint = 14;
		currentStamp = 27;
		transition = 0;
	}
	else if ((dt >= timeStamps[28]) && (dt < timeStamps[29])) { // transition to 15
		calibrationPoint = 15;
		currentStamp = 28;
		transition = 1;
	}
	else if ((dt >= timeStamps[29]) && (dt < timeStamps[30])) { // hold at 15
		calibrationPoint = 15;
		currentStamp = 29;
		transition = 0;
	}
	else if ((dt >= timeStamps[30]) && (dt < timeStamps[31])) { // transition to 16
		calibrationPoint = 16;
		currentStamp = 30;
		transition = 1;
	}
	else if ((dt >= timeStamps[31]) && (dt < timeStamps[32])) { // hold at 16
		calibrationPoint = 16;
		currentStamp = 31;
		transition = 0;
	}
	else if ((dt >= timeStamps[32]) && (dt < timeStamps[33])) { // transition to 17
		calibrationPoint = 17;
		currentStamp = 32;
		transition = 1;
	}
	else if ((dt >= timeStamps[33]) && (dt < timeStamps[34])) { // hold at 17
		calibrationPoint = 17;
		currentStamp = 33;
		transition = 0;
	}
	else if ((dt >= timeStamps[34]) && (dt < timeStamps[35])) { // transition to 18
		calibrationPoint = 18;
		currentStamp = 34;
		transition = 1;
	}
	else if ((dt >= timeStamps[35]) && (dt < timeStamps[36])) { // hold at 18
		calibrationPoint = 18;
		currentStamp = 35;
		transition = 0;
	}
	else if ((dt >= timeStamps[36]) && (dt < timeStamps[37])) { // transition to 19
		calibrationPoint = 19;
		currentStamp = 36;
		transition = 1;
	}
	else if ((dt >= timeStamps[37]) && (dt < timeStamps[38])) { // hold at 19
		calibrationPoint = 19;
		currentStamp = 37;
		transition = 0;
	}
	else if ((dt >= timeStamps[38]) && (dt < timeStamps[39])) { // transition to 20
		calibrationPoint = 20;
		currentStamp = 38;
		transition = 1;
	}
	else if ((dt >= timeStamps[39]) && (dt < timeStamps[40])) { // hold at 20
		calibrationPoint = 20;
		currentStamp = 39;
		transition = 0;
	}
	else if ((dt >= timeStamps[40]) && (dt < timeStamps[41])) { // transition to 21
		calibrationPoint = 21;
		currentStamp = 40;
		transition = 1;
	}
	else if ((dt >= timeStamps[41]) && (dt < timeStamps[42])) { // hold at 21
		calibrationPoint = 21;
		currentStamp = 41;
		transition = 0;
	}
	else if ((dt >= timeStamps[42]) && (dt < timeStamps[43])) { // transition to 22
		calibrationPoint = 22;
		currentStamp = 42;
		transition = 1;
	}
	else if ((dt >= timeStamps[43]) && (dt < timeStamps[44])) { // hold at 22
		calibrationPoint = 22;
		currentStamp = 43;
		transition = 0;
	}
	else if ((dt >= timeStamps[44]) && (dt < timeStamps[45])) { // transition to 23
		calibrationPoint = 23;
		currentStamp = 44;
		transition = 1;
	}
	else if ((dt >= timeStamps[45]) && (dt < timeStamps[46])) { // hold at 23
		calibrationPoint = 23;
		currentStamp = 45;
		transition = 0;
	}
	else if ((dt >= timeStamps[46]) && (dt < timeStamps[47])) { // transition to 24
		calibrationPoint = 24;
		currentStamp = 46;
		transition = 1;
	}
	else if ((dt >= timeStamps[47]) && (dt < timeStamps[48])) { // hold at 24
		calibrationPoint = 24;
		currentStamp = 47;
		transition = 0;
	}
	else if ((dt >= timeStamps[48]) && (dt < timeStamps[49])) { // transition to 25
		calibrationPoint = 25;
		currentStamp = 48;
		transition = 1;
	}
	else if ((dt >= timeStamps[49]) && (dt < timeStamps[50])) { // hold at 25
		calibrationPoint = 25;
		currentStamp = 49;
		transition = 0;
	}
	else if ((dt >= timeStamps[50]) && (dt < timeStamps[51])) { // transition to 26
		calibrationPoint = 26;
		currentStamp = 50;
		transition = 1;
	}
	else if ((dt >= timeStamps[51]) && (dt < timeStamps[52])) { // hold at 26
		calibrationPoint = 26;
		currentStamp = 51;
		transition = 0;
	}
	else if ((dt >= timeStamps[52]) && (dt < timeStamps[53])) { // transition to 27
		calibrationPoint = 27;
		currentStamp = 52;
		transition = 1;
	}
	else if ((dt >= timeStamps[53]) && (dt < timeStamps[54])) { // hold at 27
		calibrationPoint = 27;
		currentStamp = 53;
		transition = 0;
	}
	else if ((dt >= timeStamps[54]) && (dt < timeStamps[55])) { // transition to 28
		calibrationPoint = 28;
		currentStamp = 54;
		transition = 1;
	}
	else if ((dt >= timeStamps[55]) && (dt < timeStamps[56])) { // hold at 28
		calibrationPoint = 28;
		currentStamp = 55;
		transition = 0;
	}
	else if ((dt >= timeStamps[56]) && (dt < timeStamps[57])) { // transition to 29
		calibrationPoint = 29;
		currentStamp = 56;
		transition = 1;
	}
	else if ((dt >= timeStamps[57]) && (dt < timeStamps[58])) { // hold at 29
		calibrationPoint = 29;
		currentStamp = 57;
		transition = 0;
	}
	else if ((dt >= timeStamps[58]) && (dt < timeStamps[59])) { // transition to 30
		calibrationPoint = 30;
		currentStamp = 58;
		transition = 1;
	}
	else if ((dt >= timeStamps[59]) && (dt < timeStamps[60])) { // hold at 30
		calibrationPoint = 30;
		currentStamp = 59;
		transition = 0;
	}
	else if ((dt >= timeStamps[60]) && (dt < timeStamps[61])) { // transition to 31
		calibrationPoint = 31;
		currentStamp = 60;
		transition = 1;
	}
	else if ((dt >= timeStamps[61]) && (dt < timeStamps[62])) { // hold at 31
		calibrationPoint = 31;
		currentStamp = 61;
		transition = 0;
	}
	else if ((dt >= timeStamps[62]) && (dt < timeStamps[63])) { // transition to 32
		calibrationPoint = 32;
		currentStamp = 62;
		transition = 1;
	}
	else if ((dt >= timeStamps[63]) && (dt < timeStamps[64])) { // hold at 32
		calibrationPoint = 32;
		currentStamp = 63;
		transition = 0;
	}
	else if ((dt >= timeStamps[64]) && (dt < timeStamps[65])) { // transition to 33
		calibrationPoint = 33;
		currentStamp = 64;
		transition = 1;
	}
	else if ((dt >= timeStamps[65]) && (dt < timeStamps[66])) { // hold at 33
		calibrationPoint = 33;
		currentStamp = 65;
		transition = 0;
	}
	else if ((dt >= timeStamps[66]) && (dt < timeStamps[67])) { // transition to 34
		calibrationPoint = 34;
		currentStamp = 66;
		transition = 1;
	}
	else if ((dt >= timeStamps[67]) && (dt < timeStamps[68])) { // hold at 34
		calibrationPoint = 34;
		currentStamp = 67;
		transition = 0;
	}
	else if ((dt >= timeStamps[68]) && (dt < timeStamps[69])) { // transition to 35
		calibrationPoint = 35;
		currentStamp = 68;
		transition = 1;
	}
	else if ((dt >= timeStamps[69]) && (dt < timeStamps[70])) { // hold at 35
		calibrationPoint = 35;
		currentStamp = 69;
		transition = 0;
	}
	else if ((dt >= timeStamps[70]) && (dt < timeStamps[71])) { // transition to 36
		calibrationPoint = 36;
		currentStamp = 70;
		transition = 1;
	}
	else if ((dt >= timeStamps[71]) && (dt < timeStamps[72])) { // hold at 36
		calibrationPoint = 36;
		currentStamp = 71;
		transition = 0;
	}
	else {

		if (set[calibrationPoint - 1].set[setnum].point.size() < 30) {
			start += TransitionTime + HoldTime;
			set[calibrationPoint - 1].set[setnum].point.clear();
		}
		else {
			complete = 1; 

		}

	}

	//ensures that at least 30 gazepoints have been observed at each calibration point
	if (calibrationPoint > 1) {

		if (set[calibrationPoint - 2].set[setnum].point.size() < 30) {
			start += TransitionTime + HoldTime;
			set[calibrationPoint - 2].set[setnum].point.clear();
		}
	}


	//determines position of target between calibration points
	if (transition) {
		position.x = transitionP(dt, timeStamps[currentStamp], timeStamps[currentStamp + 1], positions[calibrationPoint - 1].x, positions[calibrationPoint].x);
		position.y = transitionP(dt, timeStamps[currentStamp], timeStamps[currentStamp + 1], positions[calibrationPoint - 1].y, positions[calibrationPoint].y);
		position.z = transitionP(dt, timeStamps[currentStamp], timeStamps[currentStamp + 1], positions[calibrationPoint - 1].z, positions[calibrationPoint].z);
		scale = 6.0;
	}
	else {
		//used on control.cpp to display the target
		position.x = positions[calibrationPoint].x;
		position.y = positions[calibrationPoint].y;
		position.z = positions[calibrationPoint].z;

		
		scale = scaleF(dt, timeStamps[currentStamp], timeStamps[currentStamp + 1]);
	}


}

float DepthCalibration::transitionP(unsigned long dTime, float pTime, float nTime, float pCoord, float nCoord) {

	float ratio = (dTime - pTime) / (nTime - pTime);
	return ((ratio * nCoord) + ((1.0 - ratio) * pCoord));

}

float DepthCalibration::scaleF(unsigned long dTime, float pTime, float nTime) {
	float ratio = (dTime - pTime) / (nTime - pTime);

	return ((ratio * 1.0) + ((1.0 - ratio) * 7.0));
}

void DepthCalibration::Gaze_Data(GLfloat xl, GLfloat yl, GLfloat xr, GLfloat yr, double ct) {
	//stores gazepoint data in set data structure
	Datapoint data;
	if (!((xl == 0) && (yl == 0) && (xr == 0) && (yr == 0))) {
		if (calibrationPoint > 0) {
			if (transition) {
				if (set[calibrationPoint - 1].set[setnum].tranisition.empty()) {
					data.xL = xl;
					data.yL = yl;
					data.xR = xr;
					data.yR = yr;
					data.time = ct - (timeStamps[currentStamp] + start);
					set[calibrationPoint - 1].set[setnum].tranisition.push_front(data);
				}
				else if (!((set[calibrationPoint - 1].set[setnum].tranisition.at(0).xL == xl) && (set[calibrationPoint - 1].set[setnum].tranisition.at(0).yL == yl) && (set[calibrationPoint - 1].set[setnum].tranisition.at(0).xR == xr) && (set[calibrationPoint - 1].set[setnum].tranisition.at(0).yR == yr))) {
					data.xL = xl;
					data.yL = yl;
					data.xR = xr;
					data.yR = yr;
					data.time = ct - (timeStamps[currentStamp] + start);
					set[calibrationPoint - 1].set[setnum].tranisition.push_front(data);
				}
			}
			else {
				if (set[calibrationPoint - 1].set[setnum].point.empty()) {
					data.xL = xl;
					data.yL = yl;
					data.xR = xr;
					data.yR = yr;
					data.time = ct - (timeStamps[currentStamp] + start);
					set[calibrationPoint - 1].set[setnum].point.push_front(data);
				}
				else if (!((set[calibrationPoint - 1].set[setnum].point.at(0).xL == xl) && (set[calibrationPoint - 1].set[setnum].point.at(0).yL == yl) && (set[calibrationPoint - 1].set[setnum].point.at(0).xR == xr) && (set[calibrationPoint - 1].set[setnum].point.at(0).yR == yr))) {
					data.xL = xl;
					data.yL = yl;
					data.xR = xr;
					data.yR = yr;
					data.time = ct - (timeStamps[currentStamp] + start);
					set[calibrationPoint - 1].set[setnum].point.push_front(data);
				}
			}
		}
	}



}

//takes raw data and calculates various fixation estimations using different algorithms
void DepthCalibration::Clean() {
	setc = setnum;
	for (int j = 0; j < 36; j++) {
		positionC = j;
		isoSet.clear();
		for (int q = 0; q < set[j].set[setc].point.size(); q++) { //this fills iso set with datapointss in increasing time order
			isoSet.push_front(set[j].set[setc].point.at(q));

		}

		Average();
		Clean_Average();
		Weighted_Average();
		Median();
		DTI();
	}
}

void DepthCalibration::Average() {

	avglX = 0;
	avglY = 0;
	avgrX = 0;
	avgrY = 0;


	for (int i = 0; i < isoSet.size(); i++) {
		avglX += isoSet.at(i).xL;
		avglY += isoSet.at(i).yL;
		avgrX += isoSet.at(i).xR;
		avgrY += isoSet.at(i).yR;
	}

	avglX = (avglX / isoSet.size());
	avglY = (avglY / isoSet.size());
	avgrX = (avgrX / isoSet.size());
	avgrY = (avgrY / isoSet.size());

	set[positionC].set[setc].m1.xl = avglX;
	set[positionC].set[setc].m1.yl = avglY;
	set[positionC].set[setc].m1.xr = avgrX;
	set[positionC].set[setc].m1.yr = avgrY;

	double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

	set[positionC].set[setc].m1.gx = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters from center screen
	set[positionC].set[setc].m1.gy = t*((avglY + avgrY) / 2.0);
	set[positionC].set[setc].m1.gz = t * (-1.0 * Convergence); //meters towards user from screen

}

//Calculates ans average but removes any outliers
void DepthCalibration::Clean_Average() {
	avglX = 0.0f;
	avglY = 0.0f;
	avgrX = 0.0f;
	avgrY = 0.0f;

	float thresholdX = 0.05; // 5cm
	float thresholdY = 0.05; // 5cm

	std::vector<float> IDTX;
	std::vector<float> IDTY;

	float VarianceX;
	float VarianceY;

	//LEFT EYE
	for (int i = 0; i < isoSet.size(); i++) {
		avglX += isoSet.at(i).xL;
		avglY += isoSet.at(i).yL;
	}
	avglX /= isoSet.size();
	avglY /= isoSet.size();


	for (int i = 0; i < isoSet.size(); i++) {
		VarianceX = isoSet.at(i).xL / avglX;
		VarianceY = isoSet.at(i).yL / avglY;

		if (VarianceX < (1 + thresholdX) && VarianceX >(1 - thresholdX) && VarianceY < (1 + thresholdY) && VarianceY >(1 - thresholdY)) {
			IDTX.push_back(isoSet.at(i).xL);
			IDTY.push_back(isoSet.at(i).yL);
		}
	}

	avglX = 0;
	avglY = 0;

	for (int i = 0; i < IDTX.size(); i++) {
		avglX += IDTX.at(i);
		avglY += IDTY.at(i);
	}

	avglX = avglX / IDTX.size();
	avglY = avglY / IDTY.size();


	//RIGHT EYE
	IDTX.clear();
	IDTY.clear();

	for (int i = 0; i < isoSet.size(); i++) {
		avgrX += isoSet.at(i).xR;
		avgrY += isoSet.at(i).yR;
	}
	avgrX /= isoSet.size();
	avgrY /= isoSet.size();




	for (int i = 0; i < isoSet.size(); i++) {
		VarianceX = isoSet.at(i).xR / avgrX;
		VarianceY = isoSet.at(i).yR / avgrY;

		if (VarianceX < (1 + thresholdX) && VarianceX >(1 - thresholdX) && VarianceY < (1 + thresholdY) && VarianceY >(1 - thresholdY)) {
			IDTX.push_back(isoSet.at(i).xR);
			IDTY.push_back(isoSet.at(i).yR);
		}
	}

	avgrX = 0;
	avgrY = 0;

	for (int i = 0; i < IDTX.size(); i++) {
		avgrX += IDTX.at(i);
		avgrY += IDTY.at(i);
	}

	avgrX = avgrX / IDTX.size();
	avgrY = avgrY / IDTY.size();

	set[positionC].set[setc].m2.xl = avglX;
	set[positionC].set[setc].m2.yl = avglY;
	set[positionC].set[setc].m2.xr = avgrX;
	set[positionC].set[setc].m2.yr = avgrY;

	double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

	set[positionC].set[setc].m2.gx = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
	set[positionC].set[setc].m2.gy = t*((avglY + avgrY) / 2.0);
	set[positionC].set[setc].m2.gz = t * (-1.0 * Convergence); //meters towards user from screen


}

//weighted average puts more important on data collected towards the end of each calibration point
void DepthCalibration::Weighted_Average() {

	avglX = 0;
	avglY = 0;
	avgrX = 0;
	avgrY = 0;

	GLfloat weight = 0;
	GLuint size = isoSet.size();


	for (GLuint i = 1; i <= size; i++) {
		weight += i;
	}


	for (GLuint i = 0; i < size; i++) {
		avglX += isoSet.at(i).xL*(float)(i / weight);
		avglY += isoSet.at(i).yL*(float)(i / weight);
		avgrX += isoSet.at(i).xR*(float)(i / weight);
		avgrY += isoSet.at(i).yR*(float)(i / weight);
	}

	set[positionC].set[setc].m3.xl = avglX;
	set[positionC].set[setc].m3.yl = avglY;
	set[positionC].set[setc].m3.xr = avgrX;
	set[positionC].set[setc].m3.yr = avgrY;

	double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

	set[positionC].set[setc].m3.gx = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
	set[positionC].set[setc].m3.gy = t*((avglY + avgrY) / 2.0);
	set[positionC].set[setc].m3.gz = t * (-1.0 * Convergence); //meters towards user from screen

}

void DepthCalibration::Median() {

	avglX = 0;
	avglY = 0;
	avgrX = 0;
	avgrY = 0;

	if (isoSet.size() > 2) {

		std::vector<GLfloat> meanlX;
		std::vector<GLfloat> meanlY;
		std::vector<GLfloat> meanrX;
		std::vector<GLfloat> meanrY;

		for (GLuint i = 0; i < isoSet.size(); i++) {
			meanlX.push_back(isoSet.at(i).xL);
			meanlY.push_back(isoSet.at(i).yL);
			meanrX.push_back(isoSet.at(i).xR);
			meanrY.push_back(isoSet.at(i).yR);
		}

		std::sort(meanlX.begin(), meanlX.end());
		std::sort(meanlY.begin(), meanlY.end());
		std::sort(meanrX.begin(), meanrX.end());
		std::sort(meanrY.begin(), meanrY.end());


		if (isoSet.size() % 2 == 0) {
			int location = isoSet.size() / 2;
			avglX = (meanlX.at(location) + meanlX.at(location - 1)) / 2;
			avglY = (meanlY.at(location) + meanlY.at(location - 1)) / 2;
			avgrX = (meanrX.at(location) + meanrX.at(location - 1)) / 2;
			avgrY = (meanrY.at(location) + meanrY.at(location - 1)) / 2;
		}
		else {
			int location = isoSet.size() / 2;
			avglX = meanlX.at(location);
			avglY = meanlY.at(location);
			avgrX = meanrX.at(location);
			avgrY = meanrY.at(location);
		}

	}

	set[positionC].set[setc].m4.xl = avglX;
	set[positionC].set[setc].m4.yl = avglY;
	set[positionC].set[setc].m4.xr = avgrX;
	set[positionC].set[setc].m4.yr = avgrY;

	double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

	set[positionC].set[setc].m4.gx = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
	set[positionC].set[setc].m4.gy = t*((avglY + avgrY) / 2.0);
	set[positionC].set[setc].m4.gz = t * (-1.0 * Convergence); //meters towards user from screen


}

void DepthCalibration::DTI() {
	/*Duration-Threshold
	this method stores the average of rouping of point which happen consequetively over a minumum duration and do not vary from one another more than
	a defined threshold.

	Can be modified to calulate the last "fixation" in a raw data set 
	*/

	avglX = 0.0f;
	avglY = 0.0f;
	avgrX = 0.0f;
	avgrY = 0.0f;

	unsigned long durationThreshold = 150;
	float distanceThreshold = 0.07; // 1/50 of screen width
	unsigned long endTime = 0;
	unsigned long beginTime = 0;
	int size;
	int wSize;

	int baseIndex = 0;

	float top = 0.0f;
	float bottom = 0.0f;
	float left = 0.0f;
	float right = 0.0f;
	float distance;

	std::deque<Datapoint> window;

	std::vector<Fixation> rFixation;
	std::vector<Fixation> lFixation;

	//LEFT EYE FIXATIONS

	while ((isoSet.size() - baseIndex) > 1) {
		avglX = 0.0f;
		avglY = 0.0f;
		avgrX = 0.0f;
		avgrY = 0.0f;

		//Initialize window over last points to cover the duration threshold
		size = isoSet.size();

		beginTime = isoSet.at(baseIndex).time;
		window.push_front(isoSet.at(baseIndex));
		do {

			if ((window.size() + baseIndex) >= size) {
				goto finishLeft;
			}
			window.push_front(isoSet.at(baseIndex + window.size()));
			endTime = window.at(0).time;
		} while (((endTime - beginTime) < durationThreshold));


		top = window.at(0).yL;
		bottom = window.at(0).yL;
		left = window.at(0).xL;
		right = window.at(0).xL;

		for (int i = 1; i < window.size(); i++) {
			if (window.at(i).yL > top) {
				top = window.at(i).yL;
			}
			if (window.at(i).yL < bottom) {
				bottom = window.at(i).yL;
			}
			if (window.at(i).xL > right) {
				right = window.at(i).xL;
			}
			if (window.at(i).xL < left) {
				left = window.at(i).xL;
			}
		}


		distance = sqrt((top - bottom)*(top - bottom) + (right - left)*(right - left));

		if (distance < distanceThreshold) {

			while (distance < distanceThreshold && ((baseIndex + window.size()) < isoSet.size())) {
				window.push_front(isoSet.at(baseIndex + window.size()));
				if (window.at(0).yL > top) {
					top = window.at(0).yL;
				}
				if (window.at(0).yL < bottom) {
					bottom = window.at(0).yL;
				}
				if (window.at(0).xL > right) {
					right = window.at(0).xL;
				}
				if (window.at(0).xL < left) {
					left = window.at(0).xL;
				}

				distance = sqrt((top - bottom)*(top - bottom) + (right - left)*(right - left));

			}

			window.pop_front();


			for (int i = 0; i < window.size(); i++) {
				avglY += window.at(i).yL;
				avglX += window.at(i).xL;
			}

			Fixation fix;

			fix.y = avglY / window.size();
			fix.x = avglX /= window.size();

			lFixation.push_back(fix);
			//goto finishLeft; //This will ensure only the most recent fixation is found


		}

		baseIndex++;
		window.clear();
	}

finishLeft:

	//RIGHT EYE

	baseIndex = 0;
	window.clear();

	while ((isoSet.size() - baseIndex)  > 1) {

		avglX = 0.0f;
		avglY = 0.0f;
		avgrX = 0.0f;
		avgrY = 0.0f;

		//Initialize window over last points to cover the duration threshold
		size = isoSet.size();

		beginTime = isoSet.at(baseIndex).time;
		window.push_front(isoSet.at(baseIndex));
		do {

			if ((window.size() + baseIndex) >= size) {

				goto finishRight;
			}
			window.push_front(isoSet.at(baseIndex + window.size()));
			endTime = window.at(0).time;
		} while (((endTime - beginTime) < durationThreshold));


		top = window.at(0).yR;
		bottom = window.at(0).yR;
		left = window.at(0).xR;
		right = window.at(0).xR;

		for (int i = 1; i < window.size(); i++) {
			if (window.at(i).yR > top) {
				top = window.at(i).yR;
			}
			if (window.at(i).yR < bottom) {
				bottom = window.at(i).yR;
			}
			if (window.at(i).xR > right) {
				right = window.at(i).xR;
			}
			if (window.at(i).xR < left) {
				left = window.at(i).xR;
			}
		}


		distance = sqrt((top - bottom)*(top - bottom) + (right - left)*(right - left));

		if (distance < distanceThreshold) {
			while (distance < distanceThreshold && ((baseIndex + window.size()) < isoSet.size())) {
				window.push_front(isoSet.at(baseIndex + window.size()));
				if (window.at(0).yR > top) {
					top = window.at(0).yR;
				}
				if (window.at(0).yR < bottom) {
					bottom = window.at(0).yR;
				}
				if (window.at(0).xR > right) {
					right = window.at(0).xR;
				}
				if (window.at(0).xR < left) {
					left = window.at(0).xR;
				}

				distance = sqrt((top - bottom)*(top - bottom) + (right - left)*(right - left));
			}

			window.pop_front();


			for (int i = 0; i < window.size(); i++) {
				avgrY += window.at(i).yR;
				avgrX += window.at(i).xR;
			}


			Fixation fix;

			fix.y = avgrY / window.size();
			fix.x = avgrX /= window.size();

			rFixation.push_back(fix);
			//goto finishRight; //This will ensure only the most recent fixation is found


		}

		baseIndex++;
		window.clear();
	}

finishRight:




	avglX = 0.0;
	avglY = 0.0;
	avgrX = 0.0;
	avgrY = 0.0;



	if (lFixation.size() != 0 && rFixation.size() != 0) {
		for (int i = 0; i < lFixation.size(); i++) {
			avglX += lFixation.at(i).x;
			avglY += lFixation.at(i).y;
		}

		avglX /= lFixation.size();
		avglY /= lFixation.size();

		for (int i = 0; i < rFixation.size(); i++) {
			avgrX += rFixation.at(i).x;
			avgrY += rFixation.at(i).y;
		}

		avgrX /= rFixation.size();
		avgrY /= rFixation.size();
	}
	else {
		avglX = 0.0;
		avglY = 0.0;
		avgrX = 0.0;
		avgrY = 0.0;
	}


	set[positionC].set[setc].m5.xl = avglX;
	set[positionC].set[setc].m5.yl = avglY;
	set[positionC].set[setc].m5.xr = avgrX;
	set[positionC].set[setc].m5.yr = avgrY;

	double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

	set[positionC].set[setc].m5.gx = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
	set[positionC].set[setc].m5.gy = t*((avglY + avgrY) / 2.0);
	set[positionC].set[setc].m5.gz = t * (-1.0 * Convergence); //meters towards user from screen	
}

//Pritns the estimated 3D gaze position at each calibration points using each fixation finding algorithm
void DepthCalibration::Print_Test() {
	std::string filename = "Depth_Test.txt";

	std::ofstream dataFile;
	dataFile.open(filename);

	dataFile << "Depth Test" << "\n";
	dataFile << "Time per calibration point: " << std::to_string(HoldTime) << " ms" << "\n";
	dataFile << "All values in meters. Origin at screen centre. +Z towards user. +X to the user right. +Y users up" << "\n";
	dataFile << "Plane depths: ( " << planedepths[0]  << ", " << planedepths[1]  << ", " << planedepths[2]  << ", " << planedepths[3]  << " )" << "\n" << "\n";
	


	for (int i = 0; i < 9; i++) {
		dataFile << "\n" << "Position " + std::to_string(i + 1) << "\n" << "\n";

		dataFile << "Depth 1" << "\n";
		dataFile << "3D Position" << "( " << set[i].position.x << " , " << set[i].position.y << " , " << set[i].position.z + 0.92 << " )" << "\n";
		for (int j = 0; j < setnum; j++) {
			dataFile << "Test " << std::to_string(j + 1) << "  ( " << set[i].set[j].m1.gx << " , " << set[j].set[j].m1.gy << " , " << set[j].set[j].m1.gz + 0.92 << " )" << "\n";
		}
		dataFile << "Depth 2" << "\n";
		dataFile << "3D Position" << "( " << set[i + 9].position.x << " , " << set[i + 9].position.y << " , " << set[i + 9].position.z + 0.92 << " )" << "\n";
		for (int j = 0; j < setnum; j++) {
			dataFile << "Test " << std::to_string(j + 1) << "  ( " << set[i + 9].set[j].m1.gx << " , " << set[i + 9].set[j].m1.gy << " , " << set[i + 9].set[j].m1.gz + 0.92 << " )" << "\n";
		}
		dataFile << "Depth 3" << "\n";
		dataFile << "3D Position" << "( " << set[i + 18].position.x << " , " << set[i + 18].position.y << " , " << set[i + 18].position.z + 0.92 << " )" << "\n";
		for (int j = 0; j < setnum; j++) {
			dataFile << "Test " << std::to_string(j + 1) << "  ( " << set[i + 18].set[j].m1.gx << " , " << set[i + 18].set[j].m1.gy << " , " << set[i + 18].set[j].m1.gz + 0.92 << " )" <<  "\n";
		}
		dataFile << "Depth 4" << "\n";
		dataFile << "3D Position" << "( " << set[i + 27].position.x << " , " << set[i + 27].position.y << " , " << set[i + 27].position.z + 0.92 << " )" << "\n";
		for (int j = 0; j < setnum; j++) {
			dataFile << "Test " << std::to_string(j + 1) << " ( " << set[i + 27].set[j].m1.gx << " , " << set[i + 27].set[j].m1.gy << " , " << set[i + 27].set[j].m1.gz + 0.92 << " )" << "\n";
		}


	}
}

//Prints raw data sets to be used inother statistical analysis software as well as a basic key information file about data collected at each calibration point
void DepthCalibration::Print() {

	depth depthSet[36];


	//First calculate gaze points using various methods
	for (int i = 0; i <= setnum; i++) {
		setc = i;
		for (int j = 0; j < 36; j++) {
			positionC = j;

			std::string filename = "Raw_P" + std::to_string(positionC + 1) + "_S" + std::to_string(setc) + ".txt";

			std::ofstream dataFile;
			dataFile.open(filename);

			dataFile << "	xl,	yl,	xr,	yr,	time (ms),	depth (x,y,z)";
			dataFile << "\n";


			for (int q = 0; q < set[positionC].set[setc].point.size(); q++) {
				dataFile << set[positionC].set[setc].point.at(q).xL;
				dataFile << ",";
				dataFile << set[positionC].set[setc].point.at(q).yL;
				dataFile << ",";
				dataFile << set[positionC].set[setc].point.at(q).xR;
				dataFile << ",";
				dataFile << set[positionC].set[setc].point.at(q).yR;
				dataFile << ",";
				dataFile << set[positionC].set[setc].point.at(q).time;
				dataFile << ",";

				double t = -EyeSeparation / (set[positionC].set[setc].point.at(q).xR - set[positionC].set[setc].point.at(q).xL - EyeSeparation);

				dataFile << -(EyeSeparation / 2.0) + (set[positionC].set[setc].point.at(q).xL + (EyeSeparation / 2.0)) * t; //meters form center screen
				dataFile << ",";
				dataFile << t*((set[positionC].set[setc].point.at(q).yL + set[positionC].set[setc].point.at(q).yR) / 2.0);
				dataFile << ",";
				dataFile << t * (-1.0 * Convergence); //meters towards user from screen
				dataFile << "\n";

				if (i == 0) {
					depthSet[j].z.push_front(t * (-1.0 * Convergence)); //meters towards user from screen)
				}
			}
			dataFile.close();

			
		}
	}

	for (int j = 0; j < 36; j++) {
		GLfloat depthAvg = 0;

		for (int i = 0; i < depthSet[j].z.size(); i++) 
			depthAvg += depthSet[j].z.at(i);
		

		depthAvg /= depthSet[j].z.size();


		positionC = j;
		std::string filename = "Key_P" + std::to_string(positionC + 1) + ".txt";
		std::ofstream dataFile;
		dataFile.open(filename);

		//print position
		dataFile << "Key Data Position " << j+1 << "\n" << "\n";

		dataFile << "Screen Coordinates" << "\n";
		dataFile << "xl = " << set[positionC].rCoord.xl << " yl = " << set[positionC].rCoord.yl << " xr = " << set[positionC].rCoord.xr << " yr = " << set[positionC].rCoord.yr << "\n";
		dataFile << "3D Coordinates" << "\n";
		dataFile << "( " << set[positionC].position.x << " , " << set[positionC].position.y << " , " << set[positionC].position.z << " )" << "\n" << "\n";
		dataFile << "Hold Time: " << HoldTime << "ms" << "\n" ;
		dataFile << "Raw data depth average: " << depthAvg << "\n" << "\n";

		dataFile << "Filters" << "\n" << "\n";

		for (int i = 0; i <= setnum; i++) 
			dataFile << "Set " << i << " datapoints: " << set[j].set[i].point.size() << "\n";

		


		dataFile << "Method: Average" << "\n" << "\n";

		for (int i = 0; i <= setnum; i++) {
			dataFile << "Set " << i << "\n";
			dataFile << "Screen Coordinates" << "\n";
			dataFile << "xl = " << set[j].set[i].m1.xl << " yl = " << set[j].set[i].m1.yl << " xr = " << set[j].set[i].m1.xr << " yr = " << set[j].set[i].m1.yr << "\n";
			dataFile << "3D Coordinates" << "\n";
			dataFile << "( " << set[j].set[i].m1.gx << " , " << set[j].set[i].m1.gy << " , " << set[j].set[i].m1.gz << " )" << "\n" << "\n";
			dataFile << "Linear depth adjust: " << ((set[j].set[i].m1.gz)*depth_Slope +depth_Intercept) << " m" << "\n" << "\n";

		}
		dataFile << "Method: Average (Outliers Removed)" << "\n" << "\n";

		for (int i = 0; i <= setnum; i++) {
			dataFile << "Set " << i << "\n";
			dataFile << "Screen Coordinates" << "\n";
			dataFile << "xl = " << set[j].set[i].m2.xl << " yl = " << set[j].set[i].m2.yl << " xr = " << set[j].set[i].m2.xr << " yr = " << set[j].set[i].m2.yr << "\n";
			dataFile << "3D Coordinates" << "\n";
			dataFile << "( " << set[j].set[i].m2.gx << " , " << set[j].set[i].m2.gy << " , " << set[j].set[i].m2.gz << " )" << "\n" << "\n";
		}
		dataFile << "Method: Average (Weighted)" << "\n" << "\n";

		for (int i = 0; i <= setnum; i++) {
			dataFile << "Set " << i << "\n";
			dataFile << "Screen Coordinates" << "\n";
			dataFile << "xl = " << set[j].set[i].m3.xl << " yl = " << set[j].set[i].m3.yl << " xr = " << set[j].set[i].m3.xr << " yr = " << set[j].set[i].m3.yr << "\n";
			dataFile << "3D Coordinates" << "\n";
			dataFile << "( " << set[j].set[i].m3.gx << " , " << set[j].set[i].m3.gy << " , " << set[j].set[i].m3.gz << " )" << "\n" << "\n";
		}
		dataFile << "Method: Median" << "\n" << "\n";

		for (int i = 0; i <= setnum; i++) {
			dataFile << "Set " << i << "\n";
			dataFile << "Screen Coordinates" << "\n";
			dataFile << "xl = " << set[j].set[i].m4.xl << " yl = " << set[j].set[i].m4.yl << " xr = " << set[j].set[i].m4.xr << " yr = " << set[j].set[i].m4.yr << "\n";
			dataFile << "3D Coordinates" << "\n";
			dataFile << "( " << set[j].set[i].m4.gx << " , " << set[j].set[i].m4.gy << " , " << set[j].set[i].m4.gz << " )" << "\n" << "\n";
		}
		dataFile << "Method: DTI" << "\n" << "\n";

		for (int i = 0; i <= setnum; i++) {
			dataFile << "Set " << i << "\n";
			dataFile << "Screen Coordinates" << "\n";
			dataFile << "xl = " << set[j].set[i].m5.xl << " yl = " << set[j].set[i].m5.yl << " xr = " << set[j].set[i].m5.xr << " yr = " << set[j].set[i].m5.yr << "\n";
			dataFile << "3D Coordinates" << "\n";
			dataFile << "( " << set[j].set[i].m5.gx << " , " << set[j].set[i].m5.gy << " , " << set[j].set[i].m5.gz << " )" << "\n" << "\n";
		}

	}

}

//Creates a linear relationship between calculate z values and the actual z values
void DepthCalibration::Linear_Fit() {
	std::deque<GLfloat> Averages;

	GLfloat avg = 0.0f;

	for (int i = 0; i < 4; i++) {
		avg = 0.0f;
		for (int j = 0; j < 9; j++) {
			avg += set[j + i * 9].set[0].m1.gz;
		}
		avg = avg / 9.0;

		Averages.push_back(avg);
	}



	//Left Eye
	double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
	for (int i = 0; i < 4; i++) {
		sumX += Averages.at(i);
		sumY += planedepths[i] - 0.92;
		sumXY += Averages.at(i) * (planedepths[i] - 0.92);
		sumX2 += Averages.at(i) * Averages.at(i);
	}

	double xMean = sumX / 3.0;
	double yMean = sumY / 3.0;

	double denominator = sumX2 - sumX * xMean;
	depth_Slope = (sumXY - sumX * yMean) / denominator;
	depth_Intercept = yMean - depth_Slope * xMean;

}