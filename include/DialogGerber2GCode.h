#ifndef DIALOGGERBER2GCODE_H
#define DIALOGGERBER2GCODE_H

#include <QDialog>
#include <QProcess>
#include <QLineEdit>
#include <QAbstractSpinBox>

namespace Ui {
class DialogGerber2GCode;
}

class DialogGerber2GCode : public QDialog
{
    Q_OBJECT
    
public:
    explicit DialogGerber2GCode(QWidget *parent = 0);
    ~DialogGerber2GCode();

protected:
    void browseInFilePath(QLineEdit *txtInPath, QLineEdit *txtOutPath, QString title, QString filter, QString dir, QString outSuffix);
    void browseOutFilePath(QLineEdit *txtPath, QString title, QString dir);
	bool setProcessArguments(QStringList &arguments);

	QString getParameter(QAbstractSpinBox *txtParam) {
		QString result = txtParam->text();
		if (result.isEmpty())
			result = "0";

		return result;
	}

protected slots:
	void browseInFrontSide();
	void browseOutFrontSide();
	void browseInBackSide();
	void browseOutBackSide();
	void browseInDrill();
	void browseOutDrill();
	void browseInOutline();
	void browseOutOutline();
	void clearMessageLog();
	void pbGenGCodeClicked();
	void printOutput();
	void printError();
	void processStarted();
	void processExit(int exitcode, QProcess::ExitStatus status);
	void onProcessError(QProcess::ProcessError error);

	void changeUnitMM();
	void changeUnitInches();

private:
    Ui::DialogGerber2GCode *ui;
	QProcess *myProcess;
    int currentUnits;
};

#endif // DIALOGGERBER2GCODE_H
