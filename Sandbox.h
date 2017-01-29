#pragma once
#include <random>
#include <deque>
#include <time.h>
#include <math.h> 
#include <algorithm>


class Sandbox
{
public:
	Sandbox(double ct);
	~Sandbox();

	void Update(double ct);
	void Gaze_Data(GLfloat xl, GLfloat yl, GLfloat xr, GLfloat yr, double ct);
	void Depth();
	void Average();
	void Clean_Average();
	void Weighted_Average();
	void Median();
	void DTI();

	GLfloat scale = 6.0f;
	glm::vec3 position[5];
	GLuint positionCount = 5;


	glm::vec3 staticPos[9];

	glm::vec3 gazepos; //3d gaze point 
	
	//dictates which type of fit is used to calculate 3d gaze position
	GLboolean NoFit = 0;
	GLboolean LinFit = 0; 
	GLboolean LinR = 0; 
	GLboolean LinR2 = 0;


	GLboolean PlaneComplete = 0; //indicates that the 2d calibration has been completed
	GLboolean DepthComplete = 0; //indicates that the 3d calibration has been completed

	double R1[3]; //holds coefficients from first regression
	double R2x[3]; //holds coeffients from second regression on x
	double R2y[3]; //holds coeffients from second regression on y
	double R2z[3]; //holds coeffients from second regression on z



	//Linear fit values passded in from 2d claibration
	GLfloat xl_Slope = 0.0f;
	GLfloat xr_Slope = 0.0f;
	GLfloat xl_Intercept = 0.0f;
	GLfloat xr_Intercept = 0.0f;

	GLfloat yl_Slope = 0.0f;
	GLfloat yr_Slope = 0.0f;
	GLfloat yl_Intercept = 0.0f;
	GLfloat yr_Intercept = 0.0f;

	GLfloat depth = 0.0f;

	glm::vec3 LEScreen;
	glm::vec3 REScreen;

	double pt = 0.0; //previous time
	double dt = 0.0; //delta time
	double et = 0.0; //elapsed time

	std::deque<Datapoint> set;


	GLfloat avglX;
	GLfloat avglY;
	GLfloat avgrX;
	GLfloat avgrY;
	

};

Sandbox::Sandbox(double ct)
{
	GLfloat time = clock() / (CLOCKS_PER_SEC / 1000);
	GLuint offset = 12520;


	//sets initial positions of targets (randomly with dictated bounds)
	for (GLuint i = 0; i < positionCount; i++) {
		position[i].x = -0.7 * (ScreenWidth / 2.0) + fmod(time + offset, 0.7 * ScreenWidth);
		position[i].y = -0.7 * (ScreenHeight / 2.0) + fmod(time + offset, 0.7 * ScreenHeight);
		position[i].z = -1.62 + fmod(time + offset, 0.8);
		offset += offset;
	}
	gazepos = glm::vec3(0.0, 0.0, 0.0);


	float thetaW = atan(((ScreenWidth / 2.0) + (EyeSeparation / 2.0)) / Convergence);
	float thetaH = atan(((ScreenHeight / 2.0)) / Convergence);


	float planedepths[9] = { //plane depths in respect to convergence plane
		0.0f, -0.10f, -0.20f, -0.30f, -0.40f, -0.50f, -0.60f, -0.70f, -0.80f
	};

	staticPos[0] = glm::vec3(-0.6 * ((Convergence - planedepths[0]) * tan(thetaW) - (EyeSeparation / 2.0)), 0.6 * ((Convergence - planedepths[0]) * tan(thetaH)), -1.0 *(Convergence - planedepths[0]));
	staticPos[1] = glm::vec3(0.0f, 0.6 * ((Convergence - planedepths[1]) * tan(thetaH)), -1.0 * (Convergence - planedepths[1]));
	staticPos[2] = glm::vec3(0.6 * ((Convergence - planedepths[2]) * tan(thetaW) - (EyeSeparation / 2.0)), 0.6 * ((Convergence - planedepths[2]) * tan(thetaH)), -1.0 * (Convergence - planedepths[2]));

	staticPos[3] = glm::vec3(0.6 * ((Convergence - planedepths[3]) * tan(thetaW) - (EyeSeparation / 2.0)), 0.0f, -1.0 * (Convergence - planedepths[3]));
	staticPos[4] = glm::vec3(0.0f, 0.0f, -1.0 * (Convergence - planedepths[4]));
	staticPos[5] = glm::vec3(-0.6 * ((Convergence - planedepths[5]) * tan(thetaW) - (EyeSeparation / 2.0)), 0.0f, -1.0 * (Convergence - planedepths[5]));

	staticPos[6] = glm::vec3(-0.6 * ((Convergence - planedepths[6]) * tan(thetaW) - (EyeSeparation / 2.0)), -0.6 * ((Convergence - planedepths[6]) * tan(thetaH)), -1.0 * (Convergence - planedepths[6]));
	staticPos[7] = glm::vec3(0.0f, -0.6 * ((Convergence - planedepths[7]) * tan(thetaH)), -1.0 * (Convergence - planedepths[7]));
	staticPos[8] = glm::vec3(0.6 * ((Convergence - planedepths[8]) * tan(thetaW) - (EyeSeparation / 2.0)), -0.6 * ((Convergence - planedepths[8]) * tan(thetaH)), -1.0 * (Convergence - planedepths[8]));




	LEScreen = glm::vec3(0.0, 0.0, 0.0);
	REScreen = glm::vec3(0.0, 0.0, 0.0);
	pt = ct;
}

Sandbox::~Sandbox()
{
}

void Sandbox::Update(double ct) {
	dt = ct - pt;
	pt = ct;

	et += dt;

	GLfloat time = clock() / (CLOCKS_PER_SEC / 1000);
	GLuint offset = 12520;

	/*checks if 3d gaze point is within a certain threshold of a target
	if so the targets position is recalculated */
	for (int i = 0; i < positionCount; i++) {
		
		if ((abs(position[i].x - gazepos.x) < 0.02) && (abs(position[i].y - gazepos.y) < 0.02) && (abs(position[i].z - gazepos.z) < 0.10))
		{
			position[i].x = -0.7 * (ScreenWidth / 2.0) + fmod(time + offset, 0.7 * ScreenWidth);
			position[i].y = -0.7 * (ScreenHeight / 2.0) + fmod(time + offset, 0.7 * ScreenHeight);
			position[i].z = -1.62 + fmod(time + offset, 0.8);
			GLuint offset = 12520;
		}
	}
	
	
	//Select which fixation algortihm to calculate gaze depth with (preferable to be the same ones used when generating lin fit and linear regression)
	//Median();
	//Clean_Average();
	//Weighted_Average();
	//Average();
	DTI();
	Depth();
	

}

void Sandbox::Depth() {

	//Loation between eyes
	double xH = 0.0f;
	double yH = 0.0f;
	double zH = 0.92f;

	//LEScreen = glm::vec3(avglX*xl_Slope + xl_Intercept, (avglY + avgrY) / 2.0, -0.92);
	//REScreen = glm::vec3(avgrX*xr_Slope + xr_Intercept, (avglY + avgrY) / 2.0, -0.92);

	//LEScreen = glm::vec3(avglX, (avglY+ avgrY)/2.0, -0.92);
	//REScreen = glm::vec3(avgrX, (avglY + avgrY) / 2.0, -0.92);


	//Calculates 3d gaze point base don selected calibration methods
	if (LinFit) {
		avglX = avglX*xl_Slope + xl_Intercept;
		avgrX = avgrX*xr_Slope + xr_Intercept;
		avglY = avglY*yl_Slope + yl_Intercept;
		avgrY = avgrY*yr_Slope + yr_Intercept;

		double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

		gazepos.x = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
		gazepos.y = t*((avglY + avgrY) / 2.0);
		gazepos.z = t * (-1.0 * Convergence); //meters towards user from screen
	}
	else {
		double t = -EyeSeparation / (avgrX - avglX - EyeSeparation);

		gazepos.x = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
		gazepos.y = t*((avglY + avgrY) / 2.0);
		gazepos.z = t * (-1.0 * Convergence); //meters towards user from screen

		if (LinR) {
			gazepos.z = R1[0] * ((avglX + avgrX) / 2.0) + R1[1] * ((avglY + avgrY) / 2.0) + R1[2] * (avgrX - avglX);
			t = gazepos.z / (-1.0 * Convergence);
			gazepos.x = -(EyeSeparation / 2.0) + (avglX + (EyeSeparation / 2.0)) * t; //meters form center screen
			gazepos.y = t*((avglY + avgrY) / 2.0);

		}
		else if (LinR2) {
			gazepos.x = R2x[0] * gazepos.x + R2x[1] * gazepos.y + R2x[2] * gazepos.z;
			gazepos.y = R2y[0] * gazepos.x + R2y[1] * gazepos.y + R2y[2] * gazepos.z;
			gazepos.z = R2z[0] * gazepos.x + R2z[1] * gazepos.y + R2z[2] * gazepos.z;

		}

	}

	//Can be used to create bounds on the location of the 3d gaze point

	/*
	if (gazepos.z < -1.91) {
		gazepos.z = -1.91;
	}

	if (gazepos.x < -(ScreenWidth / 2.0)) {
		gazepos.x = -(ScreenWidth / 2.0) + 0.01;
	}

	if (gazepos.x >(ScreenWidth / 2.0)) {
		gazepos.x = (ScreenWidth / 2.0) - 0.01;
	}

	if (gazepos.y < -(ScreenHeight / 2.0)) {
		gazepos.y = -(ScreenHeight / 2.0) + 0.01;
	}

	if (gazepos.y >(ScreenHeight / 2.0)) {
		gazepos.y = (ScreenHeight / 2.0) - 0.01;
	}
	*/

}

void Sandbox::Gaze_Data(GLfloat xl, GLfloat yl, GLfloat xr, GLfloat yr, double ct) {
	//Stores most recent gazepoint
	Datapoint data;
	if (!((xl == 0) && (yl == 0) && (xr == 0) && (yr == 0))) {

				if (set.empty()) {
					data.xL = xl;
					data.yL = yl;
					data.xR = xr;
					data.yR = yr;
					data.time = ct;
					set.push_back(data);
				}
				else if (!((set.at(0).xL == xl) && (set.at(0).yL == yl) && (set.at(0).xR == xr) && (set.at(0).yR == yr))) {
					data.xL = xl;
					data.yL = yl;
					data.xR = xr;
					data.yR = yr;
					data.time = ct;
					set.push_back(data);
				}
	}

	bool c = 1;

	if (set.size() > 2) {
		//ensure sthat only points observed in the last n ms are used for our 3d calculation
		while (((set.at(set.size() - 1).time - set.at(0).time) > 1500) && c)
		{
			set.pop_front();

			if (set.size() < 2)
			{
				c = 0;
			}
		}
	}
	


}

void Sandbox::Clean_Average() {
	float avglX = 0.0f;
	float avglY = 0.0f;
	float avgrX = 0.0f;
	float avgrY = 0.0f;

	float thresholdX = 0.100;
	float thresholdY = 0.100;

	std::vector<float> IDTX;
	std::vector<float> IDTY;

	float VarianceX;
	float VarianceY;


	//LEFT EYE
	for (int i = 0; i < set.size(); i++) {
		avglX += set.at(i).xL;
		avglY += set.at(i).yL;
	}
	avglX /= set.size();
	avglY /= set.size();


	for (int i = 0; i < set.size(); i++) {
		VarianceX = set.at(i).xL / avglX;
		VarianceY = set.at(i).yL / avglY;

		if (VarianceX < (1 + thresholdX) && VarianceX >(1 - thresholdX) && VarianceY < (1 + thresholdY) && VarianceY >(1 - thresholdY)) {
			IDTX.push_back(set.at(i).xL);
			IDTY.push_back(set.at(i).yL);
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

	for (int i = 0; i < set.size(); i++) {
		avgrX += set.at(i).xR;
		avgrY += set.at(i).yR;
	}
	avgrX /= set.size();
	avgrY /= set.size();




	for (int i = 0; i < set.size(); i++) {
		VarianceX = set.at(i).xR / avgrX;
		VarianceY = set.at(i).yR / avgrY;

		if (VarianceX < (1 + thresholdX) && VarianceX >(1 - thresholdX) && VarianceY < (1 + thresholdY) && VarianceY >(1 - thresholdY)) {
			IDTX.push_back(set.at(i).xR);
			IDTY.push_back(set.at(i).yR);
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


	if (set.size() > 30) {
		int q = 3;
	}
}

void Sandbox::Average() {

	avglX = 0;
	avglY = 0;
	avgrX = 0;
	avgrY = 0;

	for (int i = 0; i < set.size(); i++) {
		avglX += set.at(i).xL;
		avglY += set.at(i).yL;
		avgrX += set.at(i).xR;
		avgrY += set.at(i).yR;
	}

	avglX = avglX / set.size();
	avglY = avglY / set.size();
	avgrX = avgrX / set.size();
	avgrY = avgrY / set.size();
}

void Sandbox::Median() {

	avglX = 0;
	avglY = 0;
	avgrX = 0;
	avgrY = 0;

	unsigned long endTime = 0;

	std::deque<Datapoint> scaledSet;

	if (set.size() > 2) {
		endTime = set.at(set.size() - 1).time;

		for (int i = 0; i < set.size(); i++) {
			if ((endTime - set.at(i).time) < 1000) {
				scaledSet.push_front(set.at(i));
			}
		}
	}

	if (scaledSet.size() > 2) {

		std::vector<GLfloat> meanlX;
		std::vector<GLfloat> meanlY;
		std::vector<GLfloat> meanrX;
		std::vector<GLfloat> meanrY;

		for (GLuint i = 0; i < scaledSet.size(); i++) {
			meanlX.push_back(scaledSet.at(i).xL);
			meanlY.push_back(scaledSet.at(i).yL);
			meanrX.push_back(scaledSet.at(i).xR);
			meanrY.push_back(scaledSet.at(i).yR);
		}

		std::sort(meanlX.begin(), meanlX.end());
		std::sort(meanlY.begin(), meanlY.end());
		std::sort(meanrX.begin(), meanrX.end());
		std::sort(meanrY.begin(), meanrY.end());


		if (scaledSet.size() % 2 == 0) {
			int location = scaledSet.size() / 2;
			avglX = (meanlX.at(location) + meanlX.at(location - 1)) / 2;
			avglY = (meanlY.at(location) + meanlY.at(location - 1)) / 2;
			avgrX = (meanrX.at(location) + meanrX.at(location - 1)) / 2;
			avgrY = (meanrY.at(location) + meanrY.at(location - 1)) / 2;
		}
		else {
			int location = scaledSet.size() / 2;
			avglX = meanlX.at(location);
			avglY = meanlY.at(location);
			avgrX = meanrX.at(location);
			avgrY = meanrY.at(location);
		}

	}


}

void Sandbox::Weighted_Average() {

	avglX = 0;
	avglY = 0;
	avgrX = 0;
	avgrY = 0;

	unsigned long endTime = 0;

	std::deque<Datapoint> scaledSet;

	if (set.size() > 2) {
		endTime = set.at(set.size() - 1).time;

		for (int i = 0; i < set.size(); i++) {
			if ((endTime - set.at(i).time) < 400) {
				scaledSet.push_front(set.at(i));
			}
		}
	}

	GLfloat weight = 0;
	GLuint size = scaledSet.size();

	for (GLuint i = 1; i <= size; i++)
		weight += i;

	for (GLuint i = 0; i < size; i++) {
		avglX += scaledSet.at(i).xL*(float)((size - i) / weight);
		avglY += scaledSet.at(i).yL*(float)((size - i) / weight);
		avgrX += scaledSet.at(i).xR*(float)((size - i) / weight);
		avgrY += scaledSet.at(i).yR*(float)((size - i) / weight);
	}



}

void Sandbox::DTI() {
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

	while ((set.size() - baseIndex) > 1) {

		avglX = 0.0f;
		avglY = 0.0f;
		avgrX = 0.0f;
		avgrY = 0.0f;

		//Initialize window over last points to cover the duration threshold
		size = set.size();

		beginTime = set.at(baseIndex).time;
		window.push_front(set.at(baseIndex));
		do {

			if ((window.size() + baseIndex) >= size) {
				goto finishLeft;
			}
			window.push_front(set.at(baseIndex + window.size()));
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

			while (distance < distanceThreshold && ((baseIndex + window.size()) < set.size())) {
				window.push_front(set.at(baseIndex + window.size()));
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

	while ((set.size() - baseIndex)  > 1) {

		avglX = 0.0f;
		avglY = 0.0f;
		avgrX = 0.0f;
		avgrY = 0.0f;

		//Initialize window over last points to cover the duration threshold
		size = set.size();

		beginTime = set.at(baseIndex).time;
		window.push_front(set.at(baseIndex));
		do {

			if ((window.size() + baseIndex) >= size) {

				goto finishRight;
			}
			window.push_front(set.at(baseIndex + window.size()));
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
			while (distance < distanceThreshold && ((baseIndex + window.size()) < set.size())) {
				window.push_front(set.at(baseIndex + window.size()));
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


	//int q = 5;
}