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
