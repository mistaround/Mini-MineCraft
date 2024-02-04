#include "mainwindow.h"
#include "./scene/chunk.h"
#include <ui_mainwindow.h>
#include "cameracontrolshelp.h"
#include "ui_inventory.h"
#include <QResizeEvent>
#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), cHelp()
{
    ui->setupUi(this);
    ui->mygl->setFocus();
    this->playerInfoWindow.show();
    playerInfoWindow.move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center() + QPoint(this->width() * 0.75, 0));

//    this->inventoryInfoWindow.show();
//    inventoryInfoWindow.move(QGuiApplication::primaryScreen()->availableGeometry().center() + this->rect().center() - QPoint(this->width() * 0.75, 0));

    connect(ui->mygl, SIGNAL(sig_sendPlayerPos(QString)), &playerInfoWindow, SLOT(slot_setPosText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerVel(QString)), &playerInfoWindow, SLOT(slot_setVelText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerAcc(QString)), &playerInfoWindow, SLOT(slot_setAccText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerLook(QString)), &playerInfoWindow, SLOT(slot_setLookText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerChunk(QString)), &playerInfoWindow, SLOT(slot_setChunkText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerTerrainZone(QString)), &playerInfoWindow, SLOT(slot_setZoneText(QString)));

    // milestone 3: inventory
    connect(ui->mygl, SIGNAL(sig_sendInventory(bool)), this, SLOT(slot_updateInventory(bool)));
    connect(ui->mygl, SIGNAL(sig_updateInventory(BlockType,int)), &inventoryInfoWindow,
            SLOT(slot_updateBlockNumber(BlockType,int)));

    connect(&inventoryInfoWindow, SIGNAL(sig_noticeSetGrass()), this, SLOT(slot_noticeGrass()));
    connect(&inventoryInfoWindow, SIGNAL(sig_noticeSetDirt()), this, SLOT(slot_noticeDirt()));
    connect(&inventoryInfoWindow, SIGNAL(sig_noticeSetStone()), this, SLOT(slot_noticeStone()));
    connect(&inventoryInfoWindow, SIGNAL(sig_noticeSetWater()), this, SLOT(slot_noticeWater()));
    connect(&inventoryInfoWindow, SIGNAL(sig_noticeSetSnow()), this, SLOT(slot_noticeSnow()));
    connect(&inventoryInfoWindow, SIGNAL(sig_noticeSetBedRock()), this, SLOT(slot_noticeBedRock()));
    connect(&inventoryInfoWindow, SIGNAL(sig_noticeSetLava()), this, SLOT(slot_noticeLava()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::exit();
}

void MainWindow::on_actionCamera_Controls_triggered()
{
    cHelp.show();
}

void MainWindow::slot_noticeGrass() {
    ui->mygl->setCurBlockType(GRASS_BLOCK);
}
void MainWindow::slot_noticeDirt() {
    ui->mygl->setCurBlockType(DIRT);
}

void MainWindow::slot_noticeStone() {
    ui->mygl->setCurBlockType(STONE);
}

void MainWindow::slot_noticeWater() {
    ui->mygl->setCurBlockType(WATER);
}

void MainWindow::slot_noticeSnow() {
    ui->mygl->setCurBlockType(SNOW);
}

void MainWindow::slot_noticeBedRock() {
    ui->mygl->setCurBlockType(BEDROCK);
}

void MainWindow::slot_noticeLava() {
    ui->mygl->setCurBlockType(LAVA);
}

void MainWindow::slot_updateInventory(bool isOpen) {
    std::cout << "in slot update inventory" << std::endl;
    if (isOpen) {
        this->inventoryInfoWindow.show();
        inventoryInfoWindow.move(QGuiApplication::primaryScreen()->availableGeometry().center()
                                 - this->rect().center() - QPoint(this->width() * 0.01, 0));

        BlockType t = this->ui->mygl->getPlayer().selectedBlockType;

        if (t == GRASS_BLOCK) {
            this->inventoryInfoWindow.getUI()->grassRB->setChecked(true);
        }
        else if (t == DIRT) {
            this->inventoryInfoWindow.getUI()->dirtRB->setChecked(true);
        }
        else if (t == STONE) {
            this->inventoryInfoWindow.getUI()->stoneRB->setChecked(true);
        }
        else if (t == WATER) {
            this->inventoryInfoWindow.getUI()->waterRB->setChecked(true);
        }
        else if (t == SNOW) {
            this->inventoryInfoWindow.getUI()->snowRB->setChecked(true);
        }
        else if (t == BEDROCK) {
            this->inventoryInfoWindow.getUI()->bedrockRB->setChecked(true);
        }
        else if (t == LAVA) {
            this->inventoryInfoWindow.getUI()->lavaRB->setChecked(true);
        }
    }
    else {
        inventoryInfoWindow.close();
    }
}

