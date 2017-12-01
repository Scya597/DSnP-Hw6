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
#include <algorithm>
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
  // Open the aag file, parse the content and store each line string into circuit
  ifstream file(fileName.c_str());
	if (!file.is_open())	return false;
	string line;
	vector<string> circuit;

  while(!file.eof()) {
    getline(file,line,'\n');
    if (line != "") circuit.push_back(line);
  }
	if (circuit.empty())	return false;

  // Parse the header and init _pi _po _aig
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

  // add CONST_GATE
  _map[0] = new CirConstGate();

  // add PI_GATE
  for (size_t i = 0; i < piLength; i++) {
    _pi[i] = new CirPiGate(atof(circuit[i+1].c_str())/2, i+2);
    // string name;
    // stringstream stream;
    // stream << atof(circuit[i+1].c_str())/2;
    // stream >> name;
    // name += "GAT";
    // _pi[i]->setName(name);
    _map[atof(circuit[i+1].c_str())/2] = _pi[i];
  }

  // add AIG_GATE
  for (size_t i = 0; i < aigLength; i++) {
    vector<string> newAig;
    if (!lexAig(circuit[i+piLength+poLength+1], newAig)) {
      return false;
    }
    _aig[i] = new CirAigGate(atof(newAig[0].c_str())/2, i+piLength+poLength+2);
    _map[atof(newAig[0].c_str())/2] = _aig[i];
  }

  // add PO_GATE
  for (size_t i = 0; i < poLength; i++) {
    _po[i] = new CirPoGate(maxId+i+1, i+piLength+2);
    // string name;
    // stringstream stream;
    // if (int(atof(circuit[i+piLength+1].c_str())) % 2 == 0) {
    //   stream << atof(circuit[i+piLength+1].c_str())/2;
    // } else {
    //   stream << (atof(circuit[i+piLength+1].c_str())+1)/2;
    // }
    // stream >> name;
    // name += "GAT$PO";
    // _po[i]->setName(name);
    _map[maxId+i+1] = _po[i];
  }

  // handle AIG_GATE fanin and fanout
  for (size_t i = 0; i < aigLength; i++) {
    vector<string> newAig;
    if (!lexAig(circuit[i+piLength+poLength+1], newAig)) {
      return false;
    }

    for (size_t index = 1; index < 3; index++) {
      // check inverted
      if (int(atof(newAig[index].c_str())) % 2 == 0) {
        _aig[i]->setBool(false);
        map<unsigned, CirGate*>::iterator it = _map.find(atof(newAig[index].c_str())/2);
        // check floating
        if (it == _map.end()) {
          _map[atof(newAig[index].c_str())/2] = new CirUndefGate(atof(newAig[index].c_str())/2);
        }
        _aig[i]->setFanin(_map[atof(newAig[index].c_str())/2]);
        _map[atof(newAig[index].c_str())/2]->setFanout(_aig[i]);
      } else {
        _aig[i]->setBool(true);
        map<unsigned, CirGate*>::iterator it = _map.find((atof(newAig[index].c_str())-1)/2);
        // check floating
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
      _map[maxId+i+1]->setBool(false);
      map<unsigned, CirGate*>::iterator it = _map.find(atof(circuit[i+piLength+1].c_str())/2);
      if (it == _map.end()) {
        _map[atof(circuit[i+piLength+1].c_str())/2] = new CirUndefGate(atof(circuit[i+piLength+1].c_str())/2);
      }
      _map[maxId+i+1]->setFanin(_map[atof(circuit[i+piLength+1].c_str())/2]);
      _map[atof(circuit[i+piLength+1].c_str())/2]->setFanout(_map[maxId+i+1]);
    } else {
      _map[maxId+i+1]->setBool(true);
      map<unsigned, CirGate*>::iterator it = _map.find((atof(circuit[i+piLength+1].c_str())-1)/2);
      if (it == _map.end()) {
        _map[(atof(circuit[i+piLength+1].c_str())+1)/2] = new CirUndefGate((atof(circuit[i+piLength+1].c_str())-1)/2);
      }
      _map[maxId+i+1]->setFanin(_map[(atof(circuit[i+piLength+1].c_str())-1)/2]);
      _map[(atof(circuit[i+piLength+1].c_str())-1)/2]->setFanout(_map[maxId+i+1]);
    }
  }
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
  unsigned int sum = _pi.size() + _po.size() + _aig.size();
  cout << "Circuit Statistics" << endl;
  cout << "==================" << endl;
  cout << "  PI    " << setw(8) << right << _pi.size() << endl;
  cout << "  PO    " << setw(8) << right << _po.size() << endl;
  cout << "  AIG   " << setw(8) << right << _aig.size() << endl;
  cout << "------------------" << endl;
  cout << "  Total " << setw(8) << right << sum << endl;
}

void
CirMgr::printNetlist() const
{
  GateList dfsTl;

  CirGate::setGlobalRef();
  for (size_t i = 0; i < _po.size(); i++) {
    _po[i]->dfsTraversal(dfsTl);
  }

  cout << endl;
  unsigned undefNum = 0;
  for (size_t i = 0; i < dfsTl.size(); i++) {
    if (dfsTl[i]->_type == PI_GATE) {
      cout << "[" << i-undefNum << "] "<< "PI  "<< dfsTl[i]->_id //<< " (" << dfsTl[i]->_name << ")"
      << endl;
    } else if (dfsTl[i]->_type == PO_GATE) {
      string invert = "";
      string floating = "";
      if (dfsTl[i]->_invert[0]) {
        invert = "!";
      }
      if (dfsTl[i]->_fanin[0]->checkFloat()) {
        floating = "*";
      }
      cout << "[" << i-undefNum << "] "<< "PO  "<< dfsTl[i]->_id << " " << floating << invert << dfsTl[i]->_fanin[0]->_id //<< " (" << dfsTl[i]->_name << ")"
      << endl;
    } else if (dfsTl[i]->_type == AIG_GATE) {
      string invertOne = "";
      string floatingOne = "";
      string invertTwo = "";
      string floatingTwo = "";
      if (dfsTl[i]->_invert[0]) {
        invertOne = "!";
      }
      if (dfsTl[i]->_fanin[0]->_type == UNDEF_GATE) {
        floatingOne = "*";
      }
      if (dfsTl[i]->_invert[1]) {
        invertTwo = "!";
      }
      if (dfsTl[i]->_fanin[1]->_type == UNDEF_GATE) {
        floatingTwo = "*";
      }
      cout << "[" << i-undefNum << "] "<< "AIG "<< dfsTl[i]->_id << " " << floatingOne << invertOne << dfsTl[i]->_fanin[0]->_id
       << " " << floatingTwo << invertTwo << dfsTl[i]->_fanin[1]->_id << endl;
    } else if (dfsTl[i]->_type == CONST_GATE) {
      cout << "[" << i-undefNum << "] "<< "CONST0" << endl;
    } else {
      undefNum++;
    }
  }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for (size_t i = 0; i < _pi.size(); i++) {
     cout << ' ' << _pi[i]->_id;
   }
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for (size_t i = 0; i < _po.size(); i++) {
     cout << ' ' << _po[i]->_id;
   }
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
  bool floating = false;
  for (size_t i = 0; i < _aig.size(); i++) {
    if (_aig[i]->checkFloat()) {
      if (floating == false) {
        cout << "Gates with floating fanin(s):";
        floating = true;
      }
      cout << ' ' << _aig[i]->_id;
    }
  }
  if (floating == true) {
    cout << endl;
  }

  vector<unsigned> dnuGate;

  for (size_t i = 0; i < _pi.size(); i++) {
    if (_pi[i]->checkNotUsed()) {
      dnuGate.push_back(_pi[i]->_id);
    }
  }
  for (size_t i = 0; i < _aig.size(); i++) {
    if (_aig[i]->checkNotUsed()) {
      dnuGate.push_back(_aig[i]->_id);
    }
  }
  if (dnuGate.size() != 0) {
    cout << "Gates defined but not used  :";
    sort(dnuGate.begin(), dnuGate.end());
    for (size_t i = 0; i < dnuGate.size(); i++) {
      cout << ' ' << dnuGate[i];
    }
    cout << endl;
  }
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
