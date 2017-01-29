# 3D-Eye-Tracking
3D eye tracker implemented in C++ (OpenGL) which uses convergence between each eyes gaze vector while viewing 3D redners with shutter glasses. This may have a use in a variety of applications including hands free viewing of 3D medical data sets such as MRI as well as use in the entertainment industry. I would recommend reading through the Developer Guide to get a better understanding of the software and hardware used as well as the results. 

##User Manual / General Information


###Program Stages

2D Calibation (Plane_Calibration)
Runs through 9 calibration points

2D Result
Shows the result of the 2D calibration

3D Calibration (depth_Calibration)
Runs through 36 calibration points ( 9 points on 4 planes)

3D Result
Shows the result of the 3D calibration

Sandbox
Allows user to choose between 3 mini games to evaluate the accuracy of various calibration methods


###Keymap

####General
esc->exit program

####2D Calibration
q->skip to 3d calibration

####2D Display Result
q->next
r->toggle right eye raw
l->toggle left eye raw
w->toggle left average
e->toggle right average
t->toggle linear fit

####3D Display result
q->next
r->toggle right eye raw
l->toggle left eye raw
w->toggle left fixation
e->toggle right fixation
s->plane 1
d->plane 2
f->plane 3
g->plane 4
h->set 1
j->set 2 
k->set 3
l->set 4 
p->replay (not done)
t->toggle linear fit
y->display average
u->display clean average
i->display weighted average
o->display median
p->display dti

####Sandbox
i->select target practice mini game
o->select image viewer
p->select static target environment
s->increase plane depth
d->decrease plane depth
e->no fit
r->linear fit
t->(x,y,eyesep)-> z linear regression
y->(x,y,z)->x (x,y,z)->y (x,y,z)->z linear regression
f->0
g->10
h->20
j->30
k->40
l->50
z->60
x->70
c->80
v->90
b->100

###Program Files

####Camera.h / Camera.cpp
-Used to setup steresocopic frustums and view matrices

####Control.h/ Control.cpp
-processes user key input for particular stage
-updates state of current stage (passes gazepoint data etc)
-draws screen based on current stage and its values

####Depth_Calibration.h
-handles bothe the 3D calibration and 3D result stage

####GPClient.h / GPClient.cpp stdafx.h /targetver.h
-Used to read from the Gazepoint API 
NOTE: I've included the gazepoint API manual for reference

####Plane_Calibration.h
-Handles both the 2D calibration and 2D result stages

####Point_Renderer.h / Point_Renderer.cpp
-Used to draw circles on screen

####Resource_Manager.h / Resoruce Manager.cpp
-handles shaders and textures

####Sandbox.h 
-handles sandbox stage

####Shader.h / Shader.cpp
-handles shaders

####Sprite_Renderer.h / Sprite_Renderer.cpp
-used to draw rectangles on screen

####Texture.h / Texture.cpp
-handles textures

###Inlcudes

opengl32.lib
glfw3.lib
glew32s.lib
SOIL.lib

###Additional information

When first compiling you may run into an error : "Please #define _AFXDLL or do not use /MD[d]"
Changing the following should fix it

Project -> "project" Properties -> Configuration Properties -> C/C++ -> Advanced -> Show Includes:YES(/showIncludes)

Project -> "project" Properties -> Configuration Properties -> General -> Project Defaults -> Use of MFC :Use MFC in a shared DLL
