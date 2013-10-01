#ifndef QRENDERAREA_H
#define QRENDERAREA_H

#include <QWidget>
#include <QList>
#include <list>
#include "gcode-int.h"

struct GPlotterInfo
{
	GCodeInfo *ginfo; //Board Info
	GCodeInt *ginter; //GCode Interpreter
	double m_dpuX; // Dots per UNIT on X axis
	double m_dpuY; // Dots per UNIT on Y axis
	bool showProbePoints; 
	bool showDrillSpots; 
};

class QRenderArea : public QWidget
{
	Q_OBJECT

public:
	QRenderArea(QWidget *parent);
	~QRenderArea();
	void AddFileToPlot(GCodeInt *ginter);
	void RemoveFileFromPlot(GCodeInt *ginter);

	void SetShowProbePoints(GCodeInt *ginter, bool showProbePoints) {
		int index;
		GPlotterInfo &gp = GetPlotInfo(ginter, index);

		gp.showProbePoints = showProbePoints;
		update();
	}

	void SetShowDrillSpots(GCodeInt *ginter, bool showDrillSpots) {
		int index;
		GPlotterInfo &gp = GetPlotInfo(ginter, index);

		gp.showDrillSpots = showDrillSpots;
		update();
	}

	bool GetShowProbePointsStatus(GCodeInt *ginter) {
		int index;
		GPlotterInfo &gp = GetPlotInfo(ginter, index);

		return gp.showProbePoints;
	}

	bool GetShowDrillSpotsStatus(GCodeInt *ginter) {
		int index;
		GPlotterInfo &gp = GetPlotInfo(ginter, index);

		return gp.showDrillSpots;
	}

protected:
	void paintEvent(QPaintEvent *e);
	void resizeEvent(QResizeEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent *e);
	void PlotGCode(QPainter &painter, GPlotterInfo &gp);
	GPlotterInfo &GetPlotInfo(GCodeInt *ginter, int &index);

public slots:
		void ZoomToFit();

private:
	bool dragStarted;
	QPoint dragStartPoint;
	double m_scale;
	int m_originX, m_originY;
	QList<GPlotterInfo> m_listPlot; //File list to plot in the render area
	GPlotterInfo m_currPlot;
};

#endif // QRENDERAREA_H
