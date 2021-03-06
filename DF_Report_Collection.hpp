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
// Purpose        : Provide a generic class to hold DF reports and provide
//                  fix-computing methods that are independent of the actual
//                  choice of report types
//
// Special Notes  : This class can take any type of report that implements the
//                  DFLib::Abstract::Report interface, and is not constrained
//                  to require that all reports be of the same concrete class.
//                  This design may or may not be the best way to do the job.
//
//                  By design, the class assumes that the user is managing the
//                  creation and destruction of objects, and makes no attempt
//                  to destroy the collected objects in its destructor.  In
//                  the case where the user does not want to keep track of 
//                  these objects, this class provides a deleteReports()
//                  method that will delete all objects in the collection, 
//                  but this method is NEVER called unless the user
//                  does so.  That method must be called by the user before 
//                  deleting the ReportCollection object.
//                  
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
#ifndef DF_REPORT_COLLECTION_HPP
#define DF_REPORT_COLLECTION_HPP
#include "DFLib_port.h"

#include <vector>

#include "Util_Abstract_Group.hpp"
#include "DF_Abstract_Report.hpp"
#include "DF_Abstract_Point.hpp"

namespace DFLib
{
  class CPL_DLL ReportCollection : public DFLib::Abstract::Group
  {
  private: 
    std::vector<DFLib::Abstract::Report * > theReports;
    std::vector<double> evaluationPoint;
    bool f_is_valid;
    bool g_is_valid;
    bool h_is_valid;
    double function_value;
    std::vector<double> gradient;
    std::vector<std::vector<double> > hessian;

    // Declare the copy constructor and assignment operators, but
    // don't define them.  We should *never* copy a collection or attempt
    // to assign one to another.  This makes it illegal to do so.
    ReportCollection(ReportCollection &right);
    ReportCollection &operator=(ReportCollection &right);

  public:
    ReportCollection();

    /// \brief DF Report Collection destructor
    ///
    ///  Never destroys the objects in its vector, as they might be getting
    ///  used for something else by caller
    virtual ~ReportCollection();

    /// \brief destroy all reports stored in collection
    ///
    /// Provided in case our caller does NOT need the stored pointers for
    /// something else, and doesn't want to keep track of them.
    virtual void deleteReports();

    /// \brief Add a DF report to the collection
    ///
    /// \return this report's number in the collection.
    virtual int addReport(DFLib::Abstract::Report * aReport);

    /// \brief return the fix cut average of this collection's reports
    ///
    /// A fix cut is the intersection of two DF reports.  The Fix Cut 
    /// Average is the average of all fix cuts from all pairs of reports 
    /// in the collection.  Fix cuts at shallow angles can be excluded by 
    /// specifying a non-zero value for minAngle (in degrees).
    ///
    /// \param FCA Returned fix cut average
    /// \param FCA_stddev standard deviation of fix cuts <em>in user coordinates corresponding to the point provided in FCA</em>
    /// \param minAngle reports whose fix cut occur at less than this angle will not be included in the average.
    bool computeFixCutAverage(DFLib::Abstract::Point &FCA, 
                                      std::vector<double> &FCA_stddev,
                                      double minAngle=0);

    /*!
      \brief Computes least squares solution of DF problem.
       
      The least squares solution is the point that has the minimum
      orthogonal distance to all bearing lines.
        
      The least squares solution is given by

      \f$ 
      P_{LS} = (A^TA)^{-1}A^Tb 
      \f$

      where  \f$A\f$ is the \f$Nx2\f$ matrix whose \f$k^{th}\f$ row
      is the unit vector orthogonal to the bearing line from receiver
      \f$k\f$.  Row \f$k\f$ of A is therefore the vector
      \f$[\cos(\theta_k), -\sin(\theta_k)]\f$ where \f$\theta_k\f$
      is the bearing from station \f$k\f$ measured clockwise from North.
      The \f$T\f$ superscript denotes matrix transpose.
        
      \f$b\f$ is the \f$N\f$ element vector whose \f$k^{th}\f$ element
      is the projection of the position vector of the \f$k^{th}\f$ 
      receiver onto the unit vector orthogonal to that receiver's 
      bearing line:  
      \f$
      b_k = [\cos(\theta_k),-sin(\theta_k)]\cdot[r_{kx},r_{ky}]
      \f$
      where \f${\bf r}_k\f$ is the position vector of the \f$k^{th}\f$
      receiver.

      This method currently computes the matrix inverse
      \f$(A^TA)^{-1}\f$ using the closed form of matrix inverse for a
      2x2 matrix.  This is not the numerically stable thing to do, as
      this solution is ill-behaved if the matrix is near-singular.
      The correct thing to do is to form the "pseudoinverse"
      \f$A^\#=(A^TA)^{-1}A^T\f$, the numerically-stable approach to
      which involves singular value decomposition.  I do not do so
      because I thought it might be overkill.  Fortunately, the method of
      solution can be changed under the hood without any impact on callers
      should it turn out that closed-form matrix inversion is underkill 
      after all.
      
    */
    void computeLeastSquaresFix(DFLib::Abstract::Point &LS_Fix);

    /*!
      \brief Computes Stansfield estimate of Maximum Likelihood solution of DF problem

      Given a collection of DF fixes with specified standard
      deviation, computes the point of maximum likelihood in the
      approximation of small angles.  It is based on the paper
      "Statistical Theory of D.F. Fixing" by R. G. Stansfield,
      J. I.E.E. Vol 94, Part IIA, 1947.  Unfortunately, this paper is
      not available anywhere on the web, and it is hard to find it
      even in well stocked technical libraries.  I was able to obtain
      a copy through IEEE Document Search, but paid through the nose
      for it --- and due to copyright issues I am not permitted to
      make copies available.  If you wish to obtain this paper, you'll
      have to get a copy yourself.  A more easily obtained paper is
      "Airborne Direction Finding---The Theory of Navigation Errors"
      by C.J. Ancker in IRE Transactions on Aeronautical and
      Navigational Electronics, pp 199-210 (1958).  While paper is
      this paper available in PDF format through IEEE Xplore at
      http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4201630&tag=1
      it is unfortunately accessible only to those who have access
      through a library or IEEE membership.  Ancker's paper is much
      more readable than Stansfield's, as he uses a somewhat less
      opaque notation and is much more explicit about the
      approximations being made.

      Since it is so difficult to obtain primary sources describing
      the Stansfield estimator, I'll present the theory in full.


      Stansfield's approach approximates the bearing error
      \f$\theta_i(x,y)-\tilde{\theta}_i\f$ by
      \f$\sin(\theta_i(x,y)-\tilde{\theta}_i)\f$.  This is a valid
      approximation for small enough bearing error, and is the primary
      limitation of the Stansfield method.

      Fly in the ointment: Stansfield uses angles measured
      counter-clockwise from the East in those equations.  To keep
      consistent with Stansfield's paper, I retain his convention
      throughout this documentation.  Do remember when reading DFLib
      code that we use a different convention, and therefore the code
      does not exactly match the theoretical exposition.  To translate
      from Stansfield's to our usage (bearings clockwise from North)
      simply interchange cosine and sine in the final expressions
      below.

      Starting as in the Maximum Likelihood fix, we assume that the
      probability of the true bearing from any given receiver to the
      actual target location being \f$\theta_i\f$ given a measurement
      by that receiver of a bearing-to-target of
      \f$\tilde{\theta_i}\f$ is zero-mean gaussian with standard
      deviation \f$\sigma_i\f$ depending on the error between true and
      measured bearings.  Therefore, the probability of the
      transmitter being at a point \f$(x,y)\f$ is the product of the
      individual receiver gaussians:

      \f$
      P(x,y) = K*\exp(-\sum_{i=0}^n (\tilde{\theta_i} - \theta_i(x,y))^2/(2\sigma_i^2))
      \f$

      In this expression, \f$\tilde{\theta_i}\f$ is the bearing actually
      measured by receiver \f$i\f$, and \f$\theta_i(x,y)\f$ is the bearing
      from receiver \f$i\f$ to the point \f$(x,y)\f$.

      Making the substitution of small angular error, \f$\tilde{\theta_i} -
      \theta_i(x,y)\approx\sin(\tilde{\theta_i} - \theta_i(x,y))\f$
      and that \f$\sin(\tilde{\theta_i} - \theta_i(x,y))=q_i/d_i\f$,
      where \f$q_i\f$ is the perpendicular distance from our bearing
      line to the point (x,y) and \f$d_i\f$ is the distance from our
      receiver site to that point, we get:

      
      \f$
      P(q) = K*\exp(-\sum_{i=0}^n (q_i)^2/(2(d_i\sigma_i)^2))
      \f$

      Unfortunately, the \f$d_i\f$ all depend on \f$(x,y)\f$, making
      this a fairly ugly nonlinear problem to solve.  To simplify
      matters, Stansfield introduced a point which he repeatedly
      refers to as the actual position of the transmitter, making the
      approximation that \f$d_i\f$ is approximately the distance to
      that point and using it instead.  But really any point O (for
      Origin) can be used instead in what follows, so long as O isn't
      too far from (x,y) --- the point is that we are approximating
      the \f$d_i\f$, the distances from receivers to (x,y), by
      distances to O, which are independent of (x,y).  If the
      distances are long enough, this is an appropriate approximation.

      Assume that \f$p_i\f$ is the perpendicular distance from our
      bearing line i to O and that \f$\Delta x\f$ and \f$\Delta y\f$
      are the offsets from point O to point Q.  Then \f$ q_i =
      p_i+\Delta x \sin(\tilde{\theta}_i)-\Delta
      y\cos(\tilde{\theta}_i) \f$ by a very simple geometric argument.
      Substituting this mess into the new cost function and solving
      the minimization problem (differentiate the cost function w.r.t
      \f$\Delta x\f$ and \f$\Delta y\f$ assuming constant \f$d_i\f$,
      set to zero, solve the resulting pair of linear, coupled
      equations), one concludes that the values of \f$\Delta x\f$ and
      \f$\Delta y\f$ that maximize the probability are:

      \f$
      \Delta x = \frac{1}{\lambda\mu-\nu^2}\sum_i p_i\frac{\nu\cos(\tilde{\theta}_i)-\mu\sin(\tilde{\theta}_i)}{(d_i\sigma_i)^2}
      \f$
      and
      \f$
      \Delta y = \frac{1}{\lambda\mu-\nu^2}\sum_i p_i\frac{\lambda\cos(\tilde{\theta}_i)-\nu\sin(\tilde{\theta}_i)}{(d_i\sigma_i)^2}
      \f$

      with
      \f$
      \lambda=\sum_i \frac{\sin^2(\tilde{\theta}_i)}{(d_i\sigma_i)^2}
      \f$

      \f$
      \mu=\sum_i \frac{\cos^2(\tilde{\theta}_i)}{(d_i\sigma_i)^2}
      \f$

      and 
      \f$
      \nu = \sum_i \frac{\cos(\tilde{\theta}_i)\sin(\tilde{\theta}_i)}{(d_i\sigma_i)^2}
      \f$

      Finally, it is the case that the numbers \f$\lambda\f$,
      \f$\mu\f$ and \f$\nu\f$ form the elements of the covariance
      matrix for the distribution of \f$q_i\f$ and can be used to form
      a confidence region.

      If one rotates the system of coordinates around the maximum likelihood
      fix location by an angle \f$\phi\f$ such that
      \f$
      tan(2\phi) = -\frac{2\nu}{\lambda-\mu}
      \f$
      so that 

      \f$
      \Delta x = X\cos(\phi) - Y\sin(\phi)
      \f$

      \f$
      \Delta y = X\sin\phi) + Y\cos(\phi)      
      \f$
      
      where the delta quantities are offsets from the fix location,
      then the region defined by:

      \f$
      \frac{X^2}{a^2} + \frac{Y^2}{b^2} = -2 \ln(1-P')
      \f$

      encloses the region in which there is a probability \f$P'\f$ for
      the true location to be.

      \f$a\f$ and \f$b\f$ are given by:

      \f$
      \frac{1}{a^2}=\lambda - \nu\tan(\phi)
      \f$

      and

      \f$
      \frac{1}{b^2}=\mu + \nu\tan(\phi)
      \f$

      Note that Stansfield's paper has an error in it regarding the
      two expressions above.  The error was pointed out and corrected
      in a report "Probabalistic Position-Fixing" by Steve Burnett et
      al.  This paper was a report from the Claremont Graduate School,
      Claremont McKenna College Mathematics Clinic, 1986.  The only place
      I've been able to find it is the URL:

      http://oai.dtic.mil/oai/oai?verb=getRecord&metadataPrefix=html&identifier=ADA190397      

      Remember when reading the code that Stansfield uses a different angle
      convention for bearings than DFLib does.

      And there is another subtlety that is not discussed in
      Stansfield, but is discussed in "Numerical Calculations for
      Passive Geolocation Scenarios" by Don Koks

      http://www.dsto.defence.gov.au/publications/4949/DSTO-RR-0319.pdf

      Since the Stansfield fix is supposed to be a solution to the
      minimization of the cost function (and therefore the
      maximization of the probability density), the presence of the
      distance from receiver to test point in the cost function is a
      problem that interferes with the solution.  To get the
      closed-form solution, we have to assume that the \f$d_i\f$ are
      independent of the solution point, and this (false) assumption
      leads to a simple pair of linear equations to solve. To account
      for this an iterative process is employed.  Practically, this is
      the algorithm:

      0) starting with an initial guess point O, compute the distances
         from receivers to O, call them \f$d_i\f$ and use them as an
         approximation to the distances to the test point.

      Iterate:

        1) Using the \f$d_i\f$ determined in the last step, solve the
           minimization problem using the expressions above.  This
           yields an offset vector \f$(\Delta x, \Delta y)\f$ from O
           to the approximate fix.
         
        2) Compute the distances from receivers to \f$O+(\Delta x,
           \Delta y)\f$ and call them \f$d_i\f$ again.
           
        3) If \f$(\Delta x, \Delta y)\f$ has changed appreciably this
           iteration, return to step 1 and iterate again.  Otherwise,
           use \f$O+(\Delta x, \Delta y)\f$ as the Stansfield fix.

      The "changed appreciably" bit will take a little black art, I
      think.  I propose that we simply check that the norm of the
      offset vector hasn't changed more than some tolerance since the last 
      iteration.

    */
    void computeStansfieldFix(DFLib::Abstract::Point &SFix,double &am2, 
                              double &bm2, double &phi);
    /*!
      \brief Computes Maximum Likelihood solution of DF problem

      Given a collection of DF fixes with specified standard deviation,
      computes the point of maximum likelihood (ML fix).

      The probability of a transmitter being at a particular location x,y
      given that each transmitter \f$i\f$ has heard the signal at bearing 
      \f$\tilde{\theta_i}\f$ is given by a multivariate gaussian probability
      distribution:

      \f$
      P(x,y) = K*\exp(-\sum_{i=0}^n (\tilde{\theta_i} - \theta_i(x,y))^2/(2\sigma_i^2))
      \f$

      where \f$\tilde{\theta_i}\f$ is the bearing measured by the
      \f$i^{th}\f$ receiver, \f$\theta_i(x,y)\f$ is the bearing from
      the \f$i^{th}\f$ receiver to the point \f$(x,y)\f$, \f$\sigma_i\f$
      is the standard deviation of the measurements by receiver \f$i\f$, and
      \f$K\f$ is a normalization coefficient.  The argument of the exponential
      is the cost function.

      The ML fix is the point that minimizes the cost function, thereby
      maximizing the probability of that point.

      To compute the ML fix requires an initial guess, as from the
      least squares fix.  This method uses the method of Conjugate
      Gradients to find the minimum of the cost function.  Note that
      it is sometimes (though rarely) the case in DF problems that the
      cost function is too flat near the minimum for conjugate
      gradients to converge.  I have not characterized these
      pathological cases very well, but it does seem to be worst when
      the receivers are all off to one side of the transmitter.  In
      these pathological cases, the first step of conjugate gradients
      shoots off to a very distant point where the function is almost
      completely flat, and never recovers.  The only thing to do is to
      test the point returned by this method and make sure it is not
      unreasonably far from any receiver.  I tend to use 100 miles as
      the test distance for a "ridiculous" solution.

      If this method returns a ridiculous answer, one can try
      aggressiveComputeMLFix instead.  That method uses a more 
      robust---but possibly less efficient---minimization method as a 
      first step to refine the initial guess to conjugate gradients.

    */

    void computeMLFix(DFLib::Abstract::Point &MLFix);

    /*! \brief more aggressive attempt to get at an ML fix

      This is just a version of computeMLFix that tries a little harder to
      get at the answer.

      The cost function is not guaranteed to have a unique minimum,
      and even when it does it is often hard to find.  Originally, I
      used only the method of Conjugate Gradients to search for the
      minimum, but this fails rather often if the cost function has
      broad, flat areas of low value.  The failure mode is that the
      line search in the steepest descent direction (the first step of
      the CG method) takes us very, very far away because rather than
      having a minimum in that direction, the function is simply
      asymptotic to some smallish value.  Once it does this, the
      method can't recover because from there the function is
      completely flat.

      So instead, before we try CG we do a Nelder-Mead downhill
      simplex search to improve our starting guess.  So far, testing
      shows that this generally gets us very close to a minimum if
      there is one.  Sometimes, though, it also goes off to infinity
      because the system simply doesn't have a clear minimum.  In
      these cases it seems it is best for the user simply to ignore
      the fix.  A more aggressive search for a constrained minimum
      (say the best point in some range of the starting point) could
      be a helpful thing, but is probably more work than this
      deserves.

    */
    void aggressiveComputeMLFix(DFLib::Abstract::Point &MLFix);

    /*! \brief compute Cramer-Rao bounding ellipse parameters

      This function returns the inverse squares and rotation angle for
      the 1-sigma Cramer-Rao bound of our probability distribution.

      The Cramer-Rao theory states that the inverse of the
      Fisher information matrix is the best approximation to the covariance
      matrix obtainable from an unbiased estimator of the maximum likelihood.

      This method simply computes the Fisher information matrix elements and
      computes from them the parameters of the ellipse.

      If one compares the matrix elements of the FIM to the lambda, nu, and
      mu variables computed by the Stansfield estimator, one finds that they
      are equivalent if one makes the Stansfield approximation of small angles.
      Thus, this method provides precisely the same sort of error estimates
      for the Maximum Likelihood fix as the equivalent parameters returned by
      the Stansfield fix.  They may be used in exactly the same manner to 
      draw error ellipses around the ML fix.

      The Fisher Information Matrix for the probability distribution 
      associated with the ML fix is just:
      \f[
      I=\sum_i \frac{\left[ 
      \begin{array}{cc}
      (y_e-y_i)^2&-(x_e-x_i)(y_e-y_i)\\
      -(x_e-x_i)(y_e-y_i)&(x_e-x_i)^2
      \end{array} \right]}{\sigma_i^2[(x_e-x_i)^2+(y_e-y_i)^2]^2}
      \f]

      where \f$x_e\f$ and \f$y_e\f$ are the coordinates of the fix, 
      \f$x_i\f$ and \f$y_i\f$ are the coordinates of the \f$i^{th}\f$ receiver,
      and \f$\sigma_i\f$ the standard deviation associated with the 
      \f$i^{th}\f$ receiver.

      Thus, the correspondence with the Stansfield parameters is this:

      \f{eqnarray*}{

      \lambda&=&\sum_i \frac{(y_e-y_i)^2}{\sigma_i^2[(x_e-x_i)^2+(y_e-y_i)^2]^2}\\
      \nu&=&\sum_i \frac{(x_e-x_i)(y_e-y_i)}{\sigma_i^2[(x_e-x_i)^2+(y_e-y_i)^2]^2}\\
      \mu&=&\sum_i \frac{(x_e-x_i)^2}{\sigma_i^2[(x_e-x_i)^2+(y_e-y_i)^2]^2}\\
      tan(2\phi) &=& -\frac{2\nu}{\lambda-\mu}\\
      \frac{1}{a^2}&=&\lambda - \nu\tan(\phi)\\
      \frac{1}{b^2}&=&\mu + \nu\tan(\phi)
      \f}

    */
    void computeCramerRaoBounds(DFLib::Abstract::Point &MLFix,
                                double &am2, double &bm2, double &phi);

    /*!
      \brief compute cost function for point x,y
      
      this returns the cost function for the transmitter being at x,y
      given the DF reports we have.  The probability density uses the
      cost function in the argument of an exponential.  Minimizing the
      cost function will therefore maximize the probability density.
      
      The cost function is the sum
      \f$
      f(x,y) = \sum_{i=0}^n (\tilde{\theta_i} - \theta_i(x,y))^2/(2\sigma_i^2)
      \f$
      
      where \f$\tilde{\theta_i}\f$ is the measured bearing from receiver
      location i and \f$\theta_i(x,y)\f$ is the bearing from receiver
      location i to point (x,y).  Care must be taken to assure that the
      bearing differences are are always kept in the range 
      \f$-\pi<\tilde{\theta_i} - \theta_i(x,y)<=\pi\f$ to avoid
      discontinuities that break the minimization operation.
    */
    
    double computeCostFunction(std::vector<double> &evaluationPoint);
    void computeCostFunctionAndGradient(std::vector<double> &evaluationPoint,
                                                double &f,
                                                std::vector<double> &gradf);
    void computeCostFunctionAndHessian(std::vector<double> &evaluationPoint,
                                               double &f,
                                               std::vector<double> &gradf,
                                               std::vector<std::vector<double> > &h);

    // Note, unlike size(), this one doesn't count reports that are marked
    // invalid
    int numValidReports() const;

    inline virtual void toggleValidity(int i)
    {
      if (i<theReports.size()&&i>=0)
        theReports[i]->toggleValidity();
    };

    inline bool isValid(int i) const
    {
      if (i<theReports.size() && i>=0)
        return(theReports[i]->isValid());
      return (false);
    };

    inline int size() const {return theReports.size();};

    // This version returns something the caller can never use to change
    // a report from underneath us.
    inline const DFLib::Abstract::Report * getReport(int i) const
    { 
      if (i<theReports.size() && i>=0)
        return (theReports[i]); 
      return (0);
    };

    int getReportIndex(const std::string &name) const;
    int getReportIndex(const DFLib::Abstract::Report *reportPtr) const;

    inline const std::vector<double> &getReceiverLocationXY(int i) 
    { return (theReports[i]->getReceiverLocation()); };

    inline virtual void setEvaluationPoint(std::vector<double> &ep)
    {
      evaluationPoint = ep;
      f_is_valid=false;
      g_is_valid=false;
      h_is_valid=false;
    };

    /// \return function value
    inline virtual double getFunctionValue()
    {
      if (!f_is_valid)
      {
        function_value=computeCostFunction(evaluationPoint);
        f_is_valid=true;
      }
      return function_value;
    };

    /// \param g gradient returned
    /// \return function value
    inline virtual double getFunctionValueAndGradient(std::vector<double> &g)
    {
      if (!g_is_valid)
      {
        computeCostFunctionAndGradient(evaluationPoint,
                                       function_value,gradient);
        f_is_valid=true;
        g_is_valid=true;
      }
      g=gradient;
      return function_value;
    };

    /// \param g gradient returned
    /// \param h hessian returned
    /// \return function value
    inline virtual double 
    getFunctionValueAndHessian(std::vector<double> &g,
                               std::vector<std::vector<double> > &h)
    {
      if (!h_is_valid)
      {
        computeCostFunctionAndHessian(evaluationPoint,
                                      function_value,gradient,
                                      hessian);
        f_is_valid=true;
        g_is_valid=true;
        h_is_valid=true;
      }
      g=gradient;
      h=hessian;
      return function_value;
    };
  };
}
#endif // DF_REPORT_COLLECTION_HPP
