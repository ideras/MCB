#ifndef GCODE_IR_H
#define GCODE_IR_H
#include <string>
#include <map>
#include <list>
#include <vector>
#include <sstream>
#include "gcode-lexer.h"

using namespace std;

enum GExprKind { ADD_EXPR, SUB_EXPR, MUL_EXPR, DIV_EXPR, NUMBER_EXPR, VREF_EXPR };
enum GStmtKind { ASSIGN_STMT, COMMAND_STMT, SUBDECL_STMT, SUBCALL_STMT };
typedef long double Real;

//GCode Expression
class GExpr
{
public:
    virtual ~GExpr() { }
	virtual int GetKind() = 0;
	virtual GExpr *Clone() = 0;
	virtual string ToString() = 0;
    Real GetValue() { return value; }
    void SetValue(Real value) { this->value = value; }
protected:
    Real value;
};

class GBinaryExpr: public GExpr
{
public:
	GBinaryExpr(GExpr *expr1, GExpr *expr2) {
		this->expr1 = expr1;
		this->expr2 = expr2;
	}
	~GBinaryExpr() { 
		if (expr1 != 0)
			delete expr1;
		if (expr2 != 0)
			delete expr2; 
	}

	GExpr *GetLExpr() { return expr1; }
	GExpr *GetRExpr() { return expr2; }
	void SetLExpr(GExpr *expr) { expr1 = expr; }
	void SetRExpr(GExpr *expr) { expr2 = expr;}

protected:
	GExpr *expr1;
	GExpr *expr2;
};

class GAddExpr: public GBinaryExpr
{
public:
	GAddExpr(GExpr *expr1, GExpr *expr2): GBinaryExpr(expr1, expr2) {}

	int GetKind() { return ADD_EXPR; }
	GExpr *Clone()  { return new GAddExpr(expr1->Clone(), expr2->Clone()); }
	string ToString() { return "(" + expr1->ToString() + " + " + expr2->ToString() + ")"; }
};

class GSubExpr: public GBinaryExpr
{
public:
	GSubExpr(GExpr *expr1, GExpr *expr2): GBinaryExpr(expr1, expr2) {}

	int GetKind() { return SUB_EXPR; }
	GExpr *Clone()  { return new GSubExpr(expr1->Clone(), expr2->Clone()); }
	string ToString() { return "(" + expr1->ToString() + " - " + expr2->ToString() + ")"; }
};

class GMulExpr: public GBinaryExpr
{
public:
	GMulExpr(GExpr *expr1, GExpr *expr2): GBinaryExpr(expr1, expr2) {}

	int GetKind() { return MUL_EXPR; }
	GExpr *Clone()  { return new GMulExpr(expr1->Clone(), expr2->Clone()); }
	string ToString() { return "(" + expr1->ToString() + " * " + expr2->ToString() + ")"; }
};

class GDivExpr: public GBinaryExpr
{
public:
	GDivExpr(GExpr *expr1, GExpr *expr2): GBinaryExpr(expr1, expr2) {}

	int GetKind() { return DIV_EXPR; }
	GExpr *Clone()  { return new GDivExpr(expr1->Clone(), expr2->Clone()); }
	string ToString() { return "(" + expr1->ToString() + " / " + expr2->ToString() + ")"; }
};

class GNumberExpr:  public GExpr
{
public:
	GNumberExpr(long double value) { this->value = value; }

	int GetKind() { return NUMBER_EXPR; }
	GExpr *Clone()  { return new GNumberExpr(value); }

	string ToString() {
		stringstream ss;

		ss << value;
		return ss.str();
	}
};

class GVarRefExpr: public GExpr
{
public:
	GVarRefExpr(string var) { this->var = var; }

	int GetKind() { return VREF_EXPR; }
	string GetVarName() { return var; }
	GExpr *Clone()  { return new GVarRefExpr(var); }
	string ToString() { return var; }

private:
	string var;
};

//Gcode Stamement base class
class GCodeStmt
{
public:
    virtual ~GCodeStmt() { }
	virtual int GetKind() = 0;
    virtual GCodeStmt *Clone() = 0;
};

class GCodeAssign: public GCodeStmt
{
public:
	GCodeAssign(string var, GExpr *expr) { this->var = var; this->rvalue = expr; }
	~GCodeAssign() { delete rvalue; }
	string GetVariable() { return var; }
	GExpr *GetExpr() { return rvalue; }
	int GetKind( ) { return ASSIGN_STMT; }
    GCodeStmt *Clone() { return new GCodeAssign(var, rvalue->Clone()); }

private:
	string var;
	GExpr *rvalue;
};

class GCodeCommand: public GCodeStmt
{
public:
	GCodeCommand() {
        name = "";
        zformula = "";
        argNameList = "";
    }

    GCodeCommand(int commndID, Real x, Real y) {
        opcode = commndID;
        argNameList = "XY";
		arguments['X'] = new GNumberExpr(x);
        arguments['Y'] = new GNumberExpr(y);
        zformula = "";
    }

	~GCodeCommand() { FreeArguments(); }

	int GetKind() { return COMMAND_STMT; }

	int GetOpcode() { return opcode; }
	void SetOpcode(int opcode) { this->opcode = opcode; }
	string GetName() { return name; }
	void SetName(string name) { this->name = name; }
	bool IsA(int commandID) { return opcode == commandID; }
    bool IsMotionCommand() { return ((opcode != G82) && (opcode != G81)) && (HasArgument('X') || HasArgument('Y') || HasArgument('Z')); }

	void SetArgument(char argName, GExpr *expr) {
		argName = toupper(argName);

		if (arguments.find(argName) == arguments.end())
			argNameList += argName;

		arguments[argName] = expr;
	}

	bool HasArgument(char argName) { return arguments.find(toupper(argName)) != arguments.end(); }
	GExpr *GetArgument(char argName) { return arguments[toupper(argName)]; }
    
    void setZFormula(string &zformula) {
        if (!HasArgument('Z'))
            argNameList += "Z";
        
        this->zformula = zformula;
    }
    
    void Clear() {
        FreeArguments();
        name = "";
        zformula = "";
        argNameList = "";
    }
    
    void operator=(GCodeCommand &cmd) {
        this->Clear();
        this->opcode = cmd.opcode;
        this->name = cmd.name;
        this->argNameList = cmd.argNameList;
        this->zformula = cmd.zformula;
        
        for (unsigned int i = 0; i < cmd.argNameList.length(); i++ ) {
                char argName = cmd.argNameList[i];
                GExpr *value = cmd.arguments[argName]->Clone();
        
                arguments[argName] = value;
        }
    }
    
    GCodeStmt *Clone() {
        GCodeCommand *result = new GCodeCommand();

        *result = *this; //User operator =

        return result;
    }

    string ToString();

private:
	void FreeArguments() {
		map<char, GExpr *>::iterator it = arguments.begin();

		while (it != arguments.end()) {
			GExpr *expr = it->second;

			delete expr;
			it++;
		}
		arguments.clear();
	}

	int opcode;
	string name;
	string zformula;     //Interpolation Formula for Z Coordinate
	string argNameList;  //Argument Names
	map<char, GExpr *> arguments;
};

class GCodeSubCall: public GCodeStmt
{
public:
	GCodeSubCall() { name = ""; subId = 0; }
	GCodeSubCall(string name, int subId) { this->name = name; this->subId = subId; }

	~GCodeSubCall() {
		vector<GExpr *>::iterator it = arguments.begin();
		while (it != arguments.end() ) {
			GExpr *expr = *it;

			delete expr;
			
			it++;
		}
		arguments.clear();
	}

	int GetKind() { return SUBCALL_STMT; }
	int GetSubID() { return subId; }
	void SetSubID(int subId) { this->subId = subId; }
	string GetName() { return name; }
	void SetName(string name) { this->name = name; }
	void AddArgument(GExpr *arg) { arguments.push_back(arg); }
	GExpr *GetArgument(int index) { return arguments[index]; }
	int GetArgumentCount() { return arguments.size(); }

    GCodeStmt *Clone() {
        GCodeSubCall *result = new GCodeSubCall();
        result->subId = subId;
        result->name = name;

        vector<GExpr *>::iterator it = arguments.begin();
        while (it != arguments.end()) {
            GExpr *expr = *it;
            result->arguments.push_back(expr->Clone());
        }

        return result;
    }

private:
	int subId;
	string name;
	vector<GExpr *> arguments;
};

#endif
