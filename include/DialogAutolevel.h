/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
