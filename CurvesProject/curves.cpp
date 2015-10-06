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

//keysPressed: global boolean vector denoting whether a key is pressed or not
std::vector<bool> keysPressed(256, false);
bool drawing = false;
//int selected = -1;



/**
Curve class
 */
class Curve {
    
public:
    virtual float2 getPoint(float t)=0;
    
    //draw each curve in its own unique color
    //TODO: none should be in blue
    double color1 = ((double) rand() / (RAND_MAX));
    double color2 = ((double) rand() / (RAND_MAX));
    double color3 = ((double) rand() / (RAND_MAX));
    bool selected = false;
    
    void setSelected() {
        selected = true;
    }
    
    void setUnSelected() {
        selected = false;
    }
    
    double getColor1(){
        return color1;
    }
    
    //virtual method, since draw in Polyline overrides it
    virtual void draw(){
        
        if (selected) {
            glColor3d(0.0, 0.0, 1.0);
            glLineWidth(6);
        }
        else {
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
    //TODO: fix this
    //add a new method to curve/freeform. takes in cursor position and returns true
    //input: current x and y position of the mouse
    //output: whether the mouse is over that specific curve
    bool mouseOverCurve(float mouseX, float mouseY) {

        for (float i = 0; i < 1; i+=.01) {
            //get each point
            float2 point = getPoint(i);
            
            //that point's x
            float pointX = point.x;
            printf("%s", "point x: " );
            printf("%f", pointX );
            printf("%s", " point y: " );

            float pointY = point.y;
            printf("%f", pointY );
            //difference between mouseX and that point x
            float differenceBetweenX = mouseX - pointX;
            float differenceBetweenY = mouseY - pointY;
//            printf("%f", differenceBetweenX );
//            printf("%s", " " );
            if (fabs(differenceBetweenX) < .05 && fabs(differenceBetweenY) < .05) {
                printf("%s", "true");

                return true;
            }
        }
        printf("%s", "false");

        return false;

    }
    
};


/**
Freeform class
 */
class Freeform : public Curve
{
protected:
    std::vector<float2> controlPoints;
    std::vector<int> closeControlPoints;
    
public:
    virtual float2 getPoint(float t)=0;
    virtual void addControlPoint(float2 p)
    {
        controlPoints.push_back(p);
    }
    void drawControlPoints(){
        if (selected) {
            glBegin(GL_POINTS);
            for (float i = 0; i < controlPoints.size(); i++) {
                float2 point = controlPoints.at(i);
                float x = point.x;
                float y = point.y;
                glVertex2d(x, y);
                
            }
            glEnd();// draw points at control points
        }
    }
    //TODO: fix and test this func
    //get closest control points to mouse
    virtual std::vector<int> getClosestControlPoints(int x, int y) {
        closeControlPoints.clear();
        for (int i = 0; i < controlPoints.size(); i++) {
            if ((fabs(controlPoints[i].x - x) <= 0.05f && fabs(controlPoints[i].y - y) <= 0.05f)) {
                closeControlPoints.push_back(i);
            }
        }
        return closeControlPoints;
    }
};


/**
 Polyline class: draws a new polyline where control points clicked
 */
class Polyline : public Freeform {
public:
    float2 getPoint(float t) {
        return float2(0.0, 0.0);
    }
    //we add a control point
    void addControlPoint(float2 p)
    {
        controlPoints.push_back(p);
    }
    //should be much like drawControlPoints
    void draw(){
        if (selected) {
            glColor3d(0.0, 0.0, 1.0);
            glLineWidth(8);
            
        }
        else {
            glColor3d(color1, color2, color3);
            glLineWidth(4);
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
};

/**
 BezierCurve: extends freeform and implements curve using Bezier interpolation
 */
class BezierCurve : public Freeform

{
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

class LagrangeCurve : public Freeform
{
    std::vector<float> knots;
public:
    
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


std::vector<Freeform*> curves;

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
onKeyboard and onKeyboardUp: check for keyboard presses
 */
//each time a user presses a key, add a new curve to the respective vector
void onKeyboard(unsigned char key,int x, int y) {
    if (keysPressed[key] == false) {
        keysPressed[key] = true;
//        if (selectedCurve != NULL) {
//            selectedCurve->setUnSelected();
//        }
        switch (key) {
                
            case 'b':
                curvesContainer.addCurve(new BezierCurve());
                globalCounter ++;
                //TODO for testing: comment out
                curves.at(globalCounter)->setSelected();
                drawing = true;
                break;
            case 'l':
                //glColor3d(0.8, 0.7, 0.6);
                curvesContainer.addCurve(new LagrangeCurve());
                globalCounter ++;
                //TODO for testing: comment out
                curves.at(globalCounter)->setSelected();
                drawing = true;
                break;
            case 'p':
                //glColor3d(1.0, 0.5, 0.9);
                curvesContainer.addCurve(new Polyline());
                globalCounter ++;
                //TODO for testing: comment out
                curves.at(globalCounter)->setSelected();
                drawing = true;
                break;
        }
    }
    glutPostRedisplay();
    
}
void onKeyboardUp(unsigned char key, int x, int y) {
    keysPressed[key] = false;
    drawing = false;
    //TODO for testing: comment this out
    curves.at(globalCounter)->setUnSelected();
    glutPostRedisplay();
}



/**
 onMouse: Checks for mouse clicks. When a mouse is clicked, depending on which key is down, program behaves accordingly.
 */
void onMouse(int button, int state, int x, int y) {
    int viewportRect[4];
    glGetIntegerv(GL_VIEWPORT, viewportRect);
    //TODO: comment in the following lines:
//    float2 point1 = float2(0.5, 0.1);
//    float2 point2 = float2(0.6, 0.8);
//    float2 point3 = float2(-0.5, 0.7);
    
//    curvesContainer.addCurve(new BezierCurve());
//    Freeform *curvePointer = curves.at(0);
//    curvePointer->addControlPoint(point1);
//    curvePointer->addControlPoint(point2);
//    curvePointer->addControlPoint(point3);
    //check state --> left, right up down
    Freeform *curvePointer;
    if (state == GLUT_DOWN) {
        if (drawing == true) {
            curvePointer = curves.at(globalCounter);
            curvePointer->addControlPoint( float2(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0));
            //selectedCurve = curvePointer;

        }
        //else, nothing is clicked, so just get closest curve
        else if (drawing == false) {
            if (curves.at(0) != NULL) {
                int returnVal = curvesContainer.checkMouseCurves(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0);
                printf("%d", returnVal);
                if (selectedCurve != NULL) {
                    selectedCurve->setUnSelected();
                }
                
                selectedCurve = curves.at(returnVal);

            }
        }
    }


    
//    if (keysPressed['b'] == true) {
//        //curvesContainer.addCurve(new BezierCurve);
//        Freeform *bCurve = curves.at(curves.size());
//        bCurve->addControlPoint( float2(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0));
//    }
//    if (keysPressed['l'] == true) {
//        lCurve.addControlPoint( float2(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0));
//    }
//    if (keysPressed['p'] == true) {
//        p.addControlPoint(float2(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0));
//    }
    //@TODO: change this to no key pressed
    //select closest shape
//    if (keysPressed['d'] == true) {
//        
//    }
    
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
    glutCreateWindow("Hello");        	// Window is born
    
    glutKeyboardFunc(onKeyboard);
    glutKeyboardUpFunc(onKeyboardUp);
    //glutReshapeFunc(onReshape);
    glutMouseFunc(onMouse);
    glutDisplayFunc(onDisplay);                	// Register event handlers
    //   glutIdleFunc(onIdle);
    
    glutMainLoop();                    			// Event loop
    return 0;
}
