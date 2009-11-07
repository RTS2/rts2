//
// (C) 2009-10-25, Markus Wildi markus.wildi@one-arcsec.org
// 
// Fit the FWHM, FLUX_MAX of a focus run
// needs root above and version 5.0.25:  http://root.cern.ch/
// 
// 
// g++ -Wall -I `root-config --incdir` -o rts2_fit_focus rts2_fit_focus.C `root-config --libs`
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2, or (at your option)
//   any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software Foundation,
//   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//   Or visit http://www.gnu.org/licenses/gpl.html.

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <stddef.h>

#include <TF1.h>
#include <TH1.h>
#include <TH2.h>
#include <TMath.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include "TStyle.h" //gStyle
#include <TCanvas.h>
#include <TPaveText.h>
#include <TPad.h>
#include <TRandom.h>
#include <TPaveStats.h>
#include <TApplication.h>
#include <TPaveLabel.h>

using namespace std;

// need the pointer in function one_over_fwhm
TF1 *fit_fwhm_global ;

// the following comment is for historical purpose
// v[0] : variable
// par[]: parameters to be fitted
// par[0]: normalising constant
// par[1]: mean
// par[2]: sigma
// Double_t my_gaus(Double_t *v, Double_t *par)
// {
//   // printf( "my_gaus %f\n", fit_fwhm_global-> Eval( 1500., 0.,0.,0.)) ;
//   Double_t arg = 0;
//   if (par[2] != 0)
//     {
//       arg = (v[0] - par[1])/par[2];
//     }
//   Double_t fitval = par[0] * TMath::Exp(-0.5*arg*arg);
//   return fitval;
// }

//f(x) = p0 + 0*x + p2*x^2 + 0*p3*x^3 + p4*x^4
//f(x)=  p0       + p1*x^2            + p2*x^4

// y_symmetric_pol4: NO data is asymmetric
//Double_t y_symmetric_pol4(Double_t *v, Double_t *par)
//{
//  Double_t fitval= par[0]+ par[1]*v[0]*v[0]+ par[2]* v[0]*v[0]*v[0]*v[0] ;
//  return fitval;
//}

Double_t one_over_fwhm(Double_t *v, Double_t *par)
{
  //get the fwhm at foc_pos from the first fit
  Double_t fwhm= fit_fwhm_global-> Eval( v[0], 0.,0.,0.) ;
  // 0: norm, 1: shift against fit of fwhm, 2: exponent, meaning not yet understood
  Double_t fitval = par[0]/TMath::Power((fwhm- par[1]), par[2]);
  return fitval;
}

int main(int argc, char* argv[])
{
  FILE *fp ;
  char line[255];
  int   number_of_lines_fwhm=0 ;
  int   number_of_lines_flux=0 ;
  Float_t foc_pos_i ;
  Float_t fwhm_i, flux_i ;
  Float_t foc_pos_fwhm[1000], foc_pos_flux[1000] ;
  Float_t fwhm[1000], flux[1000] ;
  char *mode            = argv[1] ;
  char *filter          = argv[2] ;
  char *date            = argv[3] ;
  char *objects         = argv[4] ;
  char *fwhm_file       = argv[5] ;
  char *flux_file       = argv[6] ;
  char *output_file_name= argv[7] ;
  TApplication* rootapp ;

  // (non-)interactive mode
  // After the call of this costructor: argc=1, argv=NULL 
  if( !strcmp( mode, "1"))
    {
      rootapp = new TApplication("fit_focus",&argc, argv);
    }
  // read the FWHM, FLUX_MAX files 
    fp  = fopen( fwhm_file,"r") ;
 NEXTLINE_FWHM:    while (fgets( line, 80, fp)) 
      {
      int ret=sscanf( line, "%e %e ", &foc_pos_i, &fwhm_i) ;
      if( ret < 2) 
	{
	  printf( "FWHM Error: too few elements\n") ; 
	  goto NEXTLINE_FWHM ;
	}
      else
	{
	  foc_pos_fwhm[number_of_lines_fwhm]= foc_pos_i;
	  fwhm[number_of_lines_fwhm]= fwhm_i ;
	  number_of_lines_fwhm++ ;
	}
    }
  fclose( fp) ;
  
  if( number_of_lines_fwhm== 0)
    {
      printf( "no data found in %s, exiting\n", fwhm_file) ;
      return 1 ;
    }

  fp  = fopen( flux_file,"r") ;
 NEXTLINE_FLUX:    while (fgets( line, 80, fp)) 
    {
      int ret=sscanf( line, "%e %e ", &foc_pos_i, &flux_i) ;
      if( ret < 2) 
	{
	  printf( "FLUX Error: too few elements\n") ; 
	  goto NEXTLINE_FLUX ;
	}
      else
	{
	  foc_pos_flux[number_of_lines_flux]= foc_pos_i;
	  flux[number_of_lines_flux]= flux_i ;
	  number_of_lines_flux++ ;
	}
    }

  fclose( fp) ;

  if( number_of_lines_fwhm== 0)
    {
      printf( "no data found in %s, exiting\n", fwhm_file) ;
      return 1 ;
    }
  if( number_of_lines_flux== 0)
    {
      printf( "no data found in %s, exiting\n", flux_file) ;
      return 1 ;
    }

  gStyle-> SetOptFit();

  TCanvas *result = new TCanvas("rts2_autofocus","rts2_autofocus",200,10,800,640);
  result-> SetGrid();

  TPaveText* title = new TPaveText(.2,0.96,.8,.995);
  title-> SetBorderSize(0);

  char title_str[256] ;
  strcpy( title_str, "rts2-autofocus, ") ;
  strcat( title_str, filter) ;
  strcat( title_str, ", ") ;
  strcat( title_str, date) ;
  strcat( title_str, ", objects ") ;
  strcat( title_str, objects) ;

  title-> AddText( title_str);
  title-> Draw();
  
  // ToDo: how to deal with: Warning in <Minimize>: TLinearFitter failed in finding the solution
  //                          *** Break *** segmentation violation
  TMultiGraph *mg = new TMultiGraph("rts2_autofocus","");
  // create first graph
  TGraph *gr1 = new TGraph(number_of_lines_fwhm,foc_pos_fwhm,fwhm);
  gr1-> SetMarkerColor(kBlue);
  gr1-> SetMarkerStyle(21);
  //NO: data is asymmetric! TF1 *fit_fwhm = new TF1("y_symmetric_pol4", y_symmetric_pol4, 0., 10000., 3);
  //xsfit_fwhm_global = fit_fwhm ; // in root there is no method accepting a function pointer
  //fit_fwhm-> SetParameters(0., 1., 1.);
  //fit_fwhm-> SetParNames("p0","p1", "p2");
  //gr1-> Fit(fit_fwhm,"q") ;
  gr1-> Fit("pol4","q");

  mg-> Add(gr1);

  // read the results
  // ToDo: integrate at least chi2 into rts2-autofocus
  TF1 *fit_fwhm = gr1-> GetFunction("pol4");
  fit_fwhm_global = fit_fwhm ; // in root there is no method accepting a function pointer
 
  Double_t fwhm_chi2   = fit_fwhm-> GetChisquare();
  Double_t fwhm_p0     = fit_fwhm-> GetParameter(0);
  Double_t fwhm_p0_err = fit_fwhm-> GetParError(0);
  Double_t fwhm_p1     = fit_fwhm-> GetParameter(1);
  Double_t fwhm_p1_err = fit_fwhm-> GetParError(1);
  Double_t fwhm_p2     = fit_fwhm-> GetParameter(2);
  Double_t fwhm_p2_err = fit_fwhm-> GetParError(2);
  Double_t fwhm_p3     = fit_fwhm-> GetParameter(3);
  Double_t fwhm_p3_err = fit_fwhm-> GetParError(4);
  Double_t fwhm_p4     = fit_fwhm-> GetParameter(4);
  Double_t fwhm_p4_err = fit_fwhm-> GetParError(4);
  printf( "Result fwhm: chi2 %e,  p(0...4)=(%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e)\n", fwhm_chi2, fwhm_p0, fwhm_p0_err, fwhm_p1, fwhm_p1_err, fwhm_p2, fwhm_p2_err, fwhm_p3, fwhm_p3_err, fwhm_p4, fwhm_p4_err) ;

  Double_t fwhm_MinimumX = fit_fwhm-> GetMinimumX() ; 
  printf( "FWHM_FOCUS %f\n",fwhm_MinimumX) ; 
  printf( "FWHM parameters p0...p2 %e %e %e, chi2 %e\n",fwhm_p0, fwhm_p1, fwhm_p2, fwhm_chi2) ; 
  
  // create second graph
  // assuming constant flux, the (absolute) value of the maximum of the gaussian is solely a function of FWHM
  TGraph *gr2 = new TGraph(number_of_lines_flux,foc_pos_flux,flux);
  gr2-> SetMarkerColor(kRed);
  gr2-> SetMarkerStyle(20);
  TF1 *fit_flux = new TF1("one_over_fwhm",one_over_fwhm, 0., 10000., 3);
  fit_flux-> SetParameters(10., 1., 1.);
  fit_flux-> SetParNames("constant","offset", "exponent");
  gr2-> Fit(fit_flux,"q");

  mg-> Add(gr2);
  mg-> Draw("ap");

  Double_t flux_chi2   = fit_flux-> GetChisquare();
  Double_t flux_p0     = fit_flux-> GetParameter(0);
  Double_t flux_p0_err = fit_flux-> GetParError(0);
  Double_t flux_p1     = fit_flux-> GetParameter(1);
  Double_t flux_p1_err = fit_flux-> GetParError(1);
  Double_t flux_p2     = fit_flux-> GetParameter(2);
  Double_t flux_p2_err = fit_flux-> GetParError(2);
  Double_t flux_p3     = fit_flux-> GetParameter(3);
  Double_t flux_p3_err = fit_flux-> GetParError(4);
  Double_t flux_p4     = fit_flux-> GetParameter(4);
  Double_t flux_p4_err = fit_flux-> GetParError(4);
  Double_t flux_p5     = fit_flux-> GetParameter(5);
  Double_t flux_p5_err = fit_flux-> GetParError(5);
  printf( "Result flux: chi2 %e,  p(0...5)=(%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e)\n", flux_chi2, flux_p0, flux_p0_err, flux_p1, flux_p1_err, flux_p2, flux_p2_err, flux_p3, flux_p3_err, flux_p4, flux_p4_err, flux_p5, flux_p5_err) ;

  // ToDo: this does not provide the correct result (its the minimum of the fwhm function !)
  // Double_t flux_MaximumX = flux-> GetMaximumX( fwhm_MinimumX-1000., fwhm_MinimumX+1000.) ; 
  // printf( "FLUX_FOCUS %f\n", flux_MaximumX ) ; 
  // using instead:
  printf( "FLUX_FOCUS %f\n", fwhm_MinimumX + flux_p0) ; 
  
  //printf( "FLUX_FOCUS p0...p2 %f %f %f, chi2 %f\n",flux_p0, flux_p1, flux_p2, flux_chi2) ; 

  // make the plot nicer
  // must follow mg-> Draw
  mg-> GetXaxis()->SetTitle("FOC_POS [tick]");
  mg-> GetYaxis()->SetTitle("FWHM (blue) [px],   FLUX_MAX [a.u.]");

  //force drawing of canvas to generate the fit TPaveStats
  result-> Update();
  TPaveStats *stats1 = (TPaveStats*)gr1-> FindObject("stats") ;
  TPaveStats *stats2 = (TPaveStats*)gr2-> FindObject("stats") ;
  
  stats1-> SetTextColor(kBlue); 
  stats2-> SetTextColor(kRed); 

  stats1-> SetX1NDC(0.12); stats1-> SetX2NDC(0.32); stats1-> SetY1NDC(0.75); stats1-> SetY2NDC(0.95);
  stats2-> SetX1NDC(0.67); stats2-> SetX2NDC(0.87); stats2-> SetY1NDC(0.75); stats2-> SetY2NDC(0.95);

  result-> Modified();
  // save result as a PNG file 
  result-> SaveAs(output_file_name);
  // (non-)interactive mode
  if( !strcmp( mode, "1"))
    {
      rootapp-> Run();
    }
  return 0;
}
