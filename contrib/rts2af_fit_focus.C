//
// (C) 2009-10-25, Markus Wildi markus.wildi@one-arcsec.org
// 
// Fit the FWHM, FLUX_MAX of a focus run
// needs root above and version 5.0.25:  http://root.cern.ch/
// 
// 
// g++ -Wall -I `root-config --incdir` -o rts2_fit_focus rts2af_fit_focus.C `root-config --libs`
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
#include <TGraphErrors.h>
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
Double_t fwhm_at_minimum ;
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
// provides reasonable results
// Double_t one_over_fwhm(Double_t *v, Double_t *par)
// {
//   //get the fwhm at foc_pos from the first fit
//   Double_t fwhm= fit_fwhm_global-> Eval( v[0], 0.,0.,0.) ;
//   // 0: norm, 1: shift against fit of fwhm, 2: exponent, meaning not yet understood
//   Double_t fitval = par[0]/TMath::Power((fwhm- par[1]), par[2]);
//   return fitval;
// }
// tan(alpha)=  D/2  / focal length 
// y_foc_pos= y_foc_pos_at_minimum * FWHM_at_minimum / ( FWHM_at_minimum + tan(alpha) * (foc_pos - offset)**par[3])
// par[3] : tan(alpha)
// par[2] : no physical explanation
Double_t one_over_fwhm(Double_t *v, Double_t *par)
{
  Double_t fitval = par[0] * par[4] / ( par[4] + par[3] * TMath::Power(  TMath::Abs((v[0]- par[1])), par[2]) ) ;
  return fitval;
}
Double_t one_over_fwhm_temp(Double_t *v, Double_t *par)
{
  Double_t fitval = par[0] * fwhm_at_minimum / ( fwhm_at_minimum + par[3] * TMath::Power(  TMath::Abs((v[0]- par[1])), par[2]) ) ;
  return fitval;
}
Double_t pol2a(Double_t *v, Double_t *par)
{

  Double_t fitval = par[0] + par[1] * v[0] + par[2] * v[0] * v[0] ;
  return fitval;
}
Double_t pol3a(Double_t *v, Double_t *par)
{

  Double_t fitval = par[0] + par[1] * v[0] + par[2] * v[0] * v[0] + par[3]  * v[0] * v[0] * v[0];
  return fitval;
}


// read the FWHM, FLUX_MAX files 
int read_file( char *file, Float_t *foc_pos, Float_t *fv, Float_t *ex, Float_t *ey)
{
  char line[255];
  FILE *fp ;
  int   number_of_lines=0 ;
  Float_t foc_pos_i, f_i, ex_i, ey_i ;

  fp  = fopen( file,"r") ;
 NEXTLINE:    while (fgets( line, 80, fp)) 
    {
      int ret=sscanf( line, "%e %e %e %e", &foc_pos_i, &f_i, &ex_i, &ey_i) ;
      if( ret < 4) 
	{
	  printf( "rts2_fit_foc: too few elements on line %d\n", number_of_lines+1) ; 
	  goto NEXTLINE ;
	}
      else
	{
	  foc_pos[number_of_lines]= foc_pos_i;
	  fv[number_of_lines]= f_i ;
	  ex[number_of_lines]= ex_i ;
	  ey[number_of_lines]= ey_i ;
	  number_of_lines++ ;
	}
    }
  fclose( fp) ;

  return number_of_lines ;
}

int main(int argc, char* argv[])
{
  int   number_of_lines_fwhm=0 ;
  int   number_of_lines_flux=0 ;
  char  focStr[32] ;
  Float_t foc_pos_fwhm[1000], foc_pos_flux[1000] ;
  Float_t fwhm[1000], flux[1000] ;
  Float_t fwhm_errx[1000], fwhm_erry[1000] ;
  Float_t flux_errx[1000], flux_erry[1000] ;
  char *mode            = argv[1] ;
  char *filter          = argv[2] ;
  char *temperature     = argv[3] ;
  char *date            = argv[4] ;
  char *objects         = argv[5] ;
  char *fwhm_file       = argv[6] ;
  char *flux_file       = argv[7] ;
  char *output_file_name= argv[8] ;
  TApplication* rootapp ;
  // (non-)interactive mode
  // After the call of this costructor: argc=1, argv=NULL 
  if( !strcmp( mode, "1"))
    {
      rootapp = new TApplication("fit_focus",&argc, argv);
    }
  if(( number_of_lines_fwhm= read_file( fwhm_file, &foc_pos_fwhm[0], &fwhm[0], &fwhm_errx[0], &fwhm_erry[0])) == 0)
    {
      printf( "rts2af-fit-focus: no data found in %s, exiting\n", fwhm_file) ;
      return 1 ;
    }
  if(( number_of_lines_flux= read_file( flux_file, &foc_pos_flux[0], &flux[0], &flux_errx[0], &flux_erry[0])) == 0)
    {
      printf( "rts2af-fit-focus: no data found in %s, exiting\n", flux_file) ;
      return 1 ;
    }
  // make fit results visible
  gStyle-> SetOptFit();

  TCanvas *result = new TCanvas("rts2af","rts2af",200,10,800,640);
  result-> SetGrid();

  TPaveText* title = new TPaveText(.2,0.96,.8,.995);
  title-> SetBorderSize(0);

  char title_str[256] ;
  strcpy( title_str, "rts2af, ") ;
  strcat( title_str, filter) ;
  strcat( title_str, ", ") ;
  strcat( title_str, temperature) ;
  strcat( title_str, ", ") ;
  strcat( title_str, date) ;
  strcat( title_str, ", ob ") ;
  strcat( title_str, objects) ;

  //title-> AddText( title_str);
  //title-> Draw();
  
  // ToDo: how to deal with: Warning in <Minimize>: TLinearFitter failed in finding the solution
  //                          *** Break *** segmentation violation
  TMultiGraph *mg = new TMultiGraph("rts2af","");
  // create first graph: FWHM
  
  TGraphErrors *gr1 = new TGraphErrors(number_of_lines_fwhm,foc_pos_fwhm,fwhm, fwhm_errx, fwhm_erry);
  //TGraph *gr1 = new TGraph(number_of_lines_fwhm,foc_pos_fwhm,fwhm);
  gr1-> SetMarkerColor(kBlue);
  gr1-> SetMarkerStyle(21);
  //NO: data is asymmetric! TF1 *fit_fwhm = new TF1("y_symmetric_pol4", y_symmetric_pol4, 0., 10000., 3);
  //xsfit_fwhm_global = fit_fwhm ; // in root there is no method accepting a function pointer
  //fit_fwhm-> SetParameters(0., 1., 1.);
  //fit_fwhm-> SetParNames("p0","p1", "p2");
  //gr1-> Fit(fit_fwhm,"q") ;
  //  gr1-> Fit("pol2a","q");

  //TF1 *func = new TF1("fit",pol3a,0,100000,4);
  //   1  p0           1.85925e+03   9.55633e+02   4.57281e-04  -7.14746e-04
  //   2  p1          -6.80267e-01   3.50029e-01  -1.67476e-07  -3.80031e+00
  //   3  p2           6.14710e-05   3.16327e-05   1.51334e-11  -2.02273e+04
  //   4  p3           1.50936e-10   7.60539e-11   3.63948e-17  -1.05825e+08
  //  func->SetParameters( 1.85925e+03, -6.80267e-01, 6.14710e-05, 1.50936e-10);
  //func->SetParameters( 1.85925e+04, 0.80267e-02, 0., 0.);

  TF1 *func = new TF1("fit",pol3a,0,100000,4);
  func->SetParameters( 180., -9.0e-02, 1.0e-05, 0.);
  gr1->Fit("fit", "qWEM");

  func->SetParameters( gr1-> GetFunction("fit")->GetParameter(0), gr1-> GetFunction("fit")->GetParameter(1), gr1-> GetFunction("fit")->GetParameter(2), gr1-> GetFunction("fit")->GetParameter(3));
  gr1->Fit("fit", "q");

  mg-> Add(gr1);

  // read the results
  TF1 *fit_fwhm = gr1-> GetFunction("fit");
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

  // date and temperature of the run
  fprintf( stderr, "rts2af-fit-focus: date: %s\n", date);
  fprintf( stderr, "rts2af-fit-focus: temperature: %s\n", temperature);
  fprintf( stderr, "rts2af-fit-focus: objects: %s\n", objects);

  fprintf( stderr, "rts2af-fit-focus: result fwhm: chi2 %e,  p(0...4)=(%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e)\n", fwhm_chi2, fwhm_p0, fwhm_p0_err, fwhm_p1, fwhm_p1_err, fwhm_p2, fwhm_p2_err, fwhm_p3, fwhm_p3_err, fwhm_p4, fwhm_p4_err) ;

  Double_t fwhm_MinimumX = fit_fwhm-> GetMinimumX() ;

  
  fwhm_at_minimum= fit_fwhm-> GetMinimum() ; 

  fprintf( stderr, "rts2af-fit-focus: FWHM_FOCUS %f, FWHM at Minimum %f\n",fwhm_MinimumX, fwhm_at_minimum) ; 
  printf( "FWHM_FOCUS %f\n",fwhm_MinimumX) ; 
  printf( "FWHM parameters: chi2 %e, p0...p2 %e %e %e\n", fwhm_chi2, fwhm_p0, fwhm_p1, fwhm_p2) ; 
  
  sprintf(focStr, "%5d", (int)fwhm_MinimumX) ;
  strcat( title_str, ", fw ") ;
  strcat( title_str, focStr) ;


  // create second graph: FLUX
  // assuming constant flux, the (absolute) value of the maximum of the gaussian is solely a function of FWHM
  // ToDO: FLUX_MAX, FLUX_APER see rts2af.py 
  // used if errors will be availabe
  //  TGraphErrors *gr2 = new TGraphErrors(number_of_lines_flux,foc_pos_flux,flux, flux_errx, flux_erry);
  TGraph *gr2 = new TGraph(number_of_lines_flux,foc_pos_flux,flux);

  gr2-> SetMarkerColor(kRed);
  gr2-> SetMarkerStyle(20);
  TF1 *fit_flux = new TF1("one_over_fwhm",one_over_fwhm, 0., 10000., 5);
  fit_flux-> SetParameters(100.,      fwhm_MinimumX, 2.5,        0.072,         1000. * fwhm_at_minimum); // a little bit of magic here
  fit_flux-> SetParNames(  "constant","offset",      "exponent", "tan(alpha)", "fwhm_at_minimum");
  gr2-> Fit(fit_flux,"q");

  Double_t flux_chi2   = fit_flux-> GetChisquare();
  Double_t flux_p0     = fit_flux-> GetParameter(0); //constant
  Double_t flux_p0_err = fit_flux-> GetParError(0);
  Double_t flux_p1     = fit_flux-> GetParameter(1); //offset"
  Double_t flux_p1_err = fit_flux-> GetParError(1);
  Double_t flux_p2     = fit_flux-> GetParameter(2); //exponent
  Double_t flux_p2_err = fit_flux-> GetParError(2);
  Double_t flux_p3     = fit_flux-> GetParameter(3);
  Double_t flux_p3_err = fit_flux-> GetParError(4);
  Double_t flux_p4     = fit_flux-> GetParameter(4);
  Double_t flux_p4_err = fit_flux-> GetParError(4);
  Double_t flux_p5     = fit_flux-> GetParameter(5);
  Double_t flux_p5_err = fit_flux-> GetParError(5);
  fprintf(stderr,  "rts2af-fit-focus: result flux: chi2 %e,  p(0...5)=(%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e), (%e +/- %e)\n", flux_chi2, flux_p0, flux_p0_err, flux_p1, flux_p1_err, flux_p2, flux_p2_err, flux_p3, flux_p3_err, flux_p4, flux_p4_err, flux_p5, flux_p5_err) ;

  // ToDo: this does not provide the correct result (its the minimum of the fwhm function !)
  // Double_t flux_MaximumX = flux-> GetMaximumX( fwhm_MinimumX-1000., fwhm_MinimumX+1000.) ; 
  // printf( "FLUX_FOCUS %f\n", flux_MaximumX ) ; 
  // using instead:
  fprintf( stderr, "rts2af-fit-focus: FLUX_FOCUS %f\n", flux_p1) ; 
  printf( "FLUX_FOCUS %f\n", flux_p1) ; 
  
  printf( "FLUX parameters: chi2 %e, p0...p2 %e %e %e\n", flux_chi2, flux_p0, flux_p1, flux_p2) ; 

  sprintf(focStr, "%5d", (int)flux_p1) ;
  strcat( title_str, ", fl ") ;
  strcat( title_str, focStr) ;

  title-> AddText( title_str);
  title-> Draw();

  mg-> Add(gr2);
  mg-> Draw("ap");

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
  fprintf( stderr, "rts2af-fit-focus: parameters mode: %s, filter: %s, temperature: %s, date: %s, objects: %s, %s, %s, %s\n", mode, filter, temperature, date, objects, fwhm_file, flux_file, output_file_name) ; 

  return 0;
}
