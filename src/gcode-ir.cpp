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
