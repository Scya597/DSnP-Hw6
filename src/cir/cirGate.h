/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include "cirDef.h"

using namespace std;

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: Define your own data members and member functions, or classes
class CirGate
{
public:
   CirGate(GateType type, unsigned id, unsigned lineNo):
   _type(type), _id(id), _lineNo(lineNo), _fanin(0), _fanout(0), _ref(initRef()) {}
   virtual ~CirGate() {}

   GateType _type;
   unsigned _id;
   unsigned _lineNo;
   GateList _fanin;
   GateList _fanout;
   vector<bool> _invert;
   string _name;

   static unsigned _globalRef;
   unsigned _ref;

   // Basic access methods
   string getTypeStr() const {
     switch (_type) {
       case UNDEF_GATE: return "UNDEF";
       case PI_GATE:    return "PI";
       case PO_GATE:    return "PO";
       case AIG_GATE:   return "AIG";
       case CONST_GATE: return "CONST0";
       default:         return "";
     }
   }

   unsigned getLineNo() const { return 0; }

   // Printing functions
   // virtual void printGate() const = 0;
   void setFanin(CirGate* fanIn) {
     _fanin.push_back(fanIn);
   };

   void setFanout(CirGate* fanOut) {
     _fanout.push_back(fanOut);
   };

   void setBool(bool invert) {
     _invert.push_back(invert);
   }

   void setName(string name) {
     _name = name;
   }

   bool checkFloat() {
     for (size_t i = 0; i < _fanin.size(); i++) {
       if (_fanin[i]->_type == UNDEF_GATE) {
         return true;
       }
     }
     return false;
   }

   bool checkNotUsed() {
     if (_fanout.size() == 0) {
       return true;
     }
     return false;
   }

   // DFS Travseral
   bool isGlobalRef() {
     return (_ref == _globalRef);
   }

   void setToGlobalRef() {
     _ref = _globalRef;
   }

   static void setGlobalRef() {
     _globalRef++;
   }

   static unsigned initRef() {
     return _globalRef;
   }

   void dfsTraversal(GateList& dfsTl) {
     if (_fanin.size() > 0) {
       for (size_t i = 0; i < _fanin.size(); i++) {
         if(!_fanin[i]->isGlobalRef()) {
           _fanin[i]->setToGlobalRef();
           _fanin[i]->dfsTraversal(dfsTl);
         }
       }
     }
     dfsTl.push_back(this);
   }

   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanout(int level) const;

private:
protected:

};

class CirAigGate: public CirGate {
public:
  CirAigGate(unsigned id, unsigned lineNo): CirGate(AIG_GATE, id, lineNo) {}
};

class CirPiGate: public CirGate  {
public:
  CirPiGate(unsigned id, unsigned lineNo): CirGate(PI_GATE, id, lineNo) {}
};

class CirPoGate: public CirGate  {
public:
  CirPoGate(unsigned id, unsigned lineNo): CirGate(PO_GATE, id, lineNo) {}
};

class CirUndefGate: public CirGate  {
public:
  CirUndefGate(unsigned id): CirGate(UNDEF_GATE, id, 0) {}
};

class CirConstGate: public CirGate  {
public:
  CirConstGate(): CirGate(CONST_GATE, 0, 0) {}
};


#endif // CIR_GATE_H
