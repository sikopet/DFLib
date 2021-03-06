//-*- mode:C++ ; c-basic-offset: 2 -*-
//    DFLib: A library of Bearings Only Target Localization algorithms
//    Copyright (C) 2009-2015  Thomas V. Russo
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// Filename       : $RCSfile$
//
// Purpose        : Implement a DFLib::Abstract::Point interface such that
//                  the "user" coordinate system is whatever the user has
//                  specified with a proj.4 spatial reference system, and the
//                  "XY" coordinate system is a mercator projection on the 
//                  WGS84 ellipsoid.
//
// Special Notes  : 
//
// Creator        : 
//
// Creation Date  : 
//
// Revision Information:
// ---------------------
//
// Revision Number: $Revision$
//
// Revision Date  : $Date$
//
// Current Owner  : $Author$
//-------------------------------------------------------------------------
#include "DF_Proj_Point.hpp"
#include "Util_Misc.hpp"

#include <vector>
#include <iostream>
#include <cstdlib>

namespace DFLib
{
  namespace Proj
  {

    // class Point
    Point::Point(const std::vector<double> &uPosition,const std::vector<std::string> &projArgs)
      : theUserCoords(uPosition),
        userDirty(true),
        mercDirty(false)
    {
      // Create the proj.4 stuff.  This is highly inefficient, as it should
      // live in the base class, not each object.  Fix that.
      
      char *mercator_argv[3]={"proj=merc",
                              "datum=WGS84",
                              "lat_ts=0"};

      int numUserArgs = projArgs.size();
      char **user_argv = new char * [numUserArgs];

      for (int i=0; i<numUserArgs; ++i)
      {
        user_argv[i]=new char [projArgs[i].size()+1];
        projArgs[i].copy(user_argv[i],projArgs[i].size());
        // Null terminate the string!
        user_argv[i][projArgs[i].size()] = '\0';
      }
      if (!(userProj = pj_init(numUserArgs,user_argv)))
      {
        throw(Util::Exception("Failed to initialize user projection"));
      }
      
      if (!(mercProj = pj_init(3,mercator_argv)))
      {
        throw(Util::Exception("Failed to initialize mercator projection"));
      }
      
      // Don't bother trying to force the mercator --- we'll do that if
      // we query, because we're setting userDirty to true, just initialize
      // to junk.
      theMerc.resize(2,0.0);

      // Now must free the user_argv junk:
      for (int i=0; i<numUserArgs; ++i)
      {
        delete [] user_argv[i];
      }
      delete [] user_argv;

    }
    
    Point::Point(const Point &right)
    {

      // Must *COPY* the definition, not the pointer to it!
      // Get the text version of the definition
      char *theProjDef = pj_get_def(right.userProj,0);
      // generate a new projUJ pointer
      userProj = pj_init_plus(theProjDef);
      free(theProjDef);

      theProjDef = pj_get_def(right.mercProj,0);
      // generate a new projUJ pointer
      mercProj = pj_init_plus(theProjDef);
      free(theProjDef);

      if (right.mercDirty)
      {
        // Right's mercator's been changed without its LL being 
        // updated, so copy its mercator values and say we're dirty.
        theMerc=right.theMerc;
        mercDirty=true;
        userDirty=false;
      } 
      else
      {
        // Just copy its LL and mark dirty.
        theUserCoords=right.theUserCoords;
        userDirty=true;
        mercDirty=false;
      }
    }

    Point::~Point()
    {
      pj_free(userProj);
      pj_free(mercProj);
    }

    Point& Point::operator=(const Point& rhs)
    {
      if (this == &rhs) return *this;

      if (userProj) 
      {
        pj_free(userProj);
        userProj=0;
      }
      if (mercProj)
      {
        pj_free(mercProj);
        mercProj=0;
      }

      char *theProjDef = pj_get_def(rhs.userProj,0);
      // generate a new projUJ pointer
      userProj = pj_init_plus(theProjDef);
      free(theProjDef);

      theProjDef = pj_get_def(rhs.mercProj,0);
      // generate a new projUJ pointer
      mercProj = pj_init_plus(theProjDef);
      free(theProjDef);

      mercDirty=rhs.mercDirty;
      userDirty=rhs.userDirty;

      // This is OK, because we're copying the dirty bools, too.
      // we have to copy both of these, otherwise we risk copying only the
      // ones that haven't been updated for consistency!
      theMerc=rhs.theMerc;
      theUserCoords=rhs.theUserCoords;

      return (*this);
    }

    void Point::setXY(const std::vector<double> &mPosition)
    {
      theMerc = mPosition;
      // Essential to set userDirty=false, otherwise getXY will try to 
      // convert ll and we'll be wrong.
      mercDirty=true;  
      userDirty=false;
    }

    void Point::setUserProj(const std::vector<std::string> &projArgs)
    {
      int numUserArgs = projArgs.size();
      char **user_argv = new char * [numUserArgs];
      for (int i=0; i<numUserArgs; ++i)
      {
        user_argv[i]=new char [projArgs[i].size()+1];
        projArgs[i].copy(user_argv[i],projArgs[i].size());
        // Null terminate the string!
        user_argv[i][projArgs[i].size()] = '\0';
      }

      // before we clobber userProj, if we have already have valid userProj
      //  already then we might also have
      // valid data, and need to make sure our coordinates are correct in the
      // new system:
      if (userProj)
      {
        if (userDirty)
        {
          userToMerc(); // make sure our mercator coordinates are up-to-date
        }
        pj_free(userProj); // free the existing projection
        userProj = NULL;
      }

      if (!(userProj = pj_init(numUserArgs,user_argv)))
      {
        throw(Util::Exception("Failed to initialize user projection"));
      }
      // Now, we have just changed the projection, so mark mercDirty as if
      // we had changed the mercator coordinates ourselves.
      mercDirty=true;

      // Now must free the user_argv junk:
      for (int i=0; i<numUserArgs; ++i)
      {
        delete [] user_argv[i];
      }
      delete [] user_argv;

    }      
    
    bool Point::isUserProjLatLong() const
    {
      return (pj_is_latlong(userProj));
    }
    
    const std::vector<double> & Point::getXY()
    {
      if (userDirty)
      {
        // our LL has been changed, so update
        userToMerc();
      }
      return(theMerc);
    }

    void Point::setUserCoords(const std::vector<double> &llPosition)
    {
      theUserCoords = llPosition;
      // If we set ll, must now trump mercator.
      userDirty=true; mercDirty=false;
    }

    const std::vector<double> & Point::getUserCoords()
    {
      if (mercDirty)
      {
        // our XY has been changed, so update
        mercToUser();
      }
      return(theUserCoords);
    }
    
    Point * Point::Clone()
    {
      Point *retPoint;
      retPoint = new Point(*this);
      return retPoint;
    }

    void Point::userToMerc()
    {
      projUV data;
      double z;
      if (pj_is_latlong(userProj))
      {
        data.u = theUserCoords[0]*DEG_TO_RAD;
        data.v = theUserCoords[1]*DEG_TO_RAD;
      }
      z=0;
      if (pj_transform(userProj,mercProj,1,0,&(data.u),&(data.v),&z) != 0)
      {
        throw(Util::Exception("Failure converting user coords to Mercator"));
      }

      theMerc.resize(2);
      theMerc[0]=data.u;
      theMerc[1]=data.v;

      userDirty=false;
    }      


    void Point::mercToUser()
    {
      projUV data;
      double z;
      data.u = theMerc[0];
      data.v = theMerc[1];
      z=0;
      if (pj_transform(mercProj,userProj,1,0,&(data.u),&(data.v),&z) != 0)
      {
        throw(Util::Exception("Failure converting Mercator to LL"));
      }
      theUserCoords.resize(2);
      if (pj_is_latlong(userProj))
      {
        theUserCoords[0]=data.u*RAD_TO_DEG;
        theUserCoords[1]=data.v*RAD_TO_DEG;
      }
      mercDirty=false;
    }      
  }
}
