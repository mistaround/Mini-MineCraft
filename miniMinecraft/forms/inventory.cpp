#include "inventory.h"
#include "ui_inventory.h"
#include <stdio.h>
#include <QString>
#include <iostream>
inventory::inventory(QWidget *parent) :
    QWidget(parent),
    grassNum(5), dirtNum(5), stoneNum(5),
    waterNum(5), snowNum(5), bedrockNum(5),
    lavaNum(5), ui(new Ui::inventory_ui)
{
    ui->setupUi(this);
    slot_updateBlockNumber(GRASS_BLOCK, 0);
    slot_updateBlockNumber(DIRT, 0);
    slot_updateBlockNumber(STONE, 0);
    slot_updateBlockNumber(WATER, 0);
    slot_updateBlockNumber(SNOW, 0);
    slot_updateBlockNumber(BEDROCK, 0);
    slot_updateBlockNumber(LAVA, 0);

    connect(ui->grassRB, SIGNAL(clicked()), this, SLOT(slot_noticeSetGrass()));
    connect(ui->dirtRB, SIGNAL(clicked()), this, SLOT(slot_noticeSetDirt()));
    connect(ui->stoneRB, SIGNAL(clicked()), this, SLOT(slot_noticeSetStone()));
    connect(ui->waterRB, SIGNAL(clicked()), this, SLOT(slot_noticeSetWater()));
    connect(ui->snowRB, SIGNAL(clicked()), this, SLOT(slot_noticeSetSnow()));
    connect(ui->bedrockRB, SIGNAL(clicked()), this, SLOT(slot_noticeSetBedRock()));
    connect(ui->lavaRB, SIGNAL(clicked()), this, SLOT(slot_noticeSetLava()));


}

inventory::~inventory()
{
    delete ui;
}

Ui::inventory_ui* inventory::getUI() {
    return ui;
}


void inventory::slot_noticeSetGrass() {
    emit sig_noticeSetGrass();
}

void inventory::slot_noticeSetDirt() {
    emit sig_noticeSetDirt();
}

void inventory::slot_noticeSetStone() {
    emit sig_noticeSetStone();
}

void inventory::slot_noticeSetWater() {
    emit sig_noticeSetWater();
}

void inventory::slot_noticeSetSnow() {
    emit sig_noticeSetSnow();
}

void inventory::slot_noticeSetBedRock() {
    emit sig_noticeSetBedRock();
}

void inventory::slot_noticeSetLava() {
    emit sig_noticeSetLava();
}


void inventory::slot_updateBlockNumber(BlockType t, int num) {
    if (t == GRASS_BLOCK) {
        grassNum += num;
        std::cout << "grass num is " << grassNum;
        ui->grassLabel->setText(QString::number(grassNum));
    }
    else if (t == DIRT) {
        dirtNum += num;
        ui->dirtLabel->setText(QString::number(dirtNum));
    }
    else if (t == STONE) {
        stoneNum += num;
        ui->stoneLabel->setText(QString::number(stoneNum));
    }
    else if (t == WATER) {
        waterNum += num;
        ui->waterLabel->setText(QString::number(waterNum));
    }
    else if (t == SNOW) {
        snowNum += num;
        ui->snowLabel->setText(QString::number(snowNum));
    }
    else if (t == BEDROCK) {
        bedrockNum += num;
        ui->bedrockLabel->setText(QString::number(bedrockNum));
    }
    else if (t == LAVA) {
        lavaNum += num;
        ui->lavaLabel->setText(QString::number(lavaNum));

    }
}
