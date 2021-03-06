/* 
 * File:   ReactiveControl.cpp
 * Author: alex
 * 
 * Created on 17 de mayo de 2012, 20:02
 */

#include <vector>

#include "ReactiveControl.h"

ReactiveControl::ReactiveControl() {
    outputAdvance = 0.0;
    outputRotation = 0.0;
    va = 0.0;
    vg = 0.0;
    pointsObstacle.clear();
    rangeObstacle.clear();
    angleObstacle.clear();

    openingangleFR = PI / 3.0;
    openingangleLT = PI / 2.0;

}

ReactiveControl::ReactiveControl(const ReactiveControl& orig) {
}

ReactiveControl::~ReactiveControl() {
}

void ReactiveControl::RangeAction(float frontal, float frontalmin, float lateral) {
    rangeMaxActionFront = frontal;
    rangeMinActionFront = frontalmin;
    rangeActionLateral = lateral;
}

void ReactiveControl::ConfigReactiveControl(float kav, float krt, float veladv, float velrot) {
    kadv = kav;
    krot = krt;
    velmaxad = veladv;
    velmaxrot = velrot;
}

void ReactiveControl::SetObstacle(CinematicMap& obstacle) {
    obstacle.GetObstacle(pointsObstacle, rangeObstacle, angleObstacle);
    ObstacleDangerFront();
    ObstacleDangerLateral();
}

void ReactiveControl::SetCommand(float va_, float vg_) {
    va = va_;
    vg = vg_;
    outputAdvance = va;
    outputRotation = vg;
    Compute();

    if (va < 0.01 && vg < 0.01) { //Fin de la trayectoria
        outputRotation = 0.0;
        outputAdvance = 0.0;
    }
}

void ReactiveControl::ObstacleDangerFront() {
    pointsObstacleDangerFR.clear();
    rangeObstacleDangerFR.clear();
    angleObstacleDangerFR.clear();

    for (int i = 0; i < pointsObstacle.size(); i++) {
        if (fabs(angleObstacle[i].getValue()) < openingangleFR) {
            if (rangeObstacle[i] < rangeMaxActionFront) {
                pointsObstacleDangerFR.push_back(pointsObstacle[i]);
                rangeObstacleDangerFR.push_back(rangeObstacle[i]);
                angleObstacleDangerFR.push_back(angleObstacle[i]);
            }
        }

    }
}

void ReactiveControl::ObstacleDangerLateral() {
    pointsObstacleDangerLT.clear();
    rangeObstacleDangerLT.clear();
    angleObstacleDangerLT.clear();

    pointSecureObstacle = 10.0; //Iniciamos valor alto para calculo minimo

    for (int i = 0; i < pointsObstacle.size(); i++) {
        if (fabs(angleObstacle[i].getValue()) < openingangleLT) //Solo nos importa de -90º a 90º
        {
            if (rangeObstacle[i] < rangeActionLateral) { //Cuando dist menor a uno dado
                pointsObstacleDangerLT.push_back(pointsObstacle[i]);
                rangeObstacleDangerLT.push_back(rangeObstacle[i]);
                angleObstacleDangerLT.push_back(angleObstacle[i]);
            }
        }

        //Punto laser de distancia minima:
        if (rangeObstacle[i] < pointSecureObstacle)
            pointSecureObstacle = rangeObstacle[i];
    }
}

void ReactiveControl::Compute() {
    outputRotation = vg;
    outputAdvance = va;
    int angdch = 0;
    int angizq = 0;
    int smallObstacle = 60;
    float areaSecureObstacle = 0.40; //40cm
    cout << "pointsObstacleLateral " << pointsObstacleDangerLT.size() << " pointsObstacleFrontal " << pointsObstacleDangerFR.size() << endl;

    //**************** No hay object
    if (pointsObstacleDangerLT.size() <= 0 && pointsObstacleDangerFR.size() <= 0) {
        cout << "TRAJECTORY CONTROL" << endl;
        pointSecureObstacle = 10.0; //Reiniciamos valor minimo si no hay obstaculos
        return;
    }

    //****** 1) Velocidad de avance disminuye cuando se acerca a objeto
    if (pointsObstacleDangerFR.size() > 0) {
        float auxRangeObstacle = rangeMaxActionFront; //Inicial para coger el minimo
        float distmax = rangeMaxActionFront;
        float distmin = rangeMinActionFront;

        for (int i = 0; i < pointsObstacleDangerFR.size(); i++) {                            
            if (rangeObstacleDangerFR[i] < auxRangeObstacle) {
                auxRangeObstacle = rangeObstacleDangerFR[i];
            }
             if (angleObstacleDangerFR[i].getValue() < 0)
                    angdch++;
                else
                    angizq++;

        }

        float error = (auxRangeObstacle - distmin) / (distmax - distmin);

        if (pointSecureObstacle < areaSecureObstacle) {//Robot muy cerca del obstaculo, solo control reactivo
            outputAdvance = kadv * error;
            cout << "REACTCTRL. ADVANCE" << endl;
        } 
        else if (pointsObstacleDangerLT.size() >= smallObstacle) {//No esta cerca, pero obstaculo muy grande
            outputAdvance = kadv * error;
            cout << "REACTCTRL. ADVANCE" << endl;
        }
         
         else {//No esta cerca, y obstaculo pequeño
            //Daremos una ponderacion del 50% a ambos controles para
            //Que actuen los dos
            outputAdvance = ((kadv * error)+(va)) / 2.0;
            cout << "TRAJECTORY & REACTIVE CONTROL ADVANCE" << endl;

        }
    }

    //****** 2) Velocidad de giro  
    if (pointsObstacleDangerLT.size() > 0) {
        double auxangleMin;
        float auxrangeGiroMin = rangeActionLateral;

            for (int i = 0; i < pointsObstacleDangerLT.size(); i++) {
                if (rangeObstacleDangerLT[i] < auxrangeGiroMin) {
                    auxrangeGiroMin = rangeObstacleDangerLT[i];
                    auxangleMin = angleObstacleDangerLT[i].getValue();
                }
            }

            float errorg = openingangleLT + 0.01 - fabs(auxangleMin);

            if (pointSecureObstacle < areaSecureObstacle) {//Robot muy cerca del obstaculo, solo control reactivo
                if (angizq < angdch) {
                    cout << "REACTCTRL. LEFT" << endl;
                    outputRotation = krot*errorg;
                } else {
                    cout << "REACTCTRL. RIGHT" << endl;
                    outputRotation = -krot*errorg;
                }
            } 
            else if (pointsObstacleDangerLT.size() >= smallObstacle) {//No esta cerca, pero obstaculo muy grande
                if (angizq < angdch) {
                    cout << "REACTCTRL. LEFT" << endl;
                    outputRotation = krot*errorg;
                } else {
                    cout << "REACTCTRL. RIGHT" << endl;
                    outputRotation = -krot*errorg;
                }
            }
            else {//No esta cerca, y obstaculo pequeño
                //Daremos una ponderacion del 50% a ambos controles para
                //Que actuen los dos
                //vg puede ser negativo o positivo, por lo que no depende del angulo
                cout << "TRAJECTORY & REACTCTRL ROT" << endl;
                outputRotation = (outputRotation + vg) / 2.0;

            }

    }
    //SATURACION
    if (outputAdvance > velmaxad)
        outputAdvance = velmaxad;

    if (abs(outputRotation) > velmaxrot) {
        if (outputRotation > 0)
            outputRotation = velmaxrot;
        else if (outputRotation < 0)
            outputRotation = -velmaxrot;
        else
            outputRotation = outputRotation;
    }

}

void ReactiveControl::GetCommand(float& vad, float& vrot) {
    vad = outputAdvance;
    vrot = outputRotation;
}

void ReactiveControl::Draw() {

    if (!pointsObstacleDangerLT.empty()) {
        glPushMatrix();
        //Puntos Peligrosos (menor que rangeAction)
        glPointSize(1.5);
        glColor3ub(50, 0, 255);
        for (int i = 0; i < pointsObstacleDangerLT.size(); i++) {
            glBegin(GL_POINTS);
            glVertex3f(pointsObstacleDangerLT[i].x, pointsObstacleDangerLT[i].y, 0.4);
            glEnd();
        }
        glPopMatrix();
    }

    if (!pointsObstacleDangerFR.empty()) {
        //Puntos peligtosos frontales
        glPushMatrix();
        glPointSize(3.5);
        glColor3ub(255, 0, 0);
        for (int i = 0; i < pointsObstacleDangerFR.size(); i++) {
            glBegin(GL_POINTS);
            glVertex3f(pointsObstacleDangerFR[i].x, pointsObstacleDangerFR[i].y, 0.4);
            glEnd();
        }
        glPopMatrix();
    }

}