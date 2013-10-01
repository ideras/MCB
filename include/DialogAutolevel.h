#ifndef DIALOGAUTOLEVEL_H
#define DIALOGAUTOLEVEL_H

#include <QDialog>
#include "gcode-int.h"
#include "gcode-autoleveller.h"

namespace Ui {
class DialogAutolevel;
}

class DialogAutolevel : public QDialog, AutolevellerListener
{
    Q_OBJECT
    
public:
    explicit DialogAutolevel(QWidget *parent = 0);
    DialogAutolevel(QWidget *parent, GCodeInt *gi);
    ~DialogAutolevel();

    virtual void UpdateProgress(int value);

protected slots:
    void pbAutolevelClicked();
    void pbBrowseOutputFileClicked();

protected:

private:
    GCodeAutoleveller *gal;
    GCodeInt *ginter;
    AutolevellerInfo *m_AInfo;
    GCodeInfo *ginfo;

    Ui::DialogAutolevel *ui;
};

#endif // DIALOGAUTOLEVEL_H
