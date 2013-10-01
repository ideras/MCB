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

#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QLabel>
#include <string>
#include <sstream>
#include "gcode-int.h"
#include "MCBGenerator.h"
#include "DialogAutolevel.h"
#include "DialogGerber2GCode.h"

using namespace std;

extern stringstream out_err;

PCBMillingGenerator::PCBMillingGenerator(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
    statusLabel = new QLabel("");
    ui.statusBar->addWidget(statusLabel);
}

PCBMillingGenerator::~PCBMillingGenerator()
{
}

void PCBMillingGenerator::ListFileItemChanged(QListWidgetItem *item)
{
	QVariant v = item->data(Qt::UserRole);
	GCodeInt *ginter = (GCodeInt *)v.value<void *>();

	if (ginter == NULL)
		return;

	if (item->checkState() == Qt::Checked) {
		ui.renderArea->AddFileToPlot(ginter);
	} else {
		ui.renderArea->RemoveFileFromPlot(ginter);
	}
}

void PCBMillingGenerator::ListFileItemSelectionChanged()
{
    GCodeInt *ginter = GetSelectedListFileItem();

    if (ginter == NULL)
        return;

    QString filePath = QString::fromStdString(ginter->GetFilePath());
    statusLabel->setText(filePath);
}

void PCBMillingGenerator::ShowContextMenuForListFile(const QPoint &pos)
{
	if (ui.lstFile->selectedItems().size() == 0)
		return;

	QListWidgetItem *item = ui.lstFile->selectedItems().first();

	bool itemIsChecked = item->checkState() == Qt::Checked;

	GCodeInt *ginter = GetSelectedListFileItem();
	bool showPP = false;
	bool showDP = false;

	if (itemIsChecked) {
		showPP = ui.renderArea->GetShowProbePointsStatus(ginter);
		showDP = ui.renderArea->GetShowDrillSpotsStatus(ginter);
	}

	QMenu contextMenu(tr("List File Context Menu"), this);

	if (!ginter->HasProbePoints()) {
		QAction *actionAddPP = new QAction(tr("Add Autolevel GCODE"), this);
		connect(actionAddPP, SIGNAL(triggered()), this, SLOT(OnAddAutolevelGcodeTriggered()));

		contextMenu.addAction(actionAddPP);
	}

	if (itemIsChecked && ginter->HasProbePoints()) {
		QAction *actionShowPP = new QAction(tr("Probe Points"), this);
		actionShowPP->setCheckable(true);
		actionShowPP->setChecked(showPP);
		
		connect(actionShowPP, SIGNAL(triggered(bool)), this, SLOT(OnShowProbePointsTriggered(bool)));

		contextMenu.addAction(actionShowPP);
	}
	
	if (itemIsChecked) {
		QAction *actionShowDS = new QAction(tr("Drill Holes"), this);
		actionShowDS->setCheckable(true);
		actionShowDS->setChecked(showDP);
		connect(actionShowDS, SIGNAL(triggered(bool)), this, SLOT(OnShowDrillSpotsTriggered(bool)));

		contextMenu.addAction(actionShowDS);
	}

	contextMenu.addAction(ui.actionClose_File);

	contextMenu.exec(ui.lstFile->mapToGlobal(pos));
}

void PCBMillingGenerator::OnAddAutolevelGcodeTriggered()
{
    GCodeInt *ginter = GetSelectedListFileItem();

    DialogAutolevel *dlg = new DialogAutolevel(this, ginter);

    dlg->exec();
}

void PCBMillingGenerator::OnShowProbePointsTriggered(bool checked)
{
	if (ui.lstFile->selectedItems().isEmpty())
		return;

	GCodeInt *ginter = GetSelectedListFileItem();

	ui.renderArea->SetShowProbePoints(ginter, checked);		
}

void PCBMillingGenerator::OnShowDrillSpotsTriggered(bool checked)
{
	if (ui.lstFile->selectedItems().isEmpty())
		return;

	QListWidgetItem *item = ui.lstFile->selectedItems().first();
	QVariant v = item->data(Qt::UserRole);
	GCodeInt *ginter = (GCodeInt *)v.value<void *>();

	ui.renderArea->SetShowDrillSpots(ginter, checked);		
}

void PCBMillingGenerator::CloseSelectedFile()
{
	if (ui.lstFile->selectedItems().isEmpty())
		return;
	
	QModelIndexList selectedItems = ui.lstFile->selectionModel()->selectedIndexes();

	QListWidgetItem *item = ui.lstFile->selectedItems().first();
	QVariant v = item->data(Qt::UserRole);
	GCodeInt *ginter = (GCodeInt *)v.value<void *>();

	ui.renderArea->RemoveFileFromPlot(ginter);
	ui.lstFile->model()->removeRow(selectedItems.at(0).row());

	delete ginter;
}

void PCBMillingGenerator::LoadGCodeFile(QString filePath)
{
	string filp = filePath.toStdString();
	GCodeInt *ginter = new GCodeInt(filp);

	if ( !ginter->LoadFile() ) {
		string msg = out_err.str();
		
		QMessageBox::critical( this, "Error loading GCODE file", QString::fromStdString(msg) );
		delete ginter;

	} else {
		QFileInfo fileInfo(filePath);

		QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName(), ui.lstFile);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

		item->setData(Qt::ToolTipRole, filePath);
		QVariant v = qVariantFromValue((void *)ginter);
		item->setData(Qt::UserRole, v);
		 
		item->setCheckState(Qt::Checked);
		ui.lstFile->addItem(item);
	}
}

void PCBMillingGenerator::OpenGcodeFile()
{
	QFileDialog::Options options;
     QString selectedFilter;
     QString fileName = QFileDialog::getOpenFileName(this,
                                 tr("QFileDialog::getOpenFileName()"),
                                 "Open GCODE File",
                                 tr("GCode File (*.nc *.tap *.ngc);;All Files (*)"),
                                 &selectedFilter,
                                 options);
     if (!fileName.isEmpty())
		 LoadGCodeFile(fileName);
}

void PCBMillingGenerator::ShowGerberToGCodeDialog()
{
	DialogGerber2GCode *dlg = new DialogGerber2GCode(this);

	dlg->exec();
}
