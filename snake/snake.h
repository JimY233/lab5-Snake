#ifndef SNAKE_H
#define SNAKE_H

#include <QWidget>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QtGui>

//size defied to init window size
const int BLOCK_SIZE=25;
const int MARGIN=5; 
const int NUM_ROW=20; //number of rows
const int NUM_COL=20; //number of columns

const int TIME_INTERVAL=500;

enum Direction
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();
    virtual void paintEvent(QPaintEvent *event); //refresh the window
    virtual void keyPressEvent(QKeyEvent *event); //check the key press

public:
    void Init(); 
    void PauseGame();
    void GameOver(); 
    void FoodGenerate(); 
    void PoisonGenerate(); 
    bool isGameOver(); //check whether the game is over

private slots:
    void Update();
    void quit();
    //void Start();
    void Pause();
    void Restart();
    //void save();
    void help();
    void ActivePoison();

private:
    QPushButton *quitbutton;
    //QPushButton *start;
    QPushButton *pause;
    QPushButton *restart;
    QPushButton *poisonButton;
    QPushButton *helpbutton;
    QTimer *gameTimer;
    bool isPause; //flag of Pause
    bool isPoison;
    QPoint foodPoint; //coordinates of Food
    QPoint poisonPoint; //coordinates of Poison
    QList<QPoint> snake; //a list of points as snake structure
    Direction dir; 
    int score; 
    int bestscore;
    //QAction *saveAction;
    //QAction *exitAction;
    //QMenu *fileMenu;
};

#endif // WIDGET_H
