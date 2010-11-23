/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2010  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#ifdef DYNAMO_visualizer

#include "visualizer.hpp"
#include "../../base/is_simdata.hpp"
#include "../NparticleEventData.hpp"
#include "../liouvillean/liouvillean.hpp"
#include "../../outputplugins/tickerproperty/ticker.hpp"
#include "../units/units.hpp"
#include "../../schedulers/scheduler.hpp"
#include <boost/foreach.hpp>
#include <algorithm>
#include "../../dynamics/include.hpp"
#include <coil/clWindow.hpp>
#include <coil/RenderObj/TestWaves.hpp>
#include <coil/RenderObj/Function.hpp>
#include <coil/RenderObj/Spheres.hpp>
#include <magnet/CL/CLGL.hpp>
#include "../liouvillean/CompressionL.hpp"

SVisualizer::SVisualizer(DYNAMO::SimData* nSim, std::string nName, double tickFreq):
  System(nSim)
{
  _updateTime = tickFreq * Sim->dynamics.units().unitTime();
  dt = _updateTime;

  sysName = "Visualizer";

  //Build a window, ready to display it
  _CLWindow = new CLGLWindow(800, 600,//height, width
			     0, 0,//initPosition (x,y)
			     "Visualizer : " + nName,
			     tickFreq,
			     true);
    
  //static_cast<CLGLWindow&>(*_CLWindow).addRenderObj(new RTTestWaves((size_t)1000, 0.0f));
  
  Vector axis1 = Vector(1,1,0);
  axis1 /= axis1.nrm();
  Vector orthvec = Vector(1,0,0.3);
  orthvec /= orthvec.nrm();
  Vector axis2 = (orthvec ^ axis1);
  axis2 /= axis2.nrm();
  Vector axis3 = axis2 ^ axis1;
  axis3 /= axis3.nrm();

  static_cast<CLGLWindow&>(*_CLWindow).addRenderObj(new RFunction((size_t)100,
								  Vector(-1.5,-0.5,-1.5),
								  axis1,
								  axis2,
								  axis3,
								  -0.7,
								  -0.7,
								  1.4,
								  1.4,
								  true,
								  false
								  ));

  _sphereObject = new RTSpheres((size_t)Sim->N);
  
  static_cast<CLGLWindow&>(*_CLWindow).addRenderObj(_sphereObject);
  
  CoilMaster::getInstance().addWindow(_CLWindow);

  //Build the array of data
  particleData.resize(Sim->N);
  
  dataBuild();

  {
    const magnet::thread::ScopedLock lock(static_cast<CLGLWindow&>(*_CLWindow).getDestroyLock());
    if (!_CLWindow->isReady()) return;
    static_cast<CLGLWindow&>(*_CLWindow).getCLState().getCommandQueue().enqueueWriteBuffer
      (static_cast<RTSpheres&>(*_sphereObject).getSphereDataBuffer(),
       false, 0, Sim->N * sizeof(cl_float4), &particleData[0]);

    _lastRenderTime = static_cast<CLGLWindow&>(*_CLWindow).getLastFrameTime();
  }

 I_cout() << "Visualizer initialised\nOpenCL Plaftorm:" 
	  << static_cast<CLGLWindow&>(*_CLWindow).getCLState().getPlatform().getInfo<CL_PLATFORM_NAME>()
	  << "\nOpenCL Device:" 
	  << static_cast<CLGLWindow&>(*_CLWindow).getCLState().getDevice().getInfo<CL_DEVICE_NAME>();
}

void
SVisualizer::dataBuild() const
{
  //Check if the system is compressing
  if (Sim->dynamics.liouvilleanTypeTest<LCompression>())
    BOOST_FOREACH(const magnet::ClonePtr<Species>& spec, Sim->dynamics.getSpecies())
      {
	double diam = spec->getIntPtr()->hardCoreDiam() 
	  * (1 + static_cast<const LCompression&>(Sim->dynamics.getLiouvillean()).getGrowthRate() * Sim->dSysTime);
	
	BOOST_FOREACH(unsigned long ID, *(spec->getRange()))
	  {
	    Vector pos = Sim->particleList[ID].getPosition();
	    
	    Sim->dynamics.BCs().applyBC(pos);
	    
	    for (size_t i(0); i < NDIM; ++i)
	      particleData[ID].s[i] = pos[i];
	    
	    particleData[ID].w = diam * 0.5;
	  }
      }
  else
    BOOST_FOREACH(const magnet::ClonePtr<Species>& spec, Sim->dynamics.getSpecies())
      {
	double diam = spec->getIntPtr()->hardCoreDiam();
	
	BOOST_FOREACH(unsigned long ID, *(spec->getRange()))
	  {
	    Vector pos = Sim->particleList[ID].getPosition();
	    
	    Sim->dynamics.BCs().applyBC(pos);
	    
	    for (size_t i(0); i < NDIM; ++i)
	      particleData[ID].s[i] = pos[i];
	    
	    particleData[ID].w = diam * 0.5;
	  }
      }    
}


void
SVisualizer::runEvent() const
{
  _updateTime = static_cast<CLGLWindow&>(*_CLWindow).getUpdateInterval();
  
  double locdt = dt;
  dt += _updateTime;

  //Update test
  if (static_cast<CLGLWindow&>(*_CLWindow).simupdateTick())
    {
      //Actually move forward the system time
      Sim->dSysTime += locdt;
      Sim->ptrScheduler->stream(locdt);      
      //dynamics must be updated first
      Sim->dynamics.stream(locdt);
      locdt += Sim->freestreamAcc;
      Sim->freestreamAcc = 0;

      if (static_cast<CLGLWindow&>(*_CLWindow).dynamoParticleSync())
	Sim->dynamics.getLiouvillean().updateAllParticles();
      
      BOOST_FOREACH(magnet::ClonePtr<OutputPlugin>& Ptr, Sim->outputPlugins)
	Ptr->eventUpdate(*this, NEventData(), locdt);
      
      dataBuild();
      
      {
      	const magnet::thread::ScopedLock lock(static_cast<CLGLWindow&>(*_CLWindow).getDestroyLock());
      	if (!static_cast<CLGLWindow&>(*_CLWindow).isReady()) return;
      	static_cast<CLGLWindow&>(*_CLWindow).getCLState().getCommandQueue().enqueueWriteBuffer
      	  (static_cast<RTSpheres&>(*_sphereObject).getSphereDataBuffer(),
      	   false, 0, Sim->N * sizeof(cl_float4), &particleData[0]);
      }
    }
}

void 
SVisualizer::initialise(size_t nID)
{ ID = nID; }

#endif
