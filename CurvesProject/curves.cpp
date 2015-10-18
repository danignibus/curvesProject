//
//  curves.cpp
//  CurvesProject
//
//  Created by Dani Gnibus on 9/28/15.
//  Copyright (c) 2015 Dani Gnibus. All rights reserved.
//
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
// Needed on MsWindows
#include <windows.h>
#endif // Win32 platform

#include <vector>
#include <OpenGL/gl.h>
#include "float2.h"
#include <OpenGL/glu.h>
// Download glut from: http://www.opengl.org/resources/libraries/glut/
#include <GLUT/glut.h>
#include "curves.h"

//defining global variables
std::vector<bool> keysPressed(256, false);
bool drawing = false;
bool addingPoints = false;
bool deletingPoints = false;
bool movingAPoint = false;
int currSelectedCurve;
int controlPointVal;

/**
Curve class: Defines a virtual curve that the curves in this project inherit from
 */
class Curve {
    
public:
    //draw each curve in its own unique color
    int curveType;
    
    Curve(int curveType) : curveType(curveType) {}
//    double color1 = ((double) rand() / (RAND_MAX));
//    double color2 = ((double) rand() / (RAND_MAX));
//    double color3 = ((double) rand() / (RAND_MAX));
    double color1, color2, color3;
    bool selected = false;


    int getCurveType() {
        return curveType;
    }
    
    virtual float2 getPoint(float t)=0;
    
    void setSelected() {
        selected = true;
    }
    
    void setUnSelected() {
        selected = false;
    }
    
    //virtual method, since draw in Polyline overrides it
    virtual void draw(){
        

        
        //if curve is selected, draw it in blue with double width
        if (selected) {
            glColor3d(0.0, 0.0, 1.0);
            glLineWidth(6);
        }
        
        //else, draw in a random color
        else {
            //polyline coloring taken care of in polyline class
            if (curveType ==1.0) {
                color1 = 0.2;
                color2 = 0.9;
                color3 = 0.2;
            }
            //lagrange coloring
            else {
                color1 = 1.0;
                color2 = 0.4;
                color3 = 0.7;
            }

            
            glColor3d(color1, color2, color3);
            glLineWidth(3);
        }
        
        glBegin(GL_LINE_STRIP);
        for (float i = 0; i < 1; i+=.01) {
            float2 point = getPoint(i);
            float x = point.x;
            float y = point.y;
            glVertex2d(x, y);
        }
        glEnd();
    };

    //mouseOverCurve: takes in cursor position and returns true if mouse is over current curve
    //input: current x and y position of the mouse
    //output: True if mouse is over this curve, false otherwise.
    virtual bool mouseOverCurve(float mouseX, float mouseY) {

        for (float i = 0; i < 1; i+=.01) {
            //get each point
            float2 point = getPoint(i);
            float pointX = point.x;
            float pointY = point.y;

            //difference between mouseX and that point x
            float differenceBetweenX = mouseX - pointX;
            float differenceBetweenY = mouseY - pointY;
            
            //if the difference is marginal, return true. else return false
            if (fabs(differenceBetweenX) < .05 && fabs(differenceBetweenY) < .05) {
                return true;
            }
        }
        return false;
    }
};


/**
Freeform class. Contains additional methods for dealing with control points: getting, setting, replacing, deleting, drawing.
 */
class Freeform : public Curve
{

protected:
    std::vector<float2> controlPoints;
    std::vector<int> controlPointsNearClick;
    
public:
    
    Freeform(int curveType) : Curve(curveType) {}

    virtual float2 getPoint(float t)=0;
    
    virtual void addControlPoint(float2 p)
    {
        controlPoints.push_back(p);
    }
    
    float2* getControlPoint(int index) {
        return &controlPoints.at(index);
    }
    
    void setNewControlPointValue(int index, float2 newValue) {
        controlPoints.at(index) = newValue;
    }
    
    virtual void eraseControlPoint(int point) {
        controlPoints.erase(controlPoints.begin() + point);
    }//override in lagrange
    
    void drawControlPoints(){
        if (selected) {
            glBegin(GL_POINTS);
            for (float i = 0; i < controlPoints.size(); i++) {
                float2 point = controlPoints.at(i);
                float x = point.x;
                float y = point.y;
                glVertex2d(x, y);
                
            }
            glEnd();
        }
    }
    
    int getControlPointsSize() {
        return controlPoints.size();
    }
    
    //get closest control point to mouse
    //returns point that's closest or -1 if no point is close enough
    int getControlPointNearMouse(float x, float y) {
        for (int i = 0; i < controlPoints.size(); i++) {
            float ctrlPtX = controlPoints.at(i).x;
            float ctrlPtY = controlPoints.at(i).y;
            
            //if difference is marginal, return that control point. else return -1
            if ((fabs(ctrlPtX - x) < 0.05f && fabs(ctrlPtY - y) < 0.05f)) {
                return i;
            }
        }
        return -1;
    }
};
std::vector<Freeform*> curves;

/**
 Polyline class: draws a new polyline where control points are clicked.
 */
class Polyline : public Freeform {
public:
   // curveType = 0;
    Polyline() : Freeform(0.0) {}
    
    float2 getPoint(float t) {
        return float2(0.0, 0.0);
    }

    void addControlPoint(float2 p)
    {
        controlPoints.push_back(p);
    }
    
    //like drawControlPoints
    void draw(){
        if (selected) {
            glColor3d(0.0, 0.0, 1.0);
            glLineWidth(6);
        }
        
        else {
            glColor3d(0.6, 0.1, 0.8);
            glLineWidth(3);
        }
        glBegin(GL_LINE_STRIP);
        
        for (float i = 0; i < controlPoints.size(); i++) {
            float2 point = controlPoints.at(i);
            float x = point.x;
            float y = point.y;
            glVertex2d(x, y);
            
        }
        glEnd();
    };
    
    //check if mouse is over curve. since we don't have a getPoint function for polyline, have to do this a little differently
    bool mouseOverCurve(float mouseX, float mouseY) {
        for (int i = 0; i < controlPoints.size() -1; i++) {
            bool doesPointExist = pointBetweenCtrlPoints(controlPoints.at(i), controlPoints.at(i+1), mouseX, mouseY);
            if (doesPointExist) {
                return true;
            }
        }
        return false;
    }
    
    
    //idea for equation from: http://stackoverflow.com/questions/328107/how-can-you-determine-a-point-is-between-two-other-points-on-a-line-segment
    bool pointBetweenCtrlPoints(float2 ctrlPoint1, float2 ctrlPoint2, float mouseX, float mouseY) {
        //first, check if cross product of (b-a) and (c-a) is 0
        float crossProduct = (mouseY - ctrlPoint1.y) * (ctrlPoint2.x - ctrlPoint1.x) - (mouseX - ctrlPoint1.x) * (ctrlPoint2.y - ctrlPoint1.y);
        if (fabs(crossProduct) > 0.05){
            return false;
        }
        
        //next, check if dot product is positive
        float dotProduct = (mouseX - ctrlPoint1.x) * (ctrlPoint2.x - ctrlPoint1.x) + (mouseY - ctrlPoint1.y)*(ctrlPoint2.y - ctrlPoint1.y);
        if (dotProduct < 0) {
            return false;
        }
        
        //now, check that dot product is less than square of distance between ctrlPoint1 and ctrlPoint2
        float lengthSquared = (ctrlPoint2.x - ctrlPoint1.x) * (ctrlPoint2.x - ctrlPoint1.x) + (ctrlPoint2.y - ctrlPoint1.y) * (ctrlPoint2.y - ctrlPoint1.y);
        if (dotProduct > lengthSquared) {
            return false;
        }
        return true;
    }

    
};

/**
 BezierCurve: extends freeform and implements curve using Bezier interpolation
 */
class BezierCurve : public Freeform

{
    public:
    BezierCurve() : Freeform(1.0) {}

    static double bernstein(int i, int n, double t) {
        //i is index of control point, n is one less than number of control points
        if(n == 1) {
            if(i == 0) return 1-t;
            if(i == 1) return t;
            return 0;
        }
        if(i < 0 || i > n) return 0;
        return (1 - t) * bernstein(i,   n-1, t)
        +      t  * bernstein(i-1, n-1, t);
    }
    
    float2 getPoint(float t) //calculates a point from the control points
    {
        float2 r(0.0, 0.0);
        float weight;
        // for every control point
        for (float i = 0; i < controlPoints.size(); i++) {
            // compute weight using the Bernstein formula
            weight = bernstein(i, controlPoints.size()-1, t);
            r += controlPoints.at(i)*weight;
        }
        // add control point to r, weighted
        return r;
    }
};
Freeform *selectedCurve;

/**
 LagrangeCurve: extends freeform and implements curve using Lagrange interpolation
 */
class LagrangeCurve : public Freeform
{
    std::vector<float> knots;
public:
    LagrangeCurve() : Freeform(2) {}
    
    void addControlPoint(float2 p)
    {
        controlPoints.push_back(p);
        int vecSize = controlPoints.size();
        knots.clear();
        if (vecSize ==1) {
            knots.push_back(0);
            return;
        }
        double knotIncrement = 1.0/ (vecSize -1);
        double counter = 0;
        for (int i = 0; i < vecSize; i++) {
            knots[i] = counter;
            counter += knotIncrement;
        }
    }
    
    void eraseControlPoint(int point) {
        controlPoints.erase(controlPoints.begin() + point);
        int vecSize = controlPoints.size();

        knots.clear();
        if (vecSize ==1) {
            knots.push_back(0);
            return;
        }
        double knotIncrement = 1.0/ (vecSize -1);
        double counter = 0;
        for (int i = 0; i < vecSize; i++) {
            knots[i] = counter;
            counter += knotIncrement;
        }
    }
    
    double lagrange(int i, int n, double t) {
        //i is index of control point, n is one less than number of control points
        //t0, t1, ...tn-1 are the knot values: must be chosen by modeler just like control points
        //r0, r1...rn-1: control points sequence.
        double numerator = 1;
        double denominator = 1;
        
        for (int j = 0; j <= controlPoints.size()-1; j++)
        {
            
            if (j != i) {
                //subtract knot value at j
                numerator *= (t - knots[j]);
                denominator *= (knots[i] - knots[j]);
            }
        }
        double returnWeight = numerator/denominator;
        return returnWeight;
    }

    float2 getDerivative(float t) {
        return float2(0.0,0.0);
    }
    
    float2 getPoint(float t) //calculates a point from the control points
    {
        float2 r(0.0, 0.0);
        double weight;
        // for every control point
        for (float i = 0; i < controlPoints.size(); i++) {
            // compute weight using the Bernstein formula
            weight = lagrange(i, controlPoints.size()-1, t);
            r += controlPoints.at(i)*weight;
        }
        // add control point to r, weighted
        return r;
    }
};


//this class manages all the objects. we store object pointers.
class CurvesContainer
{
public:

    //registers an object in this countainer
    void addCurve(Freeform* curve) {
        curves.push_back(curve);
    }
    //releases all the objects stored in the object container. we only need to do this with objects created with "new"
    CurvesContainer() {
        for(unsigned int i=0; i<curves.size(); i++)
            delete curves.at(i);
    }
    //draws all the objects in the container
    void draw() {
        for(unsigned int i=0; i<curves.size(); i++) {
            curves.at(i)->draw();
        }
    }
    void drawControlPoints() {
        for(unsigned int i=0; i<curves.size(); i++) {
            curves.at(i)->drawControlPoints();
        }
    }
    int checkMouseCurves(float x, float y) {
        for (unsigned int i = 0; i < curves.size(); i++ ) {
            if (curves.at(i)->mouseOverCurve(x, y)) {
                return i;
            }
        }
        return -1;
    }

};
CurvesContainer curvesContainer;
int globalCounter = -1;

/**
onKeyboard: checks for keyboard presses. 
Each time a user presses a key that indicates they want to draw that type of curve (ie p, l, b), a new curve should be added to the respective vector. 
 If the user presses a, sets a boolean to true indicating that we are now appending points.
 If the user presses d, sets a boolean to true indicating that we are now deleting points.
 */
void onKeyboard(unsigned char key,int x, int y) {
    if (keysPressed[key] == false) {
        keysPressed[key] = true;

        switch (key) {
            
            //add a bezier curve
            case 'b':
                if (selectedCurve != NULL) {
                    selectedCurve->setUnSelected();}
                curvesContainer.addCurve(new BezierCurve());
                globalCounter ++;
                selectedCurve = curves.at(globalCounter);
                currSelectedCurve = globalCounter;
                drawing = true;
                break;
                
            //add a lagrange curve
            case 'l':
                if (selectedCurve != NULL) {
                    selectedCurve->setUnSelected();}
                curvesContainer.addCurve(new LagrangeCurve());
                globalCounter ++;
                selectedCurve = curves.at(globalCounter);
                currSelectedCurve = globalCounter;
                drawing = true;
                break;
            
            //add a polyline
            case 'p':
                if (selectedCurve != NULL) {
                    selectedCurve->setUnSelected();}
                curvesContainer.addCurve(new Polyline());
                globalCounter ++;
                selectedCurve = curves.at(globalCounter);
                currSelectedCurve = globalCounter;
                drawing = true;
                break;
                
            //deleting points
            case 'd':
                if (selectedCurve!=NULL) {
                    deletingPoints = true;
                }
                break;
            
            //cycle through all curves
            case ' ':
                if (selectedCurve != NULL) {
                    selectedCurve->setUnSelected();
                    if (currSelectedCurve +1 <= curves.size() -1) {
                        currSelectedCurve ++;
                        selectedCurve = curves.at(currSelectedCurve);
                    }
                    else {
                        selectedCurve = curves.at(0);
                        currSelectedCurve = 0;
                    }
                }
                else {
                    if (selectedCurve != NULL) {
                        selectedCurve = curves.at(0);
                        currSelectedCurve = 0;
                    }

                }
                break;
            
            //appending points
            case 'a':
                if (selectedCurve != NULL && curves.at(0) != NULL) {
                    drawing = false;
                    addingPoints = true;
                }
                break;
        }
    }
    glutPostRedisplay();
}

/**
 onKeyboardUp: checks for keys coming up
When a key is lifted, all booleans are reset to false and we make sure that there are no curves with less than 2 control points.
 */
void onKeyboardUp(unsigned char key, int x, int y) {
    keysPressed[key] = false;
    drawing = false;
    addingPoints = false;
    deletingPoints = false;
    if (curves.size() > 0) {
        Freeform *checkCtrlPtNum = curves.at(globalCounter);
        int controlPointsSize = checkCtrlPtNum->getControlPointsSize();
        if (controlPointsSize < 2) {
            //delete that curve
            curves.erase(curves.begin() + globalCounter);
            globalCounter --;
            currSelectedCurve = -1;
            selectedCurve = NULL;
        }
    }
    glutPostRedisplay();
}

/**
 onMouse: Checks for mouse clicks. When a mouse is clicked, depending on which key is down, program behaves accordingly.
 */
void onMouse(int button, int state, int x, int y) {

    int viewportRect[4];
    glGetIntegerv(GL_VIEWPORT, viewportRect);

    //check state --> left, right up down
    Freeform *curvePointer;
    
    //Case: mouse is pressed
    if (state == GLUT_DOWN) {
        
        //if we're currently drawing one of the curves (p, b, or l is pressed)
        if (drawing == true) {
            curvePointer = curves.at(globalCounter);
            curvePointer->addControlPoint( float2(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0));
            //TODO: if having problems w/ selected curve, check this
            selectedCurve = curvePointer;

        }
        
        //else, nothing is clicked, so just get closest curve
        if (addingPoints) {
            selectedCurve->addControlPoint( float2(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0));
        }
        
        if (deletingPoints) {
            int pointToDelete = selectedCurve->getControlPointNearMouse(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0);
            if (pointToDelete != -1) {
                selectedCurve->eraseControlPoint(pointToDelete);
            }
            if (selectedCurve->getControlPointsSize() <2) {
                curves.erase(curves.begin() + currSelectedCurve);
                currSelectedCurve = -1;
                globalCounter --;
                selectedCurve = NULL;
            }
        }
        
        //on a mouse click, if we're not drawing, see if we're currently touching a curve or a point
        else if (drawing == false) {
            //changed this from curves.at(0) != NULL
            if (curves.size() >0) {
                int returnVal = curvesContainer.checkMouseCurves(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0);
                if (selectedCurve != NULL) {
                    selectedCurve->setUnSelected();
                    currSelectedCurve = -1;
                }
                
                if (returnVal != -1) {
                    selectedCurve = curves.at(returnVal);
                    currSelectedCurve = returnVal;
                }

                else {
                    if (selectedCurve != NULL) {
                        selectedCurve->setUnSelected();
                       // selectedCurve = NULL;
                        currSelectedCurve = -1;
                    }
                }
                if (selectedCurve != NULL) {
                    controlPointVal = selectedCurve->getControlPointNearMouse(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0);
                    
                    if (controlPointVal != -1) {
                        //we are currently on a control point
                        movingAPoint = true;
                        // float2 *controlPointPointer = selectedCurve->getControlPoint(controlPointVal);
                    }
                }
                
            }
            
        }
    }
    
    //when we let go of a point we've been dragging, it should remain at the spot where we lift the mouse
    if (state == GLUT_UP) {
        if (movingAPoint) {
            float2 xAndY = float2(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0);
            selectedCurve->setNewControlPointValue(controlPointVal, xAndY);
            movingAPoint = false;
        }
        
    }
    glutPostRedisplay();
}

/**
 onMouseMotionFunc: Will constantly set new control point value if control point is currently being moved.
 */
void onMouseMotionFunc(int x, int y) {
    int viewportRect[4];
    glGetIntegerv(GL_VIEWPORT, viewportRect);
    if (movingAPoint) {
        float2 xAndY = float2(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0);
        selectedCurve->setNewControlPointValue(controlPointVal, xAndY);
    }
    glutPostRedisplay();
}


void onDisplay( ) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPointSize(10);
    if (selectedCurve != NULL) {
        selectedCurve->setSelected();
    }
    curvesContainer.draw();
    glColor3d(1.0, 1.0, 1.0);
    curvesContainer.drawControlPoints();
    
    glutSwapBuffers();                     		// Swap buffers for double buffering
    
}

//--------------------------------------------------------
// The entry point of the application
//--------------------------------------------------------
int main(int argc, char *argv[]) {
    glutInit(&argc, argv);                 		// GLUT initialization
    glutInitWindowSize(640, 480);				// Initial resolution of the MsWindows Window is 600x600 pixels
    glutInitWindowPosition(100, 100);            // Initial location of the MsWindows window
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);    // Image = 8 bit R,G,B + double buffer + depth buffer
    glutCreateWindow("Curves Editor");        	// Window is born
    
    glutKeyboardFunc(onKeyboard);
    glutKeyboardUpFunc(onKeyboardUp);
    //glutReshapeFunc(onReshape);
    glutMouseFunc(onMouse);
    glutDisplayFunc(onDisplay);                	// Register event handlers
    glutMotionFunc(onMouseMotionFunc);
    //   glutIdleFunc(onIdle);
    
    glutMainLoop();                    			// Event loop
    return 0;
}
