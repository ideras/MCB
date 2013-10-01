#include <sstream>

#include "gcode-ir.h"

string GCodeCommand::ToString()
{
    stringstream ss;

    ss.precision(4);

    ss << name;

    for (unsigned int i = 0; i < argNameList.length(); i++) {
        char argName = argNameList[i];
        GExpr *value = arguments[argName];

        if (argName != 'Z' || zformula.empty()) {
            ss << " " << argName << fixed << value->ToString();
        } else {
            ss << " " << argName << "[" << zformula << "]";
        }
    }

    return ss.str();
}
