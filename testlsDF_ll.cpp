//-*- mode:C++ ; c-basic-offset: 2 -*-
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#include <ctime>
#endif
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
// we need this for dmstor and various conversion factors
#include <projects.h>
#undef XY

#include "Util_Misc.hpp"
#include "gaussian_random.hpp"
#include "DF_Report_Collection.hpp"
#include "DF_LatLon_Report.hpp"
#include "Util_Minimization_Methods.hpp"

using namespace std;


int main(int argc,char **argv)
{
  
  double lon,lat;
  vector<double> transPos(2,0.0);
  int i,j;
  char dms_string[128];
  
  vector<double> FCA_stddev;
  vector<double> LS_fix;
  vector<double> NR_fix;
  DFLib::LatLon::Point FixCutAverage(transPos);
  char EW,NS;
  bool done;
  double normf,lastnormf;
  double lastf;
  
  DFLib::ReportCollection rColl;
  DFLib::Util::Minimizer bogus(&rColl);
  
  // newton-raphson temporaries
  double f;
  vector<double> gradf;
  vector<vector<double> > jac;

  ofstream gnuplotFile("testlsDFfix.gnuplot");
  ofstream gridFile("function.grid");
  gnuplotFile << "set angles degrees" << endl;
  gnuplotFile << "set parametric" << endl;
  gnuplotFile.precision(16); gnuplotFile.width(20);

#ifdef _MSC_VER
  srand(time(NULL));
#else
  srand48(time(NULL));
#endif
  if (argc < 3)
  {
    cerr << "Usage: " << argv[0] << " <trans lon> <trans lat> " << endl;
    cerr << " Remember to pipe list of receiver lon/lats into stdin!" << endl;
    exit(1);
  }

  lon=dmstor(argv[1],NULL);
  lat=dmstor(argv[2],NULL);

  cout << "Transmitter location in decimal degrees: Lon: " << lon*RAD_TO_DEG
       << " Lat: " << lat*RAD_TO_DEG << endl;

  transPos[0]=lon*RAD_TO_DEG;
  transPos[1]=lat*RAD_TO_DEG;
  DFLib::LatLon::Point transPoint(transPos);
  transPos=transPoint.getXY();

  cout << " Transmitter " << " at X=" << transPos[0]
       << " Y= " << transPos[1] << endl;


  // Now read receiver lon/lats from stdin.  These are in dms format per
  // proj.4 standard, space delimited.
  while (!cin.eof())
  {
    double temp_sigma;
    char junk_space;
    DFLib::LatLon::Report *reportPtr;
    vector<double> tempVector(2);

    cin.get(dms_string,sizeof(dms_string),' ');
    if (cin.eof())
      break;
    lon=dmstor(dms_string,NULL);
    // get the space: 
    cin.get(junk_space);
    cin.get(dms_string,sizeof(dms_string),' ');
    if (cin.eof())
      break;
    lat=dmstor(dms_string,NULL);
    cin.get(junk_space);

    cin >>  temp_sigma;

    if (cin.eof())
      break;
    DFLib::Util::gaussian_random_generator rand_gen(0,temp_sigma);

    cout << " Got receiver number " << rColl.size()
         << " Position = " << lon*RAD_TO_DEG << " " << lat*RAD_TO_DEG 
         << " With standard deviation " << temp_sigma
         << endl;
    tempVector[0]=lon*RAD_TO_DEG;
    tempVector[1]=lat*RAD_TO_DEG;
    double bearing=0; // temporary
    reportPtr = new DFLib::LatLon::Report(tempVector,bearing,temp_sigma);



    bearing=reportPtr->computeBearingToPoint(transPos)*RAD_TO_DEG;
    cout << " True bearing to transmitter is " << bearing << endl;

    bearing += rand_gen.getRandom();
    cout << " after randomizing, bearing to transmitter is " << bearing << endl;
    reportPtr->setBearing(bearing);

    rColl.addReport(reportPtr);


  }
  gnuplotFile << "plot [t=0:40000] ";

  cout << "Receiver locations in mercator: " << endl;
  for (i=0;i<rColl.size();++i)
  {
    vector<double> receiverLoc = 
      rColl.getReceiverLocationXY(i);
    double rb=dynamic_cast<DFLib::LatLon::Report const *>(rColl.getReport(i))->getBearing();
    double rbr=rColl.getReport(i)->getReportBearingRadians();
    cout << " Receiver " << i << " at X=" << receiverLoc[0]
         << " Y= " << receiverLoc[1] << endl;
    cout << "  bearing from this receiver to transmitter is "
         <<  rb
         << " degrees (" << rbr << " radians)"<< endl;

    if (i != 0)
      gnuplotFile << ",";
    gnuplotFile << receiverLoc[0] << "+sin("<<rb<<")*t,"
                << receiverLoc[1] << "+cos("<<rb<<")*t with lines title \"station " << i << "\" ";

  }
  gnuplotFile << endl;

  rColl.computeLeastSquaresFix(LS_fix);
  rColl.computeFixCutAverage(FixCutAverage,FCA_stddev);
  vector <double> FCA_point=FixCutAverage.getXY();

  gnuplotFile << "replot " << LS_fix[0] << "," << LS_fix[1] << " with points title \"LS Fix\"" << endl;
  gnuplotFile << "replot " << transPos[0] << "," << transPos[1] << " with points title \"Actual Location\"" << endl;
  gnuplotFile << "replot " << FCA_point[0] << "," << FCA_point[1] << " with points title \"Fix Cut Average\"" << endl;

  for(i = 0; i<rColl.size() ; ++i)
  {
    const vector<double> &receiverLoc = 
      rColl.getReceiverLocationXY(i);
    gnuplotFile << "replot " << receiverLoc[0] << "," << receiverLoc[1] 
                << " with points title \"Station "<< i << "\"" << endl;
  }

  cout << " Mercator coordinates of LS fix: " 
       << "X = " << LS_fix[0] << " Y = " << LS_fix[1] << endl;
  cout << " Mercator coordinates of Fix Cut Average: " 
       << "X = " << FCA_point[0] << " Y = " << FCA_point[1] << endl;
  
  DFLib::LatLon::Point LSPoint(LS_fix); // bogus point for now
  LSPoint.setXY(LS_fix);

  vector<double> latlon=LSPoint.getUserCoords();

  EW='E';
  NS='N';

  if (lon < 0)
  {
    latlon[0] *= -1;
    EW = 'W';
  }

  if (lat < 0)
  {
    latlon[1] *= -1;
    NS = 'S';
  }

  cout << "  Longitude of LS fix: " << (int) latlon[0] << "d" 
       << (latlon[0]-(int)latlon[0])*60 << "\"" << EW << endl;

  cout << "  Latitude of LS fix: " << (int) latlon[1] << "d" 
       << (latlon[1]-(int)latlon[1])*60 << "\"" << NS << endl;

  for (double minCutAngle=0; minCutAngle < 50; minCutAngle += 5.0)
  {
    if (rColl.computeFixCutAverage(FixCutAverage,FCA_stddev,minCutAngle))
      {
	vector<double> latlon=FixCutAverage.getUserCoords();
	
	
	EW='E';
	NS='N';
	
	if (latlon[0] < 0)
	  {
	    latlon[0] *= -1;
	    EW = 'W';
	  }
	
	if (latlon[1] < 0)
	  {
	    latlon[1] *= -1;
	    NS = 'S';
	  }
	
	cout << "  Longitude of Fix Cut Average (min cut angle="<< minCutAngle <<"): " << (int) latlon[0] << "d" 
	     << (latlon[0]-(int)latlon[0])*60 << "\"" << EW << endl;
	
	cout << "  Latitude of Fix Cut Average (min cut angle="<< minCutAngle <<"): " << (int) latlon[1] << "d" 
	     << (latlon[1]-(int)latlon[1])*60 << "\"" << NS << endl;
	
	cout << "   Std Dev of FCA = (" << FCA_stddev[0] << " , " 
	     << " , " << FCA_stddev[1] << ")" << endl;
      }
  }

  //
  //  NR_fix.resize(2);
  //  // write out a grid of 10 meter "pixels" showing function values
  //  for (i=-100;i<=100;++i)
  //  {
  //    for (j=-100;j<=100;++j)
  //    {
  //      NR_fix[0] = LS_fix[0]+10.0*i;
  //      NR_fix[1] = LS_fix[1]+10.0*j;
  //      gridFile << rColl.computeCostFunction(NR_fix) << " ";
  //    }
  //    gridFile << endl;
  //  }

  // Now try Conjugate Gradients on Jml, always starting from OV fix.
  NR_fix=LS_fix;
  done = false;
  j=0;

  //  rColl.computeCostFunctionAndHessian(NR_fix,f,gradf,jac);

  {
    double tempF;

    try
    {
      tempF=bogus.conjugateGradientMinimize(NR_fix,1e-5,j);
      cout << " CG says minimum at  " << NR_fix[0] << "," << NR_fix[1] 
           << " where the function is " << tempF << endl;
    }
    catch (DFLib::Util::Exception x)
    {
      cerr << " Ooops... got exception " << x.getEmsg() << endl;
    }
  }

  cout << "Final C-G ML result took " << j  << " iterations: X=" << NR_fix[0] << " Y=" << NR_fix[1] << endl;
  gnuplotFile << "replot " << NR_fix[0] << "," << NR_fix[1] << " with points title \"ML Fix\"" << endl;
  DFLib::LatLon::Point NRPoint(NR_fix); // bogus point for now
  NRPoint.setXY(NR_fix);

  latlon=NRPoint.getUserCoords();

  EW='E';
  NS='N';

  if (latlon[0] < 0)
  {
    latlon[0] *= -1;
    EW = 'W';
  }

  if (latlon[1] < 0)
  {
    latlon[1] *= -1;
    NS = 'S';
  }

  cout << "  Longitude of ML fix: " << (int) latlon[0] << "d" 
       << (latlon[0]-(int)latlon[0])*60 << "\"" << EW << endl;

  cout << "  Latitude of ML fix: " << (int) latlon[1] << "d" 
       << (latlon[1]-(int)latlon[1])*60 << "\"" << NS << endl;

  gnuplotFile << "pause -1" << endl;
  gnuplotFile.close();
}
