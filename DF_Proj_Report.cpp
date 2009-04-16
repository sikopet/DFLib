#include "DF_Proj_Report.hpp"

namespace DFLib
{
  namespace Proj
  {
    

  /// \brief DF report constructor for user-specified coordinate system.
  /// \param theLocation position vector <em>in lat/lon</em> of this report.
  /// \param Bearing bearing IN DEGREES
  /// \param std_dev standard deviation in degrees
  /// \param projArgs a vector of strings to pass to pj_init in order to 
  ///        define the user coordinate system.
    DFLib::Proj::Report::Report(const vector<double> &theLocation,
                                const double &Bearing,const double &std_dev,
                                const string &theName,
                                vector<string> &projArgs)
      : DFLib::Abstract::Report(theName,true),
        bearing(Bearing*M_PI/180.0),
        sigma(std_dev*M_PI/180.0)
    {
      receiverLocation = new Point(theLocation,projArgs);
      // Make sure our bearing is *always* 0<bearing<2*pi.  If it isn't,
      // reset it:
      while (bearing < 0)
        bearing += 2*M_PI;

    }        

    /// \brief Copy constructor

     DFLib::Proj::Report::Report(const DFLib::Proj::Report & right)
       : DFLib::Abstract::Report(right),
         bearing(right.bearing),
         sigma(right.sigma)
     {
       // do not copy the pointer to the point, make a copy and save a
       // pointer to it.
       receiverLocation = new Point(*(right.receiverLocation));
     }
  }
}