#include <QWidget>
#include <QPainter>
#include <QPen>
#include <QMessageBox>
#include <QWheelEvent>
#include <QDebug>
#include <QFile>
#include <string>
#include <cmath>
#include <sstream>
#include "qrenderarea.h"

using namespace std;

extern stringstream out_err;

QRenderArea::QRenderArea(QWidget *parent)
	: QWidget(parent)
{
	m_originX = this->size().width()/2;
	m_originY = this->size().height()/2;
	m_scale = 1.0;
	dragStarted = false;
}

QRenderArea::~QRenderArea()
{
}

void QRenderArea::ZoomToFit()
{
	int w = this->size().width();
	int h = this->size().height();
	int spx = 10, epx = w - 10;
	int spy = 5, epy = h - 5;
	double scaleX, scaleY;
	double mill_dx, mill_dy;
	GCodeInfo &gi = *m_currPlot.ginfo;

    mill_dx = (gi.BoardMaxX - gi.BoardMinX);
    mill_dy = (gi.BoardMaxY - gi.BoardMinY);

	if (gi.UnitType == UNIT_INCHES) {
		mill_dx += 0.4;
		mill_dy += 0.4;
	} else if (gi.UnitType == UNIT_MM) {
		mill_dx += 10.0;
		mill_dy += 10.0;
	}

	scaleX = (epx - spx) / (mill_dx * m_currPlot.m_dpuX);
	scaleY = (epy - spy) / (mill_dy * m_currPlot.m_dpuY);
	m_scale = (double)qRound(qMin(scaleX, scaleY) * 10.0) / 10.0;
    m_originX = qRound(spx - gi.BoardMinX * m_currPlot.m_dpuX * m_scale);
    m_originY = qRound(spy + gi.BoardMaxY * m_currPlot.m_dpuY * m_scale);

	update();
}

GPlotterInfo &QRenderArea::GetPlotInfo(GCodeInt *ginter, int &index)
{
	index = -1;

	for (int i = 0; i < m_listPlot.size(); i++) {
		GPlotterInfo &gp = m_listPlot[i];

		if (gp.ginter == ginter) {
			index = i;
			return gp;
		}
	}
}

void QRenderArea::AddFileToPlot(GCodeInt *ginter)
{
	int dpiX = QWidget::physicalDpiX();
	int dpiY = QWidget::physicalDpiY();
	GPlotterInfo gp;

	gp.ginter = ginter;
	gp.ginfo = ginter->GetGCodeInfo();
	gp.showProbePoints = true;
	gp.showDrillSpots = true;

	if (gp.ginfo->UnitType == UNIT_INCHES) {
		gp.m_dpuX = (double)dpiX;
		gp.m_dpuY = (double)dpiY;
	} else if (gp.ginfo->UnitType == UNIT_MM) {
		gp.m_dpuX = dpiX / 25.4;
		gp.m_dpuY = dpiY / 25.4;
	}

	m_listPlot.append(gp);
	m_currPlot = gp;
    ZoomToFit();
}

void QRenderArea::RemoveFileFromPlot(GCodeInt *ginter)
{
	int index;
	GPlotterInfo &gp = GetPlotInfo(ginter, index);

	if (index == -1)
		return;

	m_listPlot.removeAt(index);

	if (!m_listPlot.isEmpty())
		m_currPlot = m_listPlot.last();

	update();
}

void QRenderArea::mousePressEvent(QMouseEvent *e)
{
	qDebug() << "mousePress" << e->button() << ":" << e->pos();

	if (e->button() == Qt::MidButton) {
		dragStarted = true;
		dragStartPoint = e->pos();
	}
}

void QRenderArea::mouseMoveEvent(QMouseEvent *e)
{
	if (dragStarted) {
		qDebug() << "mouseMove" << e->button() << " " << e->pos();
	}
}

void QRenderArea::mouseReleaseEvent(QMouseEvent *e)
{
	QPoint dragEndPoint = e->pos();
	int dx, dy;
	
	if (!dragStarted)
		return;

	dragStarted = false;
	dx = dragEndPoint.x() - dragStartPoint.x();
	dy = dragEndPoint.y() - dragStartPoint.y();

	qDebug() << "mouseRelease" << e->button() << " " << e->pos() << " " << dx << ":" << dy;

	if (dx != 0 || dy != 0) {
		m_originX += dx;
		m_originY += dy;
		update();
	}
}
 
void QRenderArea::wheelEvent(QWheelEvent *e)
{
	QPoint pt = e->pos();
	double s_dpuX = m_currPlot.m_dpuX * m_scale;
	double s_dpuY = m_currPlot.m_dpuY * m_scale;
	Real x_coord, y_coord;

	x_coord = (pt.x() - m_originX)/s_dpuX;
	y_coord = (m_originY - pt.y())/s_dpuY;

	m_scale += e->delta() / qreal(600);
	m_scale = qMax(qreal(0.1), qMin(qreal(32), m_scale));

	s_dpuX = m_currPlot.m_dpuX * m_scale;
	s_dpuY = m_currPlot.m_dpuY * m_scale;
	
	int x2, y2, dx, dy;

	x2 = qRound(x_coord * s_dpuX) + m_originX;
	y2 = m_originY - qRound(y_coord * s_dpuY);

	dx = x2 - pt.x();
	dy = y2 - pt.y();

	m_originX -= dx;
	m_originY -= dy;

	 update();
}

void QRenderArea::PlotGCode(QPainter &painter, GPlotterInfo &gp)
{
	bool doPlot = false;
	Position pos1, pos2;
	double s_dpuX = gp.m_dpuX * m_scale;
	double s_dpuY = gp.m_dpuY * m_scale;
	GCodeInt *gint = gp.ginter;

	//painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::white);

	pos2.z = 2.54; //Z safe
	if (gint->HasStatements()) {

		gint->Init();
		int count = 0;
		while (gint->ExecuteNextStatement()) {
			int x1, y1, x2, y2;
			GCodeCommand *cmd = gint->GetCurrentCommand();
				
			if (cmd == NULL)
				continue;

			if ( cmd->IsMotionCommand() ) {
				pos2 = gint->GetCurrentPos();

				if (pos2.z >= 0) doPlot = false;

				if (doPlot) {
					x1 = qRound(pos1.x * s_dpuX) + m_originX;
					y1 = m_originY - qRound(pos1.y * s_dpuY);
					x2 = qRound(pos2.x * s_dpuX) + m_originX;
					y2 = m_originY - qRound(pos2.y * s_dpuY);

                    /*switch (count) {
						case 0: painter.setPen(Qt::white); break;
						case 1: painter.setPen(Qt::magenta); break;
						case 2: painter.setPen(Qt::green); break;
						case 3: painter.setPen(Qt::red); break;
                    }*/

					painter.drawLine(x1, y1, x2, y2);
					pos1 = pos2;

					count = (count + 1) & 0x03;

				} else if (pos2.z < 0.0) {
					pos1 = pos2;
					doPlot = true;
				}
            } else if (gp.showDrillSpots && ( cmd->IsA( G82 ) || cmd->IsA( G81 ) )) {
				pos2 = gint->GetCurrentPos();

				x1 = qRound(pos2.x * s_dpuX) + m_originX;
				y1 = m_originY - qRound(pos2.y * s_dpuY);

				painter.fillRect(x1-1, y1-1, 3, 3, Qt::green);
			}
		} 

		if (gp.showProbePoints) {
			list<Position> *probePoints = gint->GetProbePoints();
			list<Position>::iterator it = probePoints->begin();

			while (it != probePoints->end()) {
				int x1, y1;
				Position p = *it;

				x1 = qRound(p.x * s_dpuX) + m_originX;
				y1 = m_originY - qRound(p.y * s_dpuY);

				painter.fillRect(x1-1, y1-1, 3, 3, Qt::red);

				/*painter.save();
				painter.setBrush(Qt::red);
				painter.setPen(Qt::red);
				painter.drawEllipse(x1-2, y1-2, 4, 4);
				painter.restore();*/

				it++;
			}
		}

	}
}

void QRenderArea::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);
	int w = this->size().width();
	int h = this->size().height();

	painter.fillRect(0, 0, w, h, Qt::black);
	painter.setPen(QPen(QColor(0, 128, 255), 1, Qt::DotLine));
	painter.drawLine(m_originX, 0, m_originX, h);
	painter.drawLine(0, m_originY, w, m_originY);

	QListIterator<GPlotterInfo> it(m_listPlot);

    while ( it.hasNext() ) {
		GPlotterInfo gp = it.next();
		PlotGCode(painter, gp);
	}
}

void QRenderArea::resizeEvent(QResizeEvent *e)
{
 
}
