#ifndef INVENTORY_H
#define INVENTORY_H

#include <QWidget>
#include "./scene/chunk.h"

namespace Ui {
class inventory_ui;
}

class inventory : public QWidget
{
    Q_OBJECT

public:
    int grassNum, dirtNum, stoneNum, waterNum, snowNum, bedrockNum, lavaNum;
    bool uiIsOpen;
    explicit inventory(QWidget *parent = nullptr);
    ~inventory();
    Ui::inventory_ui* getUI();


private:
    Ui::inventory_ui *ui;

private slots:

    void slot_updateBlockNumber(BlockType t, int num);
    void slot_noticeSetGrass();
    void slot_noticeSetDirt();
    void slot_noticeSetStone();
    void slot_noticeSetWater();
    void slot_noticeSetSnow();
    void slot_noticeSetBedRock();
    void slot_noticeSetLava();


signals:
    void sig_setCurBlockGrass();
    void sig_setCurBlockDirt();
    void sig_setCurBlockStone();
    void sig_setCurBlockWater();
    void sig_setCurBlockSnow();
    void sig_setCurBlockBedRock();
    void sig_setCurBlockLava();

    void sig_noticeSetGrass();
    void sig_noticeSetDirt();
    void sig_noticeSetStone();
    void sig_noticeSetWater();
    void sig_noticeSetSnow();
    void sig_noticeSetBedRock();
    void sig_noticeSetLava();


};

#endif // INVENTORY_H
