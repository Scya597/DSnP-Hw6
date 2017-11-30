/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <sstream>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine constant (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
  // Open the aag file
  ifstream file(fileName.c_str());
	if (!file.is_open())	return false;
	string line;
	vector<string> circuit;

  while(!file.eof()) {
    getline(file,line,'\n');
    if (line != "") circuit.push_back(line);
  }
	if (circuit.empty())	return false;

  // Parse the aag file
  vector<string> header;
  if (!lexAig(circuit[0], header)) {
    return false;
  }

  int maxId = atof(header[1].c_str());
  int piLength = atof(header[2].c_str());
  int poLength = atof(header[4].c_str());
  int aigLength = atof(header[5].c_str());

  _pi.resize(piLength);
  _po.resize(poLength);
  _aig.resize(aigLength);

  _map[0] = new CirConstGate();

  for (size_t i = 0; i < piLength; i++) {
    _pi[i] = new CirPiGate(atof(circuit[i+1].c_str())/2, i+2);
    string name;
    stringstream stream;
    stream << atof(circuit[i+1].c_str())/2;
    stream >> name;
    name += "GAT";
    _pi[i]->setName(name);
    _map[atof(circuit[i+1].c_str())/2] = _pi[i];
  }

  for (size_t i = 0; i < aigLength; i++) {
    vector<string> newAig;
    if (!lexAig(circuit[i+piLength+poLength+1], newAig)) {
      return false;
    }
    _aig[i] = new CirAigGate(atof(newAig[0].c_str())/2, i+piLength+poLength+2);
    _map[atof(newAig[0].c_str())/2] = _aig[i];
  }

  for (size_t i = 0; i < poLength; i++) {
    _po[i] = new CirPoGate(maxId+i+1, i+piLength+2);
    string name;
    stringstream stream;
    if (int(atof(circuit[i+piLength+1].c_str())) % 2 == 0) {
      stream << atof(circuit[i+piLength+1].c_str())/2;
    } else {
      stream << (atof(circuit[i+piLength+1].c_str())+1)/2;
    }
    stream >> name;
    name += "GAT";
    _po[i]->setName(name);
    _map[maxId+i+1] = _po[i];
  }

  // handle AIG_GATE fanin and fanout
  for (size_t i = 0; i < aigLength; i++) {
    vector<string> newAig;
    if (!lexAig(circuit[i+piLength+poLength+1], newAig)) {
      return false;
    }

    for (size_t index = 1; index < 3; index++) {
      if (int(atof(newAig[index].c_str())) % 2 == 0) {
        _aig[i]->setBool(true);
        map<unsigned, CirGate*>::iterator it = _map.find(atof(newAig[index].c_str())/2);
        if (it == _map.end()) {
          _map[atof(newAig[index].c_str())/2] = new CirUndefGate(atof(newAig[index].c_str())/2);
        }
        _aig[i]->setFanin(_map[atof(newAig[index].c_str())/2]);
        _map[atof(newAig[index].c_str())/2]->setFanout(_aig[i]);
      } else {
        _aig[i]->setBool(false);
        map<unsigned, CirGate*>::iterator it = _map.find((atof(newAig[index].c_str())-1)/2);
        if (it == _map.end()) {
          _map[(atof(newAig[index].c_str())-1)/2] = new CirUndefGate((atof(newAig[index].c_str())-1)/2);
        }
        _aig[i]->setFanin(_map[(atof(newAig[index].c_str())-1)/2]);
        _map[(atof(newAig[index].c_str())-1)/2]->setFanout(_aig[i]);
      }
    }
  }

  // handle PO_GATE fanin
  for (size_t i = 0; i < poLength; i++) {
    if (int(atof(circuit[i+piLength+1].c_str())) % 2 == 0) {
      _map[maxId+i+1]->setBool(true);
      map<unsigned, CirGate*>::iterator it = _map.find(atof(circuit[i+piLength+1].c_str())/2);
      if (it == _map.end()) {
        _map[atof(circuit[i+piLength+1].c_str())/2] = new CirUndefGate(atof(circuit[i+piLength+1].c_str())/2);
      }
      _map[maxId+i+1]->setFanin(_map[atof(circuit[i+piLength+1].c_str())/2]);
      _map[atof(circuit[i+piLength+1].c_str())/2]->setFanout(_map[maxId+i+1]);
    } else {
      _map[maxId+i+1]->setBool(false);
      map<unsigned, CirGate*>::iterator it = _map.find((atof(circuit[i+piLength+1].c_str())+1)/2);
      if (it == _map.end()) {
        _map[(atof(circuit[i+piLength+1].c_str())+1)/2] = new CirUndefGate((atof(circuit[i+piLength+1].c_str())+1)/2);
      }
      _map[maxId+i+1]->setFanin(_map[(atof(circuit[i+piLength+1].c_str())+1)/2]);
      _map[(atof(circuit[i+piLength+1].c_str())+1)/2]->setFanout(_map[maxId+i+1]);
    }
  }
  // for (size_t i = 0; i < piLength; i++) {
  //   cout << _pi[i]->_lineNo << "   " << _pi[i]->_id << '\n';
  // }
  // for (size_t i = 0; i < poLength; i++) {
  //   cout << _po[i]->_lineNo << "   " << _po[i]->_id << '\n';
  // }
  // for (size_t i = 0; i < aigLength; i++) {
  //   cout << _aig[i]->_lineNo << "   " << _aig[i]->_id << '\n';
  // }
  // cout << _pi.size() << " " << _po.size();

  return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
}

void
CirMgr::printNetlist() const
{
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
}

void
CirMgr::writeAag(ostream& outfile) const
{
}


bool
CirMgr::lexAig(const string& option, vector<string>& tokens) const
{
   string token;
   size_t n = myStrGetTok(option, token);
   while (token.size()) {
      tokens.push_back(token);
      n = myStrGetTok(option, token, n);
   }
   return true;
}
