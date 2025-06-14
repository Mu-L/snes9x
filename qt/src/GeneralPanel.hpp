#pragma once
#include "ui_GeneralPanel.h"
#include "EmuApplication.hpp"

class GeneralPanel :
    public Ui::GeneralPanel,
    public QWidget
{
  public:
    GeneralPanel(EmuApplication *app);
    void showEvent(QShowEvent *event) override;

    EmuApplication *app;
};