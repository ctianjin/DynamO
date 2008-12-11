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

#ifndef CGNeighbourList_HPP
#define CGNeighbourList_HPP

#include "global.hpp"
#include <boost/function.hpp>
#include <sigc++/sigc++.h>

class CGNeighbourList: public CGlobal
{
protected:
  typedef boost::function<void (const CParticle&, const size_t&)> nbhoodFunc;
  
public:
  CGNeighbourList(DYNAMO::SimData* a, 
		  const char *b): 
    CGlobal(a,b)
  {}

  CGNeighbourList(CRange* a, DYNAMO::SimData* b, 
		  const char * c): 
    CGlobal(a,b,c)
  {}

  CGNeighbourList(const CGNeighbourList&);

  virtual void getParticleNeighbourhood(const CParticle&, 
					const nbhoodFunc&) const = 0;

  virtual void getParticleLocalNeighbourhood(const CParticle&, 
					     const nbhoodFunc&) const = 0;

  inline sigc::signal<void, const CParticle&, const size_t&>&
  getsigCellChangeNotify() const {return sigCellChangeNotify; }
  
  inline sigc::signal<void, const CParticle&, const size_t&>&
  getsigNewLocalNotify() const { return sigNewLocalNotify; }
  
  inline sigc::signal<void, const CParticle&, const size_t&>&
  getsigNewNeighbourNotify() const { return sigNewNeighbourNotify; }
  
  inline sigc::signal<void>&
  getReInitNotify() const { return ReInitNotify; }
  
protected:
  virtual void outputXML(xmlw::XmlStream&) const = 0;

  //Signals
  mutable sigc::signal<void, const CParticle&, const size_t&> 
  sigCellChangeNotify;

  mutable sigc::signal<void, const CParticle&, const size_t&> 
  sigNewLocalNotify;

  mutable sigc::signal<void, const CParticle&, const size_t&> 
  sigNewNeighbourNotify;

  mutable sigc::signal<void> ReInitNotify;
};

#endif
