#include <cmath>
#include <limits>
#include <sstream>
#include <QFile>
#include <QTextStream>
#include "gcode-autoleveller.h"

using namespace std;

extern stringstream out_err;

#ifdef _MSC_VER
#define isinf(x) (!_finite(x))
#endif

GCodeAutoleveller::GCodeAutoleveller(GCodeInt *ginter) {
    m_ginter = ginter;
    m_GInfo = ginter->GetGCodeInfo();
    m_nextParamNumber = 2000;
    m_cellParams = 0;

    if (m_GInfo->UnitType == UNIT_INCHES) {
        m_AInfo.ClearHeight = 0.47244;
        m_AInfo.InitialProbeZ = -0.1969;
    }
    else {
        m_AInfo.ClearHeight = 12.0;
        m_AInfo.InitialProbeZ = -5.0;
    }
}

void GCodeAutoleveller::DistanceSplit(Real from_x, Real from_y, GCodeCommand *gcmd)
{
    Real to_x = gcmd->GetArgument('X')->GetValue();
    Real to_y = gcmd->GetArgument('Y')->GetValue();

    Real dist_x = to_x - from_x;
    Real dist_y = to_y - from_y;

    if (fabs(dist_x) > m_AInfo.Gx || fabs(dist_y) > m_AInfo.Gy) {
        Real mp_x = from_x + (dist_x / 2);
        Real mp_y = from_y + (dist_y / 2);

        GCodeCommand *c1 = new GCodeCommand(gcmd->GetOpcode(), mp_x, mp_y);
        GCodeCommand *c2 = new GCodeCommand(gcmd->GetOpcode(), to_x, to_y);

        c1->SetName(gcmd->GetName());
        c2->SetName(gcmd->GetName());
        if (gcmd->HasArgument('F'))
            c1->SetArgument('F', gcmd->GetArgument('F'));

        DistanceSplit(from_x, from_y, c1);
        DistanceSplit(mp_x, mp_y, c2);
    } else {
        /* Get the interpolated Z formula */
        string zformula = GetInterpolationFormula(to_x, to_y, true);
        gcmd->setZFormula(zformula);

        m_outStmtList.push_back(gcmd);
    }
}

string GCodeAutoleveller::GetInterpolationFormula(Real x, Real y, bool isLinearMotionCommand)
{
    int cellx, celly;

    GridRef(x, y, cellx, celly);

    Real os_x = ((x - m_AInfo.x1) - ((Real) cellx * m_AInfo.Gx)) / m_AInfo.Gx;
    Real os_y = ((y - m_AInfo.y1) - ((Real) celly * m_AInfo.Gy)) / m_AInfo.Gy;

    int px_cell = cellx + (os_x > 0.5 ? 1 : -1);
    int py_cell = celly + (os_y > 0.5 ? 1 : -1);

    if (px_cell < 0 || px_cell > m_AInfo.GridMaxX) {
        px_cell = cellx;
    }
    if (py_cell < 0 || py_cell > m_AInfo.GridMaxY) {
        py_cell = celly;
    }

    Real x_pc = 0.5 + (os_x > 0.5 ? 1 - os_x : os_x);
    Real y_pc = 0.5 + (os_y > 0.5 ? 1 - os_y : os_y);

    /*
     * Now we can make sure that each of our cells has a variable in it...
     */
    EnsureCellVariable(cellx, celly);
    EnsureCellVariable(px_cell, celly);
    EnsureCellVariable(cellx, py_cell);
    EnsureCellVariable(px_cell, py_cell);

    /*
     * Now we can work out the interpolation...
     */
    stringstream ss;
    string depthParameter = isLinearMotionCommand? "#3" : "#7";

    ss.precision(3);
    ss << fixed << (x_pc * y_pc) << "*#" << CellVariable(cellx, celly) << " + " <<
            ((1 - x_pc) * y_pc) << "*#" << CellVariable(px_cell, celly) << " + " <<
            (x_pc * (1 - y_pc)) << "*#" << CellVariable(cellx, py_cell) << " + " <<
            ((1 - x_pc) * (1 - y_pc)) << "*#" << CellVariable(px_cell, py_cell) << " + " <<
            depthParameter;

    return ss.str();
}

void GCodeAutoleveller::SplitSegments(AutolevellerListener *listener)
{
    if (!m_ginter->HasStatements())
        return;

    m_AInfo.HasDrillSpots = false;

    m_AInfo.DrillSpotDepth = -numeric_limits<Real>::infinity();

    m_AInfo.GridMaxX = (int)ceil((m_GInfo->BoardMaxX - m_GInfo->BoardMinX) / m_AInfo.GridSize);
    m_AInfo.GridMaxY = (int)ceil((m_GInfo->BoardMaxY - m_GInfo->BoardMinY) / m_AInfo.GridSize);

    m_AInfo.x1 = m_GInfo->BoardMinX - m_AInfo.GridSize / 2.0;
    m_AInfo.y1 = m_GInfo->BoardMinY - m_AInfo.GridSize / 2.0 ;
    m_AInfo.x2 = m_GInfo->BoardMaxX;
    m_AInfo.y2 = m_GInfo->BoardMaxY;

    //Lets adjust Grid Size on X and Y axis
    m_AInfo.Gx = (m_AInfo.x2 - m_AInfo.x1)/(m_AInfo.GridMaxX + 0.5);
    m_AInfo.Gy = (m_AInfo.y2 - m_AInfo.y1)/(m_AInfo.GridMaxY + 0.5);

    //Init Grid Cells Array
    int cellCount = (m_AInfo.GridMaxX + 1) * (m_AInfo.GridMaxY + 1);

    if (m_cellParams != 0)
        delete [] m_cellParams;

    m_cellParams = new int[cellCount];
    memset(m_cellParams, 0, cellCount * sizeof(int));

    m_ginter->Init();
    pos = m_ginter->GetCurrentPos();
    int count = 0;

    while (m_ginter->ExecuteNextStatement()) {
        GCodeCommand *cmd = m_ginter->GetCurrentCommand();

        if (listener != NULL)
            listener->UpdateProgress(count++);

        if (cmd == NULL)
            continue;

        if ( cmd->IsMotionCommand() ) {
            SplitIfNeeded((GCodeCommand *)cmd->Clone());

			pos = m_ginter->GetCurrentPos();
        } else if ( cmd->IsA( G82 ) ) {

            m_AInfo.HasDrillSpots = true;
            if (cmd->HasArgument('Z') && isinf(m_AInfo.DrillSpotDepth))
                m_AInfo.DrillSpotDepth = cmd->GetArgument('Z')->GetValue();

            GCodeCommand *icmd = (GCodeCommand *)cmd->Clone();

            string zformula = GetInterpolationFormula(pos.x, pos.y, false);
            icmd->setZFormula(zformula);

            m_outStmtList.push_back(icmd);
        } else
            m_outStmtList.push_back(cmd->Clone());
    }
}

void GCodeAutoleveller::GenerateAutolevellingGCode(const char *outfile_path, AutolevellerListener *listener)
{
    QFile outf(outfile_path);

    outf.open(QIODevice::WriteOnly);

    if ( !outf.isOpen() ) {
        out_err << "Unable to open file: " << outfile_path << endl;
        return;
    }
    QTextStream outs(&outf);

    list<GCodeStmt *>::iterator it = m_outStmtList.begin();
    int count = 0;

    while (it != m_outStmtList.end()) {
        GCodeCommand *cmd = (GCodeCommand *)(*it);

        if (listener != NULL)
            listener->UpdateProgress(count++);

        /*
         * We'll put our stuff right after the G21 or G20
         */
        if ( cmd->IsA(G20) || cmd->IsA(G21) ) {
            outs << cmd->ToString().c_str() << endl;
            outs << "\n"
                    "(Processed with pcb-probe by Ivan de Jesus Deras 2013 [Lee Essen, 2011] )"
                    "\n"
                    "\n"
                    "(Grid Cell Size = " << m_AInfo.GridSize << "mm )\n"
                    "(Grid Cell Count = " << (m_AInfo.GridMaxX + 1) << " x " << (m_AInfo.GridMaxY + 1) << " )\n"
                    "\n"
                    "\n"
                    "#1=" << m_AInfo.ClearHeight       << "			(clearance height)\n"
                    "#2=" << m_AInfo.TraverseHeight    << "			(traverse height)\n"
                    "#3=" << m_AInfo.EngravingDepth    << "            (engraving depth)\n"
                    "#4=" << m_AInfo.ProbeMaxDepth        << "			(probe maximum depth)\n"
                    "#5=" << m_AInfo.TraverseSpeed     << "			(traverse speed)\n"
                    "#6=" << m_AInfo.ProbeSpeed        << "			(probe speed)\n";

            if (m_AInfo.HasDrillSpots)
                outs << "#7=" << m_AInfo.DrillSpotDepth << "            (drill spot depth)\n";

            outs << endl << endl;
            outs <<  "M05			(stop motor)\n"
                    "(MSG,PROBE: Position to within 5mm [~0.2 inches] of surface & resume)\n"
                    "M60			(pause, wait for resume)\n"
                    "G49			(clear any tool offsets)\n"
                    "G92.1			(zero co-ordinate offsets)\n"
                    "G91			(use relative coordinates)\n"
                    "G38.2 Z" << m_AInfo.InitialProbeZ << " F[#6]	(probe to find worksurface)\n"
                    "G90			(back to absolute)\n"
                    "G92 Z0			(zero Z)\n"
                    "G00 Z[#1]		(safe height)\n"
                    "(MSG,PROBE: Z-Axis calibrate complete, beginning probe)\n"
                    "\n"
                    "(probe routine)\n"
                    "(params: x y traverse_height probe_depth traverse_speed probe_speed)\n"
                    "O100 sub\n"
                    "G00 X[#1] Y[#2] Z[#3] F[#5]\n"
                    "G38.2 Z[#4] F[#6]\n"
                    "G00 Z[#3]\n"
                    "O100 endsub\n"
                    "\n";

            /*
             * Now we can create the code for the depth sensing bit... but
             * we should do it in a fairly optimal way
             */

            int gx, gy, rgx;

            for (gy = 0; gy <= m_AInfo.GridMaxY; gy++) {
                for (rgx = 0; rgx <= m_AInfo.GridMaxX; rgx++) {
                    if (gy & 1) {
                        gx = m_AInfo.GridMaxX - rgx;
                    } else {
                        gx = rgx;
                    }

                    // Find the point in the centre of the grid square...
                    Real px = m_AInfo.x1 + ((Real) gx * m_AInfo.Gx) + (m_AInfo.Gx / 2);
                    Real py = m_AInfo.y1 + ((Real) gy * m_AInfo.Gy) + (m_AInfo.Gy / 2);

                    if (!CellHasVariable(gx, gy))
                        continue;

                    int var = CellVariable(gx, gy);

                    outs << "(PROBE[" << gx << "," << gy << "] " << ((double)px) << " " << ((double)py) << " -> " << var << ")" << endl;
                    outs << "O100 call [" << ((double)px) << "] [" << ((double)py) << "] [#2] [#4] [#5] [#6]" << endl;
                    outs << "#" << var << " = #5063" << endl;
                }
            }

            /*
             * Now before we go into the main mill bit we need to give you a chance
             * to undo the probe connections
             */
            outs << "\n\n"
                    "G00 Z[#1]		(safe height)\n"
                    "(MSG,PROBE: Probe complete, remove connections & resume)\n"
                    "M60			(pause, wait for resume)\n"
                    "(MSG,PROBE: Beginning etch)\n"
                    "\n\n";

        } else {
            outs << cmd->ToString().c_str() << endl;
        }

        it++;
    }
    outf.close();
}


