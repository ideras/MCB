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
#include <QCoreApplication>
#include <QStringList>
#include <QProcess>
#include <QDebug>
#include "gcode-parser.h"
#include "DialogGerber2GCode.h"
#include "ui_DialogGerber2GCode.h"

DialogGerber2GCode::DialogGerber2GCode(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogGerber2GCode)
{
    ui->setupUi(this);

    currentUnits = UNIT_INCHES;
#ifdef Q_OS_WINDOWS
    QString path = QCoreApplication::applicationDirPath();
	ui->txtMessageLog->appendPlainText("Using pcb2gcode 1.1.4 from " + path + "/tools/");
#endif
}

DialogGerber2GCode::~DialogGerber2GCode()
{
    delete ui;
}


bool DialogGerber2GCode::setProcessArguments( QStringList &arguments )
{
	bool hasFiles = false;

	if (ui->gbFrontSide->isChecked())
	{
		QString frontSideInFilePath = QDir::toNativeSeparators(ui->txtFrontSideInFilePath->text().trimmed());
		QString frontSideOutFilePath = QDir::toNativeSeparators(ui->txtFrontSideOutFilePath->text().trimmed());

		if (frontSideInFilePath.isEmpty()) {
			QMessageBox::critical(this, "Error", "No front side input GERBER file selected. Aborting process !", QMessageBox::Ok);
			return false;
		}
		if (frontSideOutFilePath.isEmpty()) {
			QMessageBox::critical(this, "Error", "No front side output GCode file selected. Aborting process !", QMessageBox::Ok);
			return false;
		}

		arguments << "--front"  << frontSideInFilePath;
		arguments << "--front-output" << frontSideOutFilePath;

		hasFiles = true;
	}

	if (ui->gbBackSide->isChecked())
	{
		QString inFilePath =  QDir::toNativeSeparators(ui->txtBackSideInFilePath->text().trimmed());
		QString outFilePath =  QDir::toNativeSeparators(ui->txtBackSideOutFilePath->text().trimmed());

		if (inFilePath.isEmpty()) {
			QMessageBox::critical(this, "Error", "No back side input GERBER file selected. Aborting process !", QMessageBox::Ok);
			return false;
		}
		if (outFilePath.isEmpty()) {
			QMessageBox::critical(this, "Error", "No back side output GCode file selected. Aborting process !", QMessageBox::Ok);
			return false;
		}

		arguments << "--back" << inFilePath;
		arguments << "--back-output" << outFilePath;

		hasFiles = true;
	}
	
	if (ui->gbOutline->isChecked())
	{
		QString inFilePath = QDir::toNativeSeparators(ui->txtDrillInFilePath->text().trimmed());
		QString outFilePath = QDir::toNativeSeparators(ui->txtDrillOutFilePath->text().trimmed());

		if (inFilePath.isEmpty()) {
			QMessageBox::critical(this, "Error", "No outline input GERBER file selected. Aborting process !", QMessageBox::Ok);
			return false;
		}
		if (outFilePath.isEmpty()) {
			QMessageBox::critical(this, "Error", "No outline output GCode file selected. Aborting process !", QMessageBox::Ok);
			return false;
		}

		arguments << "--outline" << inFilePath;
		arguments << "--outline-output" << outFilePath;

		hasFiles = true;
	}

	if (ui->gbDrill->isChecked())
	{
		QString inFilePath = QDir::toNativeSeparators(ui->txtDrillInFilePath->text().trimmed());
		QString outFilePath = QDir::toNativeSeparators(ui->txtDrillOutFilePath->text().trimmed());

		if (inFilePath.isEmpty()) {
			QMessageBox::critical(this, "Error", "No drill input EXCELLON file selected. Aborting process !", QMessageBox::Ok);
			return false;
		}
		if (outFilePath.isEmpty()) {
			QMessageBox::critical(this, "Error", "No drill output GCode file selected. Aborting process !", QMessageBox::Ok);
			return false;
		}

		arguments << "--drill" << inFilePath;
		arguments << "--drill-output" << outFilePath;

		hasFiles = true;
	}

	if (!hasFiles) {
		QMessageBox::critical(this, "Error", "No input files to process. Aborting process !", QMessageBox::Ok);
		return false;
	}

	//Mill
	QString extraPasses = getParameter(ui->txtMillExtraPasses);

	arguments << "--zwork" << ui->txtMillEngravingDepth->text();
	arguments << "--zsafe" << ui->txtMillTraverseHeight->text();
	arguments << "--offset" << ui->txtMillOffset->text();
	arguments << "--mill-feed" << ui->txtMillFeed->text();
	arguments << "--mill-speed" << "30000";
	arguments << "--extra-passes" << extraPasses;

	//Outline
	arguments << "--zcut" << ui->txtOutlineCutDepth->text();
	arguments << "--cut-feed" << ui->txtOutlineCutFeed->text();
	arguments << "--cut-speed" << "20000";
	arguments << "--cut-infeed" << ui->txtOutlineCutInfeed->text();

	//Drill
	arguments << "--zdrill" << ui->txtDrillDepth->text();
	arguments << "--zchange" << ui->txtDrillToolChangeHeight->text();
	arguments << "--drill-feed" << ui->txtDrillFeed->text();
	arguments << "--drill-speed" << "20000";

	return true;
}

void DialogGerber2GCode::pbGenGCodeClicked()
{
	QString        programPath, tmpPath;
	QStringList    arguments;

#ifdef Q_OS_LINUX
    programPath = "/home/ideras/develop/pcb2gcode/pcb2gcode";
    tmpPath = "/tmp";
#else
	programPath = "\"" + QCoreApplication::applicationDirPath() + "/tools/pcb2gcode.exe\"";
    tmpPath = QCoreApplication::applicationDirPath() + "/tmp";
#endif

	if (!setProcessArguments(arguments))
		return;

	ui->pbGenGCode->setEnabled(false);

	myProcess = new QProcess(this);
	myProcess->setWorkingDirectory(tmpPath);

	connect (myProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(printOutput()));
	connect (myProcess, SIGNAL(readyReadStandardError()), this, SLOT(printError()));
	connect (myProcess, SIGNAL(started()), this, SLOT(processStarted()));
	connect (myProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processExit(int,QProcess::ExitStatus)));
	connect (myProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onProcessError(QProcess::ProcessError)) );

	myProcess->start(programPath, arguments);
    myProcess->waitForStarted();
}

void DialogGerber2GCode::printOutput()
{
	QByteArray byteArray = myProcess->readAllStandardOutput();
	QStringList strLines = QString(byteArray).split("\n");

	foreach (QString line, strLines){
		ui->txtMessageLog->appendPlainText(line.trimmed());
	}
}

void DialogGerber2GCode::printError()
{
	QByteArray byteArray = myProcess->readAllStandardError();
	QStringList strLines = QString(byteArray).split("\n");

	foreach (QString line, strLines){
		ui->txtMessageLog->appendPlainText(line);
	}
}

void DialogGerber2GCode::processStarted()
{
	ui->txtMessageLog->appendPlainText("Gerber to GCode conversion started ...");
}

void DialogGerber2GCode::processExit(int exitcode, QProcess::ExitStatus status)
{
	ui->txtMessageLog->appendPlainText("Gerber to GCode conversion DONE ...");
	ui->pbGenGCode->setEnabled(true);
}

void DialogGerber2GCode::onProcessError( QProcess::ProcessError error )
{
	QString errorText;

	switch (error) {
		case QProcess::FailedToStart: errorText = "The process failed to start. "
												  "Either the invoked program is missing, or you may have insufficient permissions"
												  " to invoke the program.";
									  break;
		case QProcess::Crashed:			errorText = "Error: Process crashed some time after starting successfully."; break;
		case QProcess::Timedout:		errorText = "Error: last waitFor...() function timed out. The state of QProcess is unchanged, and you can try calling waitFor...() again.";
		case QProcess::WriteError:		errorText = "An error occurred when attempting to write to the process. For example, the process may not be running, or it may have closed its input channel."; break;
		case QProcess::ReadError:		errorText = "Error occurred when attempting to read from the process. For example, the process may not be running."; break;
		case QProcess::UnknownError:	errorText= "Unknown error occurred. This is the default return value of error()."; break;
	}

	ui->txtMessageLog->appendPlainText(errorText);
	ui->pbGenGCode->setEnabled(true);
}

void DialogGerber2GCode::clearMessageLog()
{
	ui->txtMessageLog->clear();
}

void DialogGerber2GCode::browseInFilePath( QLineEdit *txtInPath, QLineEdit *txtOutPath, QString title, QString filter, QString dir, QString outSuffix )
{
	QFileDialog::Options options;
	QString selectedFilter;

	QString fileName = QFileDialog::getOpenFileName(this,
		title,
		dir,
		filter,
		&selectedFilter,
		options);

	if (!fileName.isEmpty()) {
		txtInPath->setText(fileName);

		if (txtOutPath->text().trimmed().isEmpty()) {
			QFileInfo fileInfo (txtInPath->text());
			QString outputFileName = fileInfo.completeBaseName() + outSuffix;
			QString outputFilePath = fileInfo.absoluteDir().absolutePath() + "/" + outputFileName;
			txtOutPath->setText(outputFilePath);
		}
	}
}

void DialogGerber2GCode::browseOutFilePath( QLineEdit *txtPath, QString title, QString dir )
{
	QFileDialog::Options options;
	QString selectedFilter;
	
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Choose output GCode File Path ..."),
		dir,
		tr("GCode File (*.nc *.tap *.ngc);;All Files (*)"),
		&selectedFilter,
		options);
	if (!fileName.isEmpty())
		txtPath->setText(fileName);

}

void DialogGerber2GCode::browseInFrontSide()
{
	browseInFilePath (	ui->txtFrontSideInFilePath, 
						ui->txtFrontSideOutFilePath,
						tr("Choose board front side GERBER file ..."),
						tr("Gerber files (*.gb0 *.gb1 *.gb *.gbr);;All Files (*)"),
						tr(""),
						tr(".front.mill.ngc")
					);
}

void DialogGerber2GCode::browseOutFrontSide()
{
	QString inPath = ui->txtFrontSideInFilePath->text().trimmed();
	if (inPath.isEmpty()) {
		QMessageBox::warning(this, "Warning", "Choose the board front side GERBER file first!", QMessageBox::Ok);
		return;
	}
	QFileInfo fileInfo (inPath);

	browseOutFilePath (	ui->txtFrontSideOutFilePath, 
						tr("Choose board front side output GCODE file ..."),
						fileInfo.absoluteDir().absolutePath()
						);
}

void DialogGerber2GCode::browseInBackSide()
{
	browseInFilePath (	ui->txtBackSideInFilePath, 
						ui->txtBackSideOutFilePath,
						tr("Choose board back side GERBER file ..."),
						tr("Gerber files (*.gb0 *.gb1 *.gb *.gbr);;All Files (*)"),
						tr(""),
						tr(".back.mill.ngc")
						);
}

void DialogGerber2GCode::browseOutBackSide()
{
	QString inPath = ui->txtBackSideInFilePath->text().trimmed();
	if (inPath.isEmpty()) {
		QMessageBox::warning(this, "Warning", "Choose the board back side GERBER file first!", QMessageBox::Ok);
		return;
	}
	QFileInfo fileInfo (inPath);

	browseOutFilePath (	ui->txtFrontSideOutFilePath, 
						tr("Choose board back side output GCODE file ..."),
						fileInfo.absoluteDir().absolutePath()
						);
}

void DialogGerber2GCode::browseInDrill()
{
	browseInFilePath (	ui->txtDrillInFilePath, 
						ui->txtDrillOutFilePath,
						tr("Choose EXCELLON Drill file ..."),
						tr("Excellon drill files (*.drl);;All Files (*)"),
						tr(""),
						tr(".drill.ngc")
						);
}

void DialogGerber2GCode::browseOutDrill()
{
	QString inPath = ui->txtDrillInFilePath->text().trimmed();
	if (inPath.isEmpty()) {
		QMessageBox::warning(this, "Warning", "Choose the board drill EXCELLON file first!", QMessageBox::Ok);
		return;
	}
	QFileInfo fileInfo (inPath);

	browseOutFilePath (	ui->txtDrillOutFilePath, 
						tr("Choose board drill output GCODE file ..."),
						fileInfo.absoluteDir().absolutePath()
						);
}

void DialogGerber2GCode::browseInOutline()
{
	browseInFilePath (	ui->txtOutlineInFilePath, 
						ui->txtOutlineOutFilePath,
						tr("Choose board outline polygon GERBER file ..."),
						tr("Gerber files (*.gb0 *.gb1 *.gb *.gbr);;All Files (*)"),
						tr(""),
						tr(".outline.ngc")
						);
}

void DialogGerber2GCode::browseOutOutline()
{
	QString inPath = ui->txtOutlineInFilePath->text().trimmed();
	if (inPath.isEmpty()) {
		QMessageBox::warning(this, "Warning", "Choose the board outline GERBER file first!", QMessageBox::Ok);
		return;
	}
	QFileInfo fileInfo (inPath);

	browseOutFilePath (	ui->txtOutlineOutFilePath, 
						tr("Choose board outline polygon output GCODE file ..."),
						fileInfo.absoluteDir().absolutePath()
					  );
}

void DialogGerber2GCode::changeUnitInches()
{
    if (currentUnits == UNIT_INCHES)
        return;

    currentUnits = UNIT_INCHES;

	//Mill
	ui->lblMillEngravingDepthUnit->setText("inches");
	ui->lblMillTraverseHeightUnit->setText("inches");
	ui->lblMillFeedUnit->setText("inches/minute");
	ui->lblMillOffsetUnit->setText("inches");

    double millEngravingDepth = ui->txtMillEngravingDepth->value();
    double millTraverseHeight = ui->txtMillTraverseHeight->value();
    double millFeed = ui->txtMillFeed->value();
    double millOffset = ui->txtMillOffset->value();

    ui->txtMillEngravingDepth->setValue(millEngravingDepth/25.4);
    ui->txtMillTraverseHeight->setValue(millTraverseHeight/25.4);
    ui->txtMillFeed->setValue(millFeed/25.4);
    ui->txtMillOffset->setValue(millOffset/25.4);

	//Drill
	ui->lblDrillDepthUnit->setText("inches");
	ui->lblDrillToolChangeHeightUnit->setText("inches");
	ui->lblDrillFeedUnit->setText("inches/minute");

    double drillDepth = ui->txtDrillDepth->value();
    double drillToolChangeHeight = ui->txtDrillToolChangeHeight->value();
    double drillFeed = ui->txtDrillFeed->value();

    ui->txtDrillDepth->setValue(drillDepth/25.4);
    ui->txtDrillToolChangeHeight->setValue(drillToolChangeHeight/25.4);
    ui->txtDrillFeed->setValue(drillFeed/25.4);

	//Outline
	ui->lblOutlineCutDepthUnit->setText("inches");
	ui->lblOutlineCutFeedUnit->setText("inches/minute");
    ui->lblOutlineCutInfeedUnit->setText("inches");

    double outlineCutDepth = ui->txtOutlineCutDepth->value();
    double outlineCutFeed = ui->txtOutlineCutFeed->value();
    double outlineCutInfeed = ui->txtOutlineCutInfeed->value();

    ui->txtOutlineCutDepth->setValue(outlineCutDepth/25.4);
    ui->txtOutlineCutFeed->setValue(outlineCutFeed/25.4);
    ui->txtOutlineCutInfeed->setValue(outlineCutInfeed/25.4);
}

void DialogGerber2GCode::changeUnitMM()
{
    if (currentUnits == UNIT_MM)
        return;

    currentUnits = UNIT_MM;

	//Mill
	ui->lblMillEngravingDepthUnit->setText("millimeters");
	ui->lblMillTraverseHeightUnit->setText("millimeters");
	ui->lblMillFeedUnit->setText("millimeters/minute");
	ui->lblMillOffsetUnit->setText("millimeters");

    double millEngravingDepth = ui->txtMillEngravingDepth->value();
    double millTraverseHeight = ui->txtMillTraverseHeight->value();
    double millFeed = ui->txtMillFeed->value();
    double millOffset = ui->txtMillOffset->value();

    ui->txtMillEngravingDepth->setValue(millEngravingDepth * 25.4);
    ui->txtMillTraverseHeight->setValue(millTraverseHeight * 25.4);
    ui->txtMillFeed->setValue(millFeed * 25.4);
    ui->txtMillOffset->setValue(millOffset * 25.4);

	//Drill
	ui->lblDrillDepthUnit->setText("millimeters");
	ui->lblDrillToolChangeHeightUnit->setText("millimeters");
	ui->lblDrillFeedUnit->setText("millimeters/minute");

    double drillDepth = ui->txtDrillDepth->value();
    double drillToolChangeHeight = ui->txtDrillToolChangeHeight->value();
    double drillFeed = ui->txtDrillFeed->value();

    ui->txtDrillDepth->setValue(drillDepth * 25.4);
    ui->txtDrillToolChangeHeight->setValue(drillToolChangeHeight * 25.4);
    ui->txtDrillFeed->setValue(drillFeed * 25.4);

	//Outline
	ui->lblOutlineCutDepthUnit->setText("millimeters");
	ui->lblOutlineCutFeedUnit->setText("millimeters/minute");
	ui->lblOutlineCutInfeedUnit->setText("millimeters");

    double outlineCutDepth = ui->txtOutlineCutDepth->value();
    double outlineCutFeed = ui->txtOutlineCutFeed->value();
    double outlineCutInfeed = ui->txtOutlineCutInfeed->value();

    ui->txtOutlineCutDepth->setValue(outlineCutDepth * 25.4);
    ui->txtOutlineCutFeed->setValue(outlineCutFeed * 25.4);
    ui->txtOutlineCutInfeed->setValue(outlineCutInfeed * 25.4);
}











