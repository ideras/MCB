#ifndef PCBMILLINGGENERATOR_H
#define PCBMILLINGGENERATOR_H

#include <QtGui/QMainWindow>
#include <QLabel>
#include "ui_MCBGenerator.h"

class PCBMillingGenerator : public QMainWindow
{
	Q_OBJECT

public:
	PCBMillingGenerator(QWidget *parent = 0, Qt::WFlags flags = 0);
	~PCBMillingGenerator();

public slots:
	void OpenGcodeFile();
	void ShowContextMenuForListFile(const QPoint &pos);
	void ListFileItemChanged(QListWidgetItem *item);
    void ListFileItemSelectionChanged();
	void CloseSelectedFile();
	void OnShowProbePointsTriggered(bool checked);
	void OnShowDrillSpotsTriggered(bool checked);
	void OnAddAutolevelGcodeTriggered();
	void ShowGerberToGCodeDialog();

private:
	void LoadGCodeFile(QString filePath);

	GCodeInt *GetSelectedListFileItem()
	{
        if (ui.lstFile->count() == 0)
            return NULL;

        if (ui.lstFile->selectedItems().size() == 0)
            return NULL;

		QListWidgetItem *item = ui.lstFile->selectedItems().first();
		QVariant v = item->data(Qt::UserRole);

		return ((GCodeInt *)v.value<void *>());
	}

	Ui::PCBMillingGeneratorClass ui;
    QLabel *statusLabel;
};

#endif // PCBMILLINGGENERATOR_H
