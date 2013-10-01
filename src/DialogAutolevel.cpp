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

#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include "DialogAutolevel.h"
#include "ui_DialogAutolevel.h"

DialogAutolevel::DialogAutolevel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAutolevel)
{
    ui->setupUi(this);
}

DialogAutolevel::DialogAutolevel(QWidget *parent, GCodeInt *gi):
    QDialog(parent),
    ui(new Ui::DialogAutolevel)
{
    ui->setupUi(this);

    this->ginter = gi;
    gal = new GCodeAutoleveller(gi);
    ginfo = ginter->GetGCodeInfo();
    m_AInfo = gal->GetAutolevellerInfo();

    ui->pb1->setRange(0, 100);
    ui->pb1->setValue(0);

	double boardWidth = ginfo->BoardMaxX - ginfo->BoardMinX;
	double boardHeight = ginfo->BoardMaxY - ginfo->BoardMinY;
	QString boardSize = QString::number(boardWidth) + " x " + QString::number(boardHeight);

    QString strIFilePath = QString::fromStdString(gi->GetFilePath());
    QFileInfo fileInfo(strIFilePath);
    QString outputFileName = fileInfo.completeBaseName() + ".probe.ngc";
    QString outputFilePath = fileInfo.absoluteDir().absolutePath() + "/" + outputFileName;

    ui->txtInputFile->setText(strIFilePath);
    ui->txtInputFile->setEnabled(false);
    ui->pbBrowseInputFile->setEnabled(false);

    ui->txtOutputFile->setText(outputFilePath);

    if (ginfo->UnitType == UNIT_INCHES) {
        ui->rbInches->setChecked(true);

        ui->txtGridSize->setValue(0.2);
        ui->txtEngDepth->setValue(ginfo->MillRouteDepth);
        ui->txtMaxDepth->setValue(-0.039);
        ui->txtTraverseHeight->setValue(0.02);
        ui->txtTraverseSpeed->setValue(15.7);
        ui->txtProbeZSpeed->setValue(2.4);

		ui->lblBoardSize->setText("Board Size: " + boardSize + " inches");
    }
    else {
        ui->rbMM->setChecked(true);

        ui->txtGridSize->setValue(5.0);
        ui->txtEngDepth->setValue(ginfo->MillRouteDepth);
        ui->txtMaxDepth->setValue(-1.0);
        ui->txtTraverseHeight->setValue(0.5);
        ui->txtTraverseSpeed->setValue(400);
        ui->txtProbeZSpeed->setValue(60);

        ui->lblGridSizeUnit->setText("mm");
        ui->lblMaxDepthUnit->setText("mm");
        ui->lblEngravingUnit->setText("mm");
        ui->lblProbeZSpeedUnit->setText("mm/minutes");
        ui->lblTravHeightUnit->setText("mm");
        ui->lblTravSpeedUnit->setText("mm/minutes");

		ui->lblBoardSize->setText("Board Size: " + boardSize + " mm");
    }
}

DialogAutolevel::~DialogAutolevel()
{
    delete ui;
    delete gal;
}

void DialogAutolevel::UpdateProgress(int value)
{
    ui->pb1->setValue(value);
}

void DialogAutolevel::pbAutolevelClicked()
{
     QString outFileName = ui->txtOutputFile->text();

     m_AInfo->GridSize = ui->txtGridSize->value();
     m_AInfo->TraverseHeight = ui->txtTraverseHeight->value();
     m_AInfo->ProbeMaxDepth = ui->txtMaxDepth->value();
     m_AInfo->TraverseSpeed = ui->txtTraverseSpeed->value();
     m_AInfo->ProbeSpeed = ui->txtProbeZSpeed->value();
     m_AInfo->EngravingDepth = ui->txtEngDepth->value();

     ui->lblProgress->setText("Processing Input File ...");
     ui->pb1->setMinimum(0);
     ui->pb1->setMaximum(ginter->GetStatementCount() - 1);

     gal->SplitSegments(this);

     ui->lblProgress->setText("Generating output file ...");
     ui->pb1->setMaximum(gal->GetOutputSize() - 1);
     ui->pb1->setValue(0);

     gal->GenerateAutolevellingGCode(outFileName.toStdString().c_str(), this);

     QMessageBox::information(this, "Information", "File Generated Successfully !!", QMessageBox::Ok);
}

void DialogAutolevel::pbBrowseOutputFileClicked()
{
    QFileDialog::Options options;
    QString selectedFilter;
    QFileInfo fileInfo(ui->txtInputFile->text());

    QString fileName = QFileDialog::getSaveFileName(this,
                                 tr("Choose output GCode File Path ..."),
                                 fileInfo.absoluteDir().absolutePath(),
                                 tr("GCode File (*.nc *.tap *.ngc);;All Files (*)"),
                                 &selectedFilter,
                                 options);
    if (!fileName.isEmpty())
        ui->txtOutputFile->setText(fileName);
}
