#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cameracontrolshelp.h"
#include "playerinfo.h"
#include "./scene/chunk.h"
#include "forms/inventory.h"



namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionQuit_triggered();

    void on_actionCamera_Controls_triggered();

    void slot_updateInventory(bool isOpen);

    void slot_noticeGrass();
    void slot_noticeDirt();
    void slot_noticeStone();
    void slot_noticeWater();
    void slot_noticeSnow();
    void slot_noticeBedRock();
    void slot_noticeLava();

private:
    Ui::MainWindow *ui;
    CameraControlsHelp cHelp;
    PlayerInfo playerInfoWindow;
    inventory inventoryInfoWindow;

};


#endif // MAINWINDOW_H
