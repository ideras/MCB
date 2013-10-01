#ifndef GCODEAUTOLEVELLER_H
#define GCODEAUTOLEVELLER_H

#include <QMap>
#include <QDebug>
#include <map>
#include <cmath>
#include <string>
#include <list>
#include "gcode-lexer.h"
#include "gcode-int.h"
#include "gcode-ir.h"

using namespace std;

struct AutolevellerInfo
{
    double GridSize;
    double Gx, Gy; //Adjusted GridSize on X and Y axes
    double SplitOver;

    //Probing Area
    Real x1;
    Real y1;
    Real x2;
    Real y2;

    //Route Depth
    double EngravingDepth;

    //Drill Spots Fields
    bool HasDrillSpots;
    double DrillSpotDepth;

    //Number of cells in Grid
    int GridMaxX;
    int GridMaxY;

    //Configurable output parameters
    double ClearHeight;
    double TraverseHeight;    //Traverse height
    double ProbeMaxDepth;        //Probe max depth, stop at this position if not triggered
    double InitialProbeZ;     // Initial probe Z position to find work surface
    double TraverseSpeed;     // Traverse Speed in Units (inches or mm) per Minute
    double ProbeSpeed;        //Probe Speed Units (inches or mm) per Minute
};

class AutolevellerListener {

public:
    virtual void UpdateProgress(int value) = 0;
};

class GCodeAutoleveller
{
public:
    GCodeAutoleveller(GCodeInt *ginter);

    ~GCodeAutoleveller() {

        if (m_cellParams != 0)
            delete [] m_cellParams;
    }

    void SplitSegments(AutolevellerListener *listener = NULL);
    void GenerateAutolevellingGCode(const char *outfile_path, AutolevellerListener *listener = NULL);
    int GetOutputSize() { return m_outStmtList.size(); }

    AutolevellerInfo *GetAutolevellerInfo() { return &m_AInfo; }

private:
    string GetInterpolationFormula(Real x, Real y, bool isLinearMotionCommand);
    void DistanceSplit(Real from_x, Real from_y, GCodeCommand *gcmd);

    void SplitIfNeeded(GCodeCommand *gcmd) {
        if (pos.z >= 0 || !gcmd->HasArgument('X') || !gcmd->HasArgument('Y')) {
            m_outStmtList.push_back(gcmd);
        } else {

            /*
             * We now have a start and end position, we can call our recursive routine...
             */
            DistanceSplit(pos.x, pos.y, gcmd);
        }
    }

    /*
     * This makes sure we have a variable assigned to a given cell
     */
    void EnsureCellVariable(int gx, int gy)
    {
        int index = gx + gy * (m_AInfo.GridMaxX + 1);

        if (m_cellParams[index] == 0) {

            m_cellParams[index] = m_nextParamNumber++;
        }
    }

    /*
     * This functions returns true if a cell has a variable (gcode parameter) associated,
     * false otherwise
     */
    bool CellHasVariable(int gx, int gy) {
        int index = gx + gy *(m_AInfo.GridMaxX + 1);

        return m_cellParams[index] != 0;
    }

    /*
     * This functions returns the variable (gcode parameter) associated with a cell.
     */
    int CellVariable(int gx, int gy) {
        int index = gx + gy * (m_AInfo.GridMaxX + 1);

        return m_cellParams[index];
    }

    /*
     * Given a co-ordinate we can work out a grid x and y number
     * so we can lookup stuff
     */
    void GridRef(Real x, Real y, int &ref_x, int &ref_y) {

        Real zero_x = x - m_AInfo.x1;
        Real zero_y = y - m_AInfo.y1;

        ref_x = (int)floor(zero_x / m_AInfo.Gx);
        ref_y = (int)floor(zero_y / m_AInfo.Gy);
    }

    int *m_cellParams; //GCode parameters associated with every cell in the Grid
    int m_nextParamNumber;
    GCodeInt *m_ginter;
    list<GCodeStmt *> *m_inStmtList;
    list<GCodeStmt *>  m_outStmtList;
    GCodeInfo *m_GInfo;
    AutolevellerInfo m_AInfo;
    Position pos;
};

#endif // GCODEAUTOLEVELLER_H
