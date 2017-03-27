// Daniel Lozano
// Trabajo 1 - Grilla
// g++ --std=c++11 main.cpp -o main -lGL -lGLU -lglut

#define GLUT_DISABLE_ATEXIT_HACK
#include <math.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "GL/glut.h"
#include <random>
#include <iostream>

enum MOVEMENT {KEEP=0, UP, DOWN, LEFT, RIGHT};

typedef int coordenate;

using namespace std;

#define EXP 2.7182
#define RED 0
#define GREEN 0
#define BLUE 0
#define ALPHA 1
#define ECHAP 27
#define TIME_ELAPSED .25

void init_scene();
void render_scene();
GLvoid initGL();
GLvoid window_display();
GLvoid window_reshape(GLsizei width, GLsizei height);
GLvoid window_key(unsigned char key, int x, int y);
GLvoid window_idle();

int _time=0;
int timebase=0;
float angle=0;
float angularSpeed=40.0f;
int anim = 0;
bool pause = false;

/* Clase punto en un espacio 2D*/
class Point
{
public:
    coordenate x, y;

    Point(coordenate _x, coordenate _y)
        :x(_x),y(_y) {}

    Point(Point & _p)
        :x(_p.x), y(_p.y) {}
};

/* Clase punto en un espacio 2D con las características par el sistema Grilla*/
class GridPoint
{
public:
    float pheromone;
    int food;
    bool is_nest;

    GridPoint(float _pheromone=0.0, int _food=0)
        :pheromone(_pheromone), food(_food), is_nest(false) {}
};


/* Clase grilla, representación del sistema implementado */
class Grid
{
public:
    coordenate size_x, size_y; // variables de tamanho de grilla
    Point * nest; // Puntero al punto del nido
    int max_food_per_point = 50; // cantidad maxima de comida por punto
    int agents_count; // cantidad de agentes en el sistema
    GridPoint ** matrix; // variable de grilla 


    /* Variables probalísticas de funcion softmax */
    float probability_is_nest_and_has_food = 7.0;
    float probability_is_nest = 4.0;
    float probability_food_and_has_food = 2.0;
    float probability_food = 6.0;
    float probability_pheromone_and_has_food = 4.0;
    float probability_pheromone = 3.0;
    float probability_empty = 1.0;

    
    float temperature; // variable temperatura para funcion softmax

    /* Clase agente, representación de agente en el sistema grilla */
    class Agent
    {
    public:
        Point * position; // variable de posición actual
        Point * previous_position; // variable de posición anterior
        int food; // variable cantidad de comida
        Grid * grid; // puntero a grilla

    public:
    		  /* Constructor Agente, generacion de posicion aleatoriamente, e iniciacion de variables */
        Agent(Grid & _grid)
        {
            random_device rd;
            mt19937 mt(rd());

            grid = &_grid;

            /* distribucion uniforme para posicion del agente */
            uniform_int_distribution<coordenate> dist_x(0, grid->size_x-1);
            uniform_int_distribution<coordenate> dist_y(0, grid->size_y-1);

            int temp_x = dist_x(mt);
            int temp_y = dist_y(mt);
            position = new Point(temp_x, temp_y);
            previous_position = new Point(*position);
            food = 0;
            cout << "Agent created [OK] at x = " << temp_x << " , y = " << temp_y << endl;
        }

        /* función de movimientos de agente (quedarse,  arriba,  abajo,  izquierda  y  derecha) */
        bool move(int _m) {
            switch(_m)
            {
            case KEEP: // movimiento quedarse
                previous_position->x = position->x;
                previous_position->y = position->y;
                return true;
            case LEFT: // movimiento izquierda
                if(position->x > 0)
                {
                    previous_position->x = position->x;
                    --position->x;
                    return true;
                }
                break;
            case RIGHT: // movimiento derecha
                if(position->x < grid->size_x-1)
                {
                    previous_position->x = position->x;
                    ++position->x;
                    return true;
                }
                break;
            case DOWN: // movimiento abajo
                if(position->y > 0)
                {
                    previous_position->y = position->y;
                    --position->y;
                    return true;
                }
                break;
            case UP: // movimiento arriba
                if(position->y < grid->size_y-1)
                {
                    previous_position->y = position->y;
                    ++position->y;
                    return true;
                }
                break;
            }
            return false;
        }

        /* Función para dibujar el agente */
        void draw() {
            glPushMatrix();
            glTranslatef(position->x, position->y, 0);
            glutSolidCube(.5);
            glPopMatrix();
        }
    };

    vector<Agent> * agents; // vector de agentes

    /* función de generación aleatoria de posición de nido */
    void generate_nest() {
        random_device rd;
        mt19937 mt(rd());
        uniform_int_distribution<coordenate> dist_x(0, size_x-1);
        uniform_int_distribution<coordenate> dist_y(0, size_y-1);

        int temp_x = dist_x(mt);
        int temp_y = dist_y(mt);
        nest = new Point(temp_x, temp_y);
        matrix[temp_x][temp_y].is_nest = true;

        cout << "Nest created [OK] at x = " << temp_x << " , y = " << temp_y << endl;
    }

    /* función de generación aleatoria de posiciones de comida y cantidades de comida */
    void generate_food() {
        random_device rd;
        mt19937 mt(rd());


        uniform_int_distribution<coordenate> dist_food_points(0, size_x);
        uniform_int_distribution<coordenate> dist_food_per_point(1, max_food_per_point);
        uniform_int_distribution<coordenate> dist_x(0, size_x-1);
        uniform_int_distribution<coordenate> dist_y(0, size_y-1);

        for(size_t i= dist_food_points(mt) ; i>0 ; --i)
            matrix[dist_x(mt)][dist_y(mt)].food = dist_food_per_point(mt);
    }

public:
    Grid(coordenate _size_x, coordenate _size_y, int _agents_count) {
        size_x = _size_x;
        size_y = _size_y;
        temperature = 10.0;
        agents_count = _agents_count;

        agents = new vector<Agent>();

        for(size_t i=0 ; i<_agents_count ; ++i)
            agents->push_back(Agent(*this));

        matrix = new GridPoint * [size_x];

        for(size_t i=0 ; i<size_x ; ++i)
            matrix[i] = new GridPoint[size_y];

        generate_nest();
        generate_food();
    }

    /* funcion de percepcion ( 4 vecindad) del agente con el mundo grilla  */
    void perceive(vector<float> & _probabilites, Agent & _a, int _pos, int _x_increase, int _y_increase) {
    				/* Condicionales para valores en cada accion dependiendo de las posiciones */

    			 // Si posición es nido y agente tiene comida
        if(matrix[_a.position->x+_x_increase][_a.position->y+_y_increase].is_nest and _a.food)
            _probabilites[_pos] = probability_is_nest_and_has_food;
        // Si posición lugar es nido y agente no tiene comida
        else if(matrix[_a.position->x+_x_increase][_a.position->y+_y_increase].is_nest and !_a.food)
            _probabilites[_pos] = probability_is_nest;
        // Si posición es comida y agente tiene comida
        else if(matrix[_a.position->x+_x_increase][_a.position->y+_y_increase].food and _a.food)
            _probabilites[_pos] = probability_food_and_has_food;
        // Si posición es comida y agente no tiene comida
        else if(matrix[_a.position->x+_x_increase][_a.position->y+_y_increase].food)
            _probabilites[_pos] = probability_food;
        // Si posición tiene feromona y agente tiene comida
        else if(matrix[_a.position->x+_x_increase][_a.position->y+_y_increase].pheromone and _a.food)
            _probabilites[_pos] = probability_pheromone_and_has_food;
        // Si posición tiene feromona, agente no tiene comida y no es la posicion anterior del agente
        else if(matrix[_a.position->x+_x_increase][_a.position->y+_y_increase].pheromone and !_a.food and _a.previous_position->x == _a.position->x+_x_increase and _a.previous_position->y == _a.position->y+_y_increase)
            _probabilites[_pos] = probability_pheromone;
        else // si es una posición vacia
            _probabilites[_pos] = probability_empty;
    }

    /* funcion de iteracion de cada agente, percepcion y eleccion probabilistica*/
    void iteration() {
        random_device rd;
        mt19937 mt(rd());
        uniform_int_distribution<coordenate> dist_movements(0, 4);

        /* iteracion por cada agente */
        for(auto & a : *agents)
        {
            int m;
            do
            {
                vector<float> probabilites(5, 0.0);

                perceive(probabilites, a, 0, 0, 0);
                if(a.position->y != 0) // si puede moverse hacia abajo
                    perceive(probabilites, a, 2, 0, -1);
                if(a.position->y != size_y-1) // si puede moverse hacia arriba
                    perceive(probabilites, a, 1, 0, 1);
                if(a.position->x != 0) // si puede moverse hacia la izquierda
                    perceive(probabilites, a, 3, -1, 0);
                if(a.position->x != size_x-1) // si puede moverse hacia derecha
                    perceive(probabilites, a, 4, 1, 0);

                /* calculo de sumatoria de funcion softmax */
                float sum = 0.0;
                for(auto & w : probabilites)
                    sum += pow(EXP, w/temperature); 

                /* vectores de movimiento y valores de funcion softmax para cada accion */
                vector<float> values = { 0,0,1,1,2,2,3,3,4,4 };
                vector<float> weights =  {  pow(EXP, probabilites[0]/temperature)/sum,  0, pow(EXP, probabilites[1]/temperature)/sum, 0,  pow(EXP, probabilites[2]/temperature)/sum, 0,  pow(EXP, probabilites[3]/temperature)/sum, 0,  pow(EXP, probabilites[4]/temperature)/sum};
                piecewise_constant_distribution<> d(values.begin(), values.end(), weights.begin());
                m = d(mt);
            } while(!a.move(m)); // bucle de movimiento de agente

            matrix[a.position->x][a.position->y].pheromone = 1.0; // rastro de feromona
            if(matrix[a.position->x][a.position->y].is_nest) // dejar comida en nido
            {
                matrix[a.position->x][a.position->y].food += a.food;
                a.food = 0;
            }
            else if(matrix[a.position->x][a.position->y].food and !a.food) // extraccion de comida
            {
                --matrix[a.position->x][a.position->y].food;
                ++a.food;
            }
        }
    }

    /* Función para dibujar la grilla */
    void draw() {
        for(size_t i=0,j ; i<size_x ; ++i)
        {
            for(j=0 ; j<size_y ; ++j)
            {
                glPushMatrix();
                glTranslatef(i, j, 0);

                /* dibujo de nido */
                if(matrix[i][j].is_nest)
                {
                    glColor3f(0.0, 1.0, 1.0);
                    glutSolidCube(1);
                    glColor3f(1.0, 1.0, 1.0);
                }/* dibujo de comida */
                else if(matrix[i][j].food)
                {
                    glColor3f(0.0, 1.0, 0.0);
                    glutSolidSphere(matrix[i][j].food/(1.*max_food_per_point), 8, 8);
                    glColor3f(1.0, 1.0, 1.0);
                }

                /* dibujo de feromona */
                if(matrix[i][j].pheromone)
                {
                    if (!pause and anim / 1000.0 > TIME_ELAPSED)
                    {
                        matrix[i][j].pheromone -= 0.1; // evaporacion de feromona
                        if(matrix[i][j].pheromone < 0.0)
                            matrix[i][j].pheromone = 0.0;
                    }
                    glColor3f(1.0, 0.0, 0.0);
                    glutSolidTorus(matrix[i][j].pheromone/5.0, 0.25, 6, 6);
                    glColor3f(1.0, 1.0, 1.0);
                }

               /* dibujo de lineas de la grilla */
                glColor3f(.5, .5, .5);
                glLineWidth(1);
                if(i != size_x-1)
                {
                    glBegin(GL_LINES);
                    glVertex3f(0.0, 0.0, 0.0);
                    glVertex3f(1, 0, 0);
                    glEnd();
                }

                if(j != size_y-1)
                {
                    glBegin(GL_LINES);
                    glVertex3f(0.0, 0.0, 0.0);
                    glVertex3f(0, 1, 0);
                    glEnd();
                }
                glColor3f(1.0, 1.0, 1.0);

                glPopMatrix();
            }
        }
    }
};

// Instanciacion de grilla
Grid myGrid(20, 20, 30);

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

    glutInitWindowSize(800, 800);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("CBI: Grid");

    initGL();
    init_scene();
    glutDisplayFunc(&window_display);
    glutReshapeFunc(&window_reshape);
    glutKeyboardFunc(&window_key);
    //function called on each frame
    glutIdleFunc(&window_idle);
    glutMainLoop();

    return 1;
}

GLvoid initGL() {
    GLfloat position[] = { 0.0f, 0.0f, 5.0f, 0.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_LIGHT0);    
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);    
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glClearColor(RED, GREEN, BLUE, ALPHA);
}

GLvoid window_display() {
    _time = glutGet(GLUT_ELAPSED_TIME);
    float dt = (_time-timebase);
    timebase = _time;
    angle += (angularSpeed * dt);
    anim += dt;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-25.0f, 25.0f, -25.0f, 25.0f, -25.0f, 25.0f);
    glTranslatef(-23, -23, 0);
    myGrid.draw();

    // llamda de dibujar agentes
    for(auto &a : *myGrid.agents)
        a.draw();

    if(!pause)
        if (anim / 1000.0 > TIME_ELAPSED)
        {
            myGrid.iteration(); // llamda de funcion iteracion de cada agente, percepcion y eleccion probabilistica
            anim = 0.0;
        }
    glutSwapBuffers();
    glFlush();
}

GLvoid window_reshape(GLsizei width, GLsizei height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-25.0f, 25.0f, -25.0f, 25.0f, -25.0f, 25.0f);
    glMatrixMode(GL_MODELVIEW);
}

void init_scene() {
}

GLvoid window_key(unsigned char key, int x, int y) {
    switch (key) {
    case ECHAP:
        exit(1);
        break;
    case 32:
        pause = !pause;
        break;
    default:
        printf("La touche %d non active.\n", key);
        break;
    }
}

//function called on each frame
GLvoid window_idle() {
    glutPostRedisplay();
}