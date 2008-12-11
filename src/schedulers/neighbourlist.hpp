/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2008  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CSNeighbourList_H
#define CSNeighbourList_H

#include "scheduler.hpp"
#include <sigc++/sigc++.h>

class CSNeighbourList: public CScheduler
{
public:
  CSNeighbourList(const XMLNode&, const DYNAMO::SimData*);

  CSNeighbourList(const DYNAMO::SimData*, CSSorter*);

  virtual void rebuildList() { initialise(); }

  virtual void initialise();

  virtual void addEvents(const CParticle&);
  
  virtual void operator<<(const XMLNode&);

protected:
  virtual void outputXML(xmlw::XmlStream&) const;

  void addInteractionEvent(const CParticle&, const size_t&) const;

  void addLocalEvent(const CParticle&, const size_t&) const;
  
  size_t NBListID;

  void virtualCellNewNeighbour(const CParticle&, const CParticle&);

  sigc::connection cellChange;
  sigc::connection cellChangeLocal;
  sigc::connection reinit;  
};

#endif
