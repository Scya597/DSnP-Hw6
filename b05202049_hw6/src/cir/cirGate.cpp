/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include <algorithm>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

extern CirMgr *cirMgr;

// TODO: Implement memeber functions for class(es) in cirGate.h

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
CirGate::reportGate() const
{
  cout << "==================================================" << endl;
  stringstream ss;
  ss << "= " + getTypeStr() << '(' << _id << ")";
  if (_name != "") {
    ss << "\"" << _name << "\"";
  }
  ss << ", line " << getLineNo();
  cout << setw(49) << left << ss.str() << "=" << endl;
  cout << "==================================================" << endl;
}

void
CirGate::travelGateIn(CirGate* gate, bool inv, int level, GateList& report) const {
  for (size_t i = 0; i < _currentTravelLevel; i++) { cout << "  "; }
  if (inv) { cout << "!"; }
  cout << gate->getTypeStr() << " " << gate->_id;

  GateList::iterator it;
  it = find(report.begin(), report.end(), gate);
  if (it != report.end()) {
    if (gate->_fanin.size() > 0) {
      cout << " (*)" << endl;
    } else {
      cout << endl;
    }
  } else {
    cout << endl;
    report.push_back(gate);
    if (_currentTravelLevel < level && gate->_fanin.size() > 0) {
      _currentTravelLevel++;
      for (size_t i = 0; i < gate->_fanin.size(); i++) {
        travelGateIn(gate->_fanin[i], gate->_invert[i], level, report);
      }
      _currentTravelLevel--;
    }
  }
};

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   GateList report;

   cout << getTypeStr() << " " << _id << endl;
   if (_currentTravelLevel < level  && _fanin.size() > 0) {
     _currentTravelLevel++;
     for (size_t i = 0; i < _fanin.size(); i++) {
       travelGateIn(_fanin[i], _invert[i], level, report);
     }
     _currentTravelLevel--;
   }
}

void
CirGate::travelGateOut(CirGate* gate, bool inv, int level, GateList& report) const {
  for (size_t i = 0; i < _currentTravelLevel; i++) { cout << "  "; }
  if (inv) { cout << "!"; }
  cout << gate->getTypeStr() << " " << gate->_id;

  GateList::iterator it;
  it = find(report.begin(), report.end(), gate);
  if (it != report.end()) {
    if (gate->_fanout.size() > 0) {
      cout << " (*)" << endl;
    } else {
      cout << endl;
    }
  } else {
    cout << endl;
    report.push_back(gate);
    if (_currentTravelLevel < level && gate->_fanout.size() > 0) {
      _currentTravelLevel++;
      for (size_t i = 0; i < gate->_fanout.size(); i++) {

        bool invert = true;
        for (size_t j = 0; j < gate->_fanout[i]->_fanin.size(); j++) {
          if (gate->_fanout[i]->_fanin[j] == gate) {
            invert = gate->_fanout[i]->_invert[j];
          }
        }
        travelGateOut(gate->_fanout[i], invert, level, report);
      }
      _currentTravelLevel--;
    }
  }
};

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   GateList report;

   cout << getTypeStr() << " " << _id << endl;
   if (_currentTravelLevel < level  && _fanout.size() > 0) {
     _currentTravelLevel++;
     for (size_t i = 0; i < _fanout.size(); i++) {
       bool invert = true;
       for (size_t j = 0; j < _fanout[i]->_fanin.size(); j++) {
         if (_fanout[i]->_fanin[j] == this) {
           invert = _fanout[i]->_invert[j];
         }
       }
       travelGateOut(_fanout[i], invert, level, report);
     }
     _currentTravelLevel--;
   }
}

unsigned
CirGate::_globalRef = 0;

int
CirGate::_currentTravelLevel = 0;
