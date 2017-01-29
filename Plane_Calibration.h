#pragma once
#include <deque>


struct Datapoint {
	float xL; // left eye x
	float yL; // left eye y 
	float xR; // right eye x
	float yR; // right eye y
	unsigned long time; //time since the beginning of current calibration point
};

struct ScreenCoord {
	float xl;
	float yl;
	float xr;
	float yr;
};

struct Fixation {
	float x;
	float y;
};

struct Filtered {
	float xl;
	float yl;
	float xr;
	float yr;
	float gx;
	float gy;
	float gz;
};

struct VisualAngle {
	double xl;
	double yl;
	double xr;
	double yr;
};

struct PointSet {
	std::deque<Datapoint> point; //point holds the eyetrack data over the course of calibration over a point
	std::deque<Datapoint> tranisition; //tranisition holds the eytrack data when transitioning to a point
	Filtered m1; //average
	Filtered m2; //clean average
	Filtered m3; //weighted average
	Filtered m4; //median
	Filtered m5; //dti
	VisualAngle angle;
	VisualAngle angleError;
};

struct CalibrationPoints {
	ScreenCoord rCoord; // real coordinates shown on screen
	VisualAngle rAngle; // angle between left and rigth eye vectors
	PointSet set[4]; //holds data sets for each 3d calibration run through
	glm::vec3 position; //the 3d position of the calibration point
};

class PlaneCalibration
{
public:
	PlaneCalibration(double start, GLuint setnum);
	~PlaneCalibration();
	void Update(double ct);
	void Gaze_Data(GLfloat xl, GLfloat yl, GLfloat xr, GLfloat yr, double ct);
	void Clean();
	void Linear_Fit();

	glm::vec3 positions[10]; //[psotopn of each clibration point (position[0] is initial position)
	
	double timeStamps[19];
	const double TransitionTime = 700; //time betwene calibration points
	const double HoldTime = 1500; //time at each calibration point
	double start; //plane claibration begin time

	GLuint setnum = 0; 

	GLfloat scale;
	GLfloat scaleFactor = 6.0;
	glm::vec3 position;
	GLuint calibrationPoint;
	bool transition;
	GLuint currentStamp;

	CalibrationPoints set[9];

	bool complete = 0;

	//Linear fit values
	GLfloat xl_Slope = 0.0f;
	GLfloat xr_Slope = 0.0f;
	GLfloat xl_Intercept = 0.0f;
	GLfloat xr_Intercept = 0.0f;

	GLfloat yl_Slope = 0.0f;
	GLfloat yr_Slope = 0.0f;
	GLfloat yl_Intercept = 0.0f;
	GLfloat yr_Intercept = 0.0f;



private:
	float PlaneCalibration::transitionP(unsigned long dTime, float pTime, float nTime, float pCoord, float nCoord);
	float PlaneCalibration::scaleF(unsigned long dTime, float pTime, float nTime);

	void Average();
};

PlaneCalibration::PlaneCalibration(double start, GLuint setnum)
{
	int positionCount = 0;

	this->setnum = setnum;


	//2D position set defined by percentage from center opf screen
	positions[positionCount++] = glm::vec3(-1.2 * (ScreenWidth / 2.0), 0.8 * (ScreenHeight / 2.0), -1.0 * Convergence); // off screen start location

	positions[positionCount++] = glm::vec3(-0.6 * (ScreenWidth / 2.0), 0.8 * (ScreenHeight / 2.0), -1.0 * Convergence);
	positions[positionCount++] = glm::vec3(0.0, 0.8 * (ScreenHeight / 2), -1.0 * Convergence);
	positions[positionCount++] = glm::vec3(0.6 * (ScreenWidth / 2.0), 0.8 * (ScreenHeight / 2.0), -1.0 * Convergence);
	positions[positionCount++] = glm::vec3(0.6 * (ScreenWidth / 2.0), 0.0, -1.0 * Convergence);
	positions[positionCount++] = glm::vec3(0.0, 0.0, -1.0 * Convergence);
	positions[positionCount++] = glm::vec3(-0.6 * (ScreenWidth / 2.0), 0.0, -1.0 * Convergence);
	positions[positionCount++] = glm::vec3(-0.6 * (ScreenWidth / 2.0), -0.8 * (ScreenHeight / 2.0), -1.0 * Convergence);
	positions[positionCount++] = glm::vec3(0.0, -0.8 * (ScreenHeight / 2.0), -1.0 * Convergence);
	positions[positionCount++] = glm::vec3(0.6 * (ScreenWidth / 2.0), -0.8 * (ScreenHeight / 2.0), -1.0 * Convergence);

	for (int i = 1; i < 10; i++) {
		set[i-1].rCoord.xl = positions[i].x;
		set[i-1].rCoord.xr = positions[i].x;
		set[i-1].rCoord.yl = positions[i].y;
		set[i-1].rCoord.yr = positions[i].y;
	}

	for (int i = 0; i < 9; i++) {
		set[i].rAngle.xl = glm::degrees(atan(abs((set[i].rCoord.xl + (EyeSeparation / 2.0))) / Convergence));
		set[i].rAngle.yl = glm::degrees(atan(abs(set[i].rCoord.yl) / Convergence));
		set[i].rAngle.xr = glm::degrees(atan(abs((set[i].rCoord.xr - (EyeSeparation / 2.0))) / Convergence));
		set[i].rAngle.yr = glm::degrees(atan(abs(set[i].rCoord.yr) / Convergence));
	}

	this->start = start;
	int stampCount = 1;
	timeStamps[0] = 8000;
	
	//setting time stamp values for calibration
	for (int i = 0; i < 9; i++) {
		timeStamps[stampCount] = timeStamps[stampCount - 1] + TransitionTime;
		stampCount++;
		timeStamps[stampCount] = timeStamps[stampCount - 1] + HoldTime;
		stampCount++;
	}
	
}

PlaneCalibration::~PlaneCalibration()
{
}

//based on the current time we evaluate the position and size of the target
void PlaneCalibration::Update(double ct) {
	
	double dt = ct - start;


	if (dt < timeStamps[0]) {
		calibrationPoint = 0;
		currentStamp = 0;
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
	else {
		
		if (set[calibrationPoint-1].set[setnum].point.size() < 10) {
			start += TransitionTime + HoldTime;
			set[calibrationPoint-1].set[setnum].point.clear();
		}
		else {

			complete = 1;
		}
		
	}
	
	
	//ensures that at least 10 gaze points have been observed at each calibration point
	if (calibrationPoint > 1) {
		
		if (set[calibrationPoint - 2].set[setnum].point.size() < 10) {
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

float PlaneCalibration::transitionP(unsigned long dTime, float pTime, float nTime, float pCoord, float nCoord) {

	float ratio = (dTime - pTime) / (nTime - pTime);
	return ((ratio * nCoord) + ((1.0 - ratio) * pCoord));

}

float PlaneCalibration::scaleF(unsigned long dTime, float pTime, float nTime) {
	float ratio = (dTime - pTime) / (nTime - pTime);

	return ((ratio * 1.0) + ((1.0 - ratio) * 7.0));
}

void PlaneCalibration::Gaze_Data(GLfloat xl, GLfloat yl, GLfloat xr, GLfloat yr, double ct) {
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

void PlaneCalibration::Clean() {
	Average();
}

void PlaneCalibration::Average() {
	

	for( GLuint position = 0; position < 9;  position++)
	{
		float avglX = 0.0f;
		float avglY = 0.0f;
		float avgrX = 0.0f;
		float avgrY = 0.0f;

		//LEFT EYE
		for (int i = 0; i < set[position].set[setnum].point.size(); i++) {
			avglX += set[position].set[setnum].point.at(i).xL;
			avglY += set[position].set[setnum].point.at(i).yL;
		}
		set[position].set[setnum].m1.xl = avglX / set[position].set[setnum].point.size();
		set[position].set[setnum].m1.yl = avglY / set[position].set[setnum].point.size();



		//RIGHT EYE
		for (int i = 0; i < set[position].set[setnum].point.size(); i++) {
			avgrX += set[position].set[setnum].point.at(i).xR;
			avgrY += set[position].set[setnum].point.at(i).yR;
		}
		set[position].set[setnum].m1.xr = avgrX / set[position].set[setnum].point.size();
		set[position].set[setnum].m1.yr = avgrY / set[position].set[setnum].point.size();
	}



	//Calculate Visual Angle Error
	for (GLuint position = 0; position < 9; position++)
	{
		set[position].set[setnum].angle.xl = glm::degrees(atan(abs((set[position].set[setnum].m1.xl + (EyeSeparation / 2.0))) / Convergence));
		set[position].set[setnum].angle.yl = glm::degrees(atan(abs(set[position].set[setnum].m1.yl) / Convergence));
		set[position].set[setnum].angle.xr = glm::degrees(atan(abs((set[position].set[setnum].m1.xr - (EyeSeparation / 2.0))) / Convergence));
		set[position].set[setnum].angle.yr = glm::degrees(atan(abs(set[position].set[setnum].m1.yr) / Convergence));

		set[position].set[setnum].angleError.xl = abs(set[position].set[setnum].angle.xl- set[position].rAngle.xl);
		set[position].set[setnum].angleError.yl = abs(set[position].set[setnum].angle.yl - set[position].rAngle.yl);
		set[position].set[setnum].angleError.xr = abs(set[position].set[setnum].angle.xr - set[position].rAngle.xr);
		set[position].set[setnum].angleError.yr = abs(set[position].set[setnum].angle.yr - set[position].rAngle.yr);
	}
	

	int q = 5;
}

void PlaneCalibration::Linear_Fit() {
	//fit linear line to real x and y and observed x and y for both eyes individually
	

	std::deque<GLfloat> leftAverages;
	std::deque<GLfloat> rightAverages;

	std::deque<GLfloat> leftAveragesY;
	std::deque<GLfloat> rightAveragesY;

	
	GLfloat avgXL = 0.0f;
	GLfloat avgXR = 0.0f;
	
	avgXL += set[0].set[0].m1.xl;
	avgXR += set[0].set[0].m1.xr;
	avgXL += set[5].set[0].m1.xl;
	avgXR += set[5].set[0].m1.xr;
	avgXL += set[6].set[0].m1.xl;
	avgXR += set[6].set[0].m1.xr;
	
	avgXL = avgXL / 3.0;
	avgXR = avgXR / 3.0;

	leftAverages.push_back(avgXL);
	rightAverages.push_back(avgXR);
	
	avgXL = 0.0f;
	avgXR = 0.0f;

	avgXL += set[1].set[0].m1.xl;
	avgXR += set[1].set[0].m1.xr;
	avgXL += set[4].set[0].m1.xl;
	avgXR += set[4].set[0].m1.xr;
	avgXL += set[7].set[0].m1.xl;
	avgXR += set[7].set[0].m1.xr;

	avgXL = avgXL / 3.0;
	avgXR = avgXR / 3.0;

	leftAverages.push_back(avgXL);
	rightAverages.push_back(avgXR);

	avgXL = 0.0f;
	avgXR = 0.0f;

	avgXL += set[2].set[0].m1.xl;
	avgXR += set[2].set[0].m1.xr;
	avgXL += set[3].set[0].m1.xl;
	avgXR += set[3].set[0].m1.xr;
	avgXL += set[8].set[0].m1.xl;
	avgXR += set[8].set[0].m1.xr;

	avgXL = avgXL / 3.0;
	avgXR = avgXR / 3.0;

	leftAverages.push_back(avgXL);
	rightAverages.push_back(avgXR);

	
	//Left Eye
	double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
	for (int i = 0; i<3; i++) {
		sumX += leftAverages.at(i);
		sumY += set[i].rCoord.xl;
		sumXY += leftAverages.at(i) * set[i].rCoord.xl;
		sumX2 += leftAverages.at(i) * leftAverages.at(i);
	}

	double xMean = sumX / 3.0;
	double yMean = sumY / 3.0;

	double denominator = sumX2 - sumX * xMean;
	xl_Slope = (sumXY - sumX * yMean) / denominator;
	xl_Intercept = yMean - xl_Slope * xMean;

	//Right Eye
	sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
	for (int i = 0; i<3; i++) {
		sumX += rightAverages.at(i);
		sumY += set[i].rCoord.xr;
		sumXY += rightAverages.at(i) * set[i].rCoord.xr;
		sumX2 += rightAverages.at(i) * rightAverages.at(i);
	}

	xMean = sumX / 3.0;
	yMean = sumY / 3.0;

	denominator = sumX2 - sumX * xMean;
	xr_Slope = (sumXY - sumX * yMean) / denominator;
	xr_Intercept = yMean - xr_Slope * xMean;

	
	GLfloat avgYL = 0.0f;
	GLfloat avgYR = 0.0f;

	avgYL += set[0].set[0].m1.yl;
	avgYR += set[0].set[0].m1.yr;
	avgYL += set[1].set[0].m1.yl;
	avgYR += set[1].set[0].m1.yr;
	avgYL += set[2].set[0].m1.yl;
	avgYR += set[2].set[0].m1.yr;

	avgYL = avgYL / 3.0;
	avgYR = avgYR / 3.0;

	leftAveragesY.push_back(avgYL);
	rightAveragesY.push_back(avgYR);

	avgYL = 0.0f;
	avgYR = 0.0f;

	avgYL += set[3].set[0].m1.yl;
	avgYR += set[3].set[0].m1.yr;
	avgYL += set[4].set[0].m1.yl;
	avgYR += set[4].set[0].m1.yr;
	avgYL += set[5].set[0].m1.yl;
	avgYR += set[5].set[0].m1.yr;

	avgYL = avgYL / 3.0;
	avgYR = avgYR / 3.0;

	leftAveragesY.push_back(avgYL);
	rightAveragesY.push_back(avgYR);

	avgYL = 0.0f;
	avgYR = 0.0f;

	avgYL += set[6].set[0].m1.yl;
	avgYR += set[6].set[0].m1.yr;
	avgYL += set[7].set[0].m1.yl;
	avgYR += set[7].set[0].m1.yr;
	avgYL += set[8].set[0].m1.yl;
	avgYR += set[8].set[0].m1.yr;

	avgYL = avgYL / 3.0;
	avgYR = avgYR / 3.0;

	leftAveragesY.push_back(avgYL);
	rightAveragesY.push_back(avgYR);


	//Left Eye
	sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
	for (int i = 0; i<3; i++) {
		sumX += leftAveragesY.at(i);
		sumY += set[i*3].rCoord.yl;
		sumXY += leftAveragesY.at(i) * set[i*3].rCoord.yl;
		sumX2 += leftAveragesY.at(i) * leftAveragesY.at(i);
	}

	xMean = sumX / 3.0;
	yMean = sumY / 3.0;

	denominator = sumX2 - sumX * xMean;
	yl_Slope = (sumXY - sumX * yMean) / denominator;
	yl_Intercept = yMean - yl_Slope * xMean;

	//Right Eye
	sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
	for (int i = 0; i<3; i++) {
		sumX += rightAveragesY.at(i);
		sumY += set[i*3].rCoord.yr;
		sumXY += rightAveragesY.at(i) * set[i*3].rCoord.yr;
		sumX2 += rightAveragesY.at(i) * rightAveragesY.at(i);
	}

	xMean = sumX / 3.0;
	yMean = sumY / 3.0;

	denominator = sumX2 - sumX * xMean;
	yr_Slope = (sumXY - sumX * yMean) / denominator;
	yr_Intercept = yMean - yr_Slope * xMean;

}