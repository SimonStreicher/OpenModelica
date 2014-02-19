/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Linköping University,
 * Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3
 * AND THIS OSMC PUBLIC LICENSE (OSMC-PL).
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S
 * ACCEPTANCE OF THE OSMC PUBLIC LICENSE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from Linköping University, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or
 * http://www.openmodelica.org, and in the OpenModelica distribution.
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS
 * OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */

/*
 * Developed by:
 * FH-Bielefeld
 * Developer: Vitalij Ruge
 * Contact: vitalij.ruge@fh-bielefeld.de
 */

#include "../ipoptODEstruct.h"
#include "simulation_data.h"
#include "../../simulation/options.h"

#ifdef WITH_IPOPT

#include "../localFunction.h"

static int set_local_jac_struct(IPOPT_DATA_ *iData, int *nng);
static int local_jac_struct(IPOPT_DATA_ *iData, int *nng);
static int local_jac_struct_print(IPOPT_DATA_ *iData);
static int local_jac_struct_dense(IPOPT_DATA_ *iData, int * nng);
static int check_nominal(IPOPT_DATA_ *iData, double min, double max, double nominal, modelica_boolean set, int i, double x0);
static int optimizer_coeff_setings(IPOPT_DATA_ *iData);
static int optimizer_bounds_setings(DATA *data, IPOPT_DATA_ *iData);
static int optimizer_time_setings(IPOPT_DATA_ *iData);
static int optimizer_print_step(IPOPT_DATA_ *iData);


/*initial*/
static int move_grid(IPOPT_DATA_ *iData);

/*!
 *  free
 *  intarface to simulation
 *  author: Vitalij Ruge
 **/
int destroyIpopt(SOLVER_INFO* solverInfo)
{
  return freeIpoptData( (IPOPT_DATA_*)solverInfo->solverData );
}


/*!
 *  allocate
 *  author: Vitalij Ruge
 **/
int allocateIpoptData(IPOPT_DATA_ *iData)
{
  int deg1, deg2;
  long int i, j;
  int ng = iData->NRes+iData->nc*iData->deg*iData->nsi;
  deg1 = iData->deg + 1;
  deg2 = deg1 + 1;

  iData->nJ = iData->nc + iData->nx;
  iData->gmin = (double*)calloc(ng,sizeof(double));
  iData->gmax = (double*)calloc(ng,sizeof(double));
  iData->mult_g = (double*)malloc(ng*sizeof(double));
  iData->mult_x_L = (double*)malloc(iData->NV*sizeof(double));
  iData->mult_x_U = (double*)malloc(iData->NV*sizeof(double));
  iData->a1 = (double*)malloc(deg1*sizeof(double));
  iData->a2 = (double*)malloc(deg1*sizeof(double));
  iData->a3 = (double*)malloc(deg1*sizeof(double));
  iData->d1 = (double*)malloc(deg2*sizeof(double));
  iData->d2 = (double*)malloc(deg2*sizeof(double));
  iData->d3 = (double*)malloc(deg2*sizeof(double));
  iData->lhs = (double*)malloc((int)iData->nJ*sizeof(double));
  iData->rhs = (double*)malloc((int)iData->nJ*sizeof(double));
  iData->dotx0 = (double*)malloc(iData->nx*sizeof(double));
  iData->dotx1 = (double*)malloc(iData->nx*sizeof(double));
  iData->dotx2 = (double*)malloc(iData->nx*sizeof(double));
  iData->dotx3 = (double*)malloc(iData->nx*sizeof(double));
  iData->xmin = (double*)malloc(iData->nx*sizeof(double));
  iData->xmax = (double*)malloc(iData->nx*sizeof(double));
  iData->umin = (double*)malloc(iData->nu*sizeof(double));
  iData->umax = (double*)malloc(iData->nu*sizeof(double));
  iData->vmin = (double*)malloc(iData->nv*sizeof(double));
  iData->vmax = (double*)malloc(iData->nv*sizeof(double));
  iData->vnom = (double*)malloc(iData->nv*sizeof(double));
  iData->scalVar = (double*)malloc(iData->nv*sizeof(double));
  iData->scalf = (double*)malloc(iData->nx*sizeof(double));
  iData->Vmin = (double*)malloc(iData->NV*sizeof(double));
  iData->Vmax = (double*)malloc(iData->NV*sizeof(double));
  iData->v = (double*)malloc(iData->NV*sizeof(double));
  iData->w = (double*)malloc((iData->nsi + 1)*(iData->nv)*sizeof(double));
  iData->time = (double*)malloc((iData->deg*iData->nsi +1) *sizeof(double));
  iData->start_u = (double*)malloc(iData->nv*sizeof(double));

  iData->J = (double**) malloc(iData->nJ * sizeof(double*));
  for(i = 0; i < iData->nJ; i++)
    iData->J[i] = (double*) calloc(iData->nv, sizeof(double));

  iData->gradF = (double*) calloc(iData->nv, sizeof(double));
  iData->gradF_ = (double*) calloc(iData->nv, sizeof(double));
  iData->gradF0 = (double*) calloc(iData->nv, sizeof(double));
  iData->gradF00 = (double*) calloc(iData->nv, sizeof(double));

  iData->sv = (double*)malloc(iData->nv*sizeof(double));
  iData->sh = (double*)malloc(iData->nJ*sizeof(double));

  iData->vsave = (double*)malloc(iData->nv*sizeof(double));
  iData->eps = (double*)malloc(iData->nv*sizeof(double));

  iData->J0 = (double**) malloc(iData->nJ * sizeof(double*));
  for(i = 0; i < iData->nJ; i++)
    iData->J0[i] = (double*) calloc(iData->nv, sizeof(double));

  iData->gradFomc = (double**) malloc((2) * sizeof(double*));
  for(i = 0; i < 2; i++)
    iData->gradFomc[i] = (double*) calloc(iData->nv, sizeof(double));

  iData->numJ = (double**) malloc(iData->nJ * sizeof(double*));
  for(i = 0; i < iData->nJ; i++)
    iData->numJ[i] = (double*) calloc(iData->nv, sizeof(double));

  iData->H = (long double***) malloc(iData->nJ * sizeof(long double**));
  for(i = 0; i < iData->nJ; i++)
  {
    iData->H[i] = (long double**) malloc(iData->nv* sizeof(long double*));
    for(j = 0; j < iData->nv; j++)
      iData->H[i][j] = (long double*) malloc(iData->nv* sizeof(long double));
  }
  iData->oH = (long double**) malloc(iData->nv * sizeof(long double*));
  for(i = 0; i < iData->nv; i++)
    iData->oH[i] = (long double*) calloc(iData->nv, sizeof(long double));

  iData->mH = (long double**) malloc(iData->nv * sizeof(long double*));
  for(i = 0; i < iData->nv; i++)
    iData->mH[i] = (long double*) calloc(iData->nv, sizeof(long double));

  iData->nlocalJac = 0;
  iData->knowedJ = (int**) malloc( iData->nJ* sizeof(int*));
  for(i = 0; i < iData->nJ; i++)
    iData->knowedJ[i] = (int*) calloc(iData->nv, sizeof(int));

  iData->Hg = (short**) malloc(iData->nv * sizeof(short*));
  for(i = 0; i < iData->nv; i++)
    iData->Hg[i] = (short*) calloc(iData->nv, sizeof(short));

  iData->dt = (double*)malloc((iData->nsi) *sizeof(double));

  if(iData->nc > 0)
    for(i = iData->nx; i<ng; i+=iData->nJ)
      for(j=0;j<iData->nc;++j)
        iData->gmin[i+j] = -1e21;

  return 0;
}

/*!
 *  free
 *  author: Vitalij Ruge
 **/
int freeIpoptData(IPOPT_DATA_ *iData)
{
  int i,j;
  int nJ = (int) iData->nJ;

  for(i = 0; i < nJ; i++){
    free(iData->J[i]);
    free(iData->J0[i]);
    free(iData->numJ[i]);
    free(iData->knowedJ[i]);
  }
  free(iData->J0);
  free(iData->J);
  free(iData->numJ);
  free(iData->knowedJ);

  for(i=0;i<2;++i)
    free(iData->gradFomc[i]);

  for(i = 0; i < iData->nv; i++){
    free(iData->oH[i]);
    free(iData->mH[i]);
    free(iData->Hg[i]);
  }
  free(iData->oH);
  free(iData->mH);
  free(iData->Hg);

  for(i = 0; i < nJ; i++){

    for(j = 0;j<iData->nv; ++j)
      free(iData->H[i][j]);

    free(iData->H[i]);
  }
  free(iData->H);

  free(iData->gradF);
  free(iData->gradF_);
  free(iData->gradF0);
  free(iData->gradF00);
  free(iData->gmin);
  free(iData->gmax);
  free(iData->mult_g);
  free(iData->mult_x_L);
  free(iData->mult_x_U);
  free(iData->a1);
  free(iData->a2);
  free(iData->a3);
  free(iData->d1);
  free(iData->d2);
  free(iData->d3);
  free(iData->dotx0);
  free(iData->dotx1);
  free(iData->dotx2);
  free(iData->dotx3);
  free(iData->xmin);
  free(iData->xmax);
  free(iData->umin);
  free(iData->umax);
  free(iData->vmin);
  free(iData->vmax);
  free(iData->vnom);
  free(iData->scalVar);
  free(iData->scalf);
  free(iData->Vmin);
  free(iData->Vmax);
  free(iData->v);
  free(iData->time);
  free(iData->w);
  free(iData->dt);
  free(iData->lhs);
  free(iData->rhs);
  free(iData->sv);
  free(iData->sh);
  free(iData->start_u);

  for(i = 0; i<3;++i) {
    if(iData->data->simulationInfo.analyticJacobians[i].seedVars){
        free(iData->data->simulationInfo.analyticJacobians[i].seedVars);
        free(iData->data->simulationInfo.analyticJacobians[i].resultVars);
        free(iData->data->simulationInfo.analyticJacobians[i].tmpVars);
        free(iData->data->simulationInfo.analyticJacobians[i].sparsePattern.leadindex);
        free(iData->data->simulationInfo.analyticJacobians[i].sparsePattern.index);
       free(iData->data->simulationInfo.analyticJacobians[i].sparsePattern.colorCols);
    }
  }

  if(ACTIVE_STREAM(LOG_IPOPT_FULL))
  {
    for(i =0; i<iData->nv;++i)
      if(iData->pFile[i])
        fclose(iData->pFile[i]);
    if(iData->pFile)
      free(iData->pFile);
  }

  free(iData);
  iData = NULL;
  return 0;
}

/*!
 *  set data from model
 *  author: Vitalij Ruge
 **/
int loadDAEmodel(DATA *data, IPOPT_DATA_ *iData)
{
  int id;
  double *c;

  iData->nx = data->modelData.nStates;
  iData->nu = data->modelData.nInputVars;
  iData->nc = data->modelData.nOptimizeConstraints;

  iData->nsi = data->simulationInfo.numSteps;
  iData->t0 = data->simulationInfo.startTime;
  iData->tf = data->simulationInfo.stopTime;

  /***********************/
  iData->nX = iData->nx * iData->deg;
  iData->nU = iData->nu * iData->deg;

  iData->NX = iData->nX * iData->nsi + iData->nx;
  iData->NU = iData->nU * iData->nsi + iData->nu;

  iData->nv = iData->nx + iData->nu;
  iData->nV = iData->nX + iData->nU;
  iData->NV = iData->NX + iData->NU;

  iData->nRes = iData->nx*iData->deg;
  iData->NRes = iData->nRes * iData->nsi;

  iData->endN = iData->NV - iData->nv;

  /***********************/
  allocateIpoptData(iData);
  move_grid(iData);
  optimizer_coeff_setings(iData);

  /***********************/
  set_local_jac_struct(iData,&id);
  iData->njac = iData->deg*(iData->nlocalJac-iData->nx+iData->nsi*iData->nlocalJac+iData->deg*iData->nsi*iData->nx)-iData->deg*id;
  iData->nhess = 0.5*iData->nv*(iData->nv + 1)*(1+iData->deg*iData->nsi);

  /***********************/
  iData->x0 = iData->data->localData[1]->realVars;
  optimizer_bounds_setings(data, iData);
  optimizer_time_setings(iData);

  /***********************/
  if(ACTIVE_STREAM(LOG_IPOPT_FULL))
    optimizer_print_step(iData);

  if(ACTIVE_STREAM(LOG_IPOPT_JAC) && ACTIVE_STREAM(LOG_IPOPT_HESSE))
    local_jac_struct_print(iData);

  return 0;
}

/*!
 *  time grid for optimization
 *  author: Vitalij Ruge
 **/
int move_grid(IPOPT_DATA_ *iData)
{
  int i;
  double t;

  t = iData->t0;
  iData->dt_default = (iData->tf - iData->t0)/(iData->nsi);
  for(i=0;i<iData->nsi; ++i){
    iData->dt[i] = iData->dt_default;
    t = (i+1)*iData->dt[i];
  }

  iData->dt[iData->nsi-1] = iData->dt_default + (iData->tf - t );
  assert(iData->nsi>0);
/*
  for(i = 0; i<iData->nsi;++i){
    printf("\n*dt[%i] =%g | %i",i,iData->dt[i],iData->nsi);
  }
*/
  return 0;
}

/*
 *  function calculates a jacobian matrix struct
 *  author: vitalij
 */
static int local_jac_struct_dense(IPOPT_DATA_ *iData, int * nng)
{
  int **J;
  short **Hg;
  int i,j,ii;
  int nJ = (int)(iData->nJ);

  J = iData->knowedJ;
  Hg = iData->Hg;
  //iData->useNumJac == 2

  for(i = 0; i < iData->nJ; ++i)
    for(j = 0; j < iData->nv; ++j){
        J[i][j] = 1.0;
        ++iData->nlocalJac;
       }


  for(i = 0; i <iData->nv; ++i)
    for(j = 0; j < iData->nv; ++j)
      for(ii = 0; ii < nJ; ++ii)
        if(J[ii][i]*J[ii][j])
          Hg[i][j] = 1;

  *nng = iData->nc * iData->nv;
  return 0;
}

/*
 *  function calculates a jacobian matrix struct
 *  author: Willi
 */
static int local_jac_struct(IPOPT_DATA_ *iData, int * nng)
{
  DATA * data = iData->data;
  const int index = 2;
  int **J;
  short **Hg;
  int i,j,l,ii,nx, id;
  int *cC,*lindex;
  int nJ = (int)(iData->nJ);
  id = 0;

  J = iData->knowedJ;
  Hg = iData->Hg;

  nx = data->simulationInfo.analyticJacobians[index].sizeCols;
  cC =  (int*)data->simulationInfo.analyticJacobians[index].sparsePattern.colorCols;
  lindex = (int*)data->simulationInfo.analyticJacobians[index].sparsePattern.leadindex;

  for(i = 1; i < data->simulationInfo.analyticJacobians[index].sparsePattern.maxColors + 1; ++i){
    for(ii = 0; ii<nx; ++ii){
      if(cC[ii] == i){
        data->simulationInfo.analyticJacobians[index].seedVars[ii] = 1.0;
      }
    }

    for(ii = 0; ii < nx; ii++){
      if(cC[ii] == i){
        if(0 == ii)
          j = 0;
        else
          j = lindex[ii-1];

        for(; j<lindex[ii]; ++j){
          l = data->simulationInfo.analyticJacobians[index].sparsePattern.index[j];
          J[l][ii] = 1;
          ++iData->nlocalJac;
          if(l>= iData->nx) id++;
        }
      }
    }

    for(ii = 0; ii<nx; ++ii){
      if(cC[ii] == i){
        data->simulationInfo.analyticJacobians[index].seedVars[ii] = 0.0;
      }
    }
  }

  for(ii = 0; ii < iData->nx; ii++){
    if(J[ii][ii] == 0)
      ++iData->nlocalJac;
    J[ii][ii] = 1.0;
  }


  for(i = 0; i <iData->nv; ++i)
    for(j = 0; j < iData->nv; ++j)
      for(ii = 0; ii < nJ; ++ii)
        if(J[ii][i]*J[ii][j])
          Hg[i][j] = 1;

  *nng = id;
  return 0;
}

static int set_local_jac_struct(IPOPT_DATA_ *iData, int *nng)
{
  char *cflags;
  iData->useNumJac = 0;
  cflags = (char*)omc_flagValue[FLAG_IPOPT_JAC];
  if(cflags){
    if(!strcmp(cflags,"NUM"))
     iData->useNumJac = 1;
   else if(!strcmp(cflags,"SYM") || !strcmp(cflags,"sym"))
     iData->useNumJac = 0;
   else if(!strcmp(cflags,"NUMDENSE") || !strcmp(cflags,"NUMDENSE"))
   iData->useNumJac = 2;
   else
   warningStreamPrint(LOG_STDOUT, 1, "not support ipopt_hesse=%s",cflags);
  }

  if(iData->useNumJac == 2){
  local_jac_struct_dense(iData, nng);
  } else {
    local_jac_struct(iData, nng);
  }
  return 0;
}

static int local_jac_struct_print(IPOPT_DATA_ *iData)
{
  int ii,j;
  int nJ = (int)(iData->nx+iData->nc);
  int **J;
  short **Hg;

  J = iData->knowedJ;
  Hg = iData->Hg;

  printf("\n*****JAC******");
  for(ii = 0; ii < nJ; ++ii){
    printf("\n");
    for(j =0;j<iData->nv;++j)
      printf("%i \t",J[ii][j]);
    printf("\n");
  }
  printf("\n*****HESSE******");
  for(ii = 0; ii < iData->nv; ++ii){
    printf("\n");
    for(j =0;j<iData->nv;++j)
      printf("%i \t",Hg[ii][j]);
    printf("\n");
  }

}


/*!
 *  heuristic for nominal value
 *  author: Vitalij Ruge
 **/
static int check_nominal(IPOPT_DATA_ *iData, double min, double max, double nominal, modelica_boolean set, int i, double x0)
{
  if(set){
    iData->vnom[i] = fmax(fabs(nominal),1e-16);
  }else{
    double amax, amin;
    amax = fabs(max);
    amin = fabs(min);
    iData->vnom[i] = fmax(amax,amin);
    if(iData->vnom[i] > 1e12){
        double tmp = fmin(amax,amin);
        if(tmp<1e12){
          iData->vnom[i] = fmax(tmp,x0);
        }else{
          iData->vnom[i] = 1.0 + x0;
        }
      }
      
    iData->vnom[i] = fmax(iData->vnom[i],1e-16);
  }
  return 0;
}


/*!
 *  seting collocation coeff
 *  author: Vitalij Ruge
 **/
static int optimizer_coeff_setings(IPOPT_DATA_ *iData)
{
  if(iData->deg == 3){
     iData->c1 = 0.15505102572168219018027159252941086080340525193433;
     iData->c2 = 0.64494897427831780981972840747058913919659474806567;
     iData->c3 = 1.0;

     iData->e1 = 0.27639320225002103035908263312687237645593816403885;
     iData->e2 = 0.72360679774997896964091736687312762354406183596115;
     iData->e3 = 1.0;

     iData->a1[0] = 4.1393876913398137178367408896470696703591369767880;
     iData->a1[1] = 3.2247448713915890490986420373529456959829737403284;
     iData->a1[2] = 1.1678400846904054949240412722156950122337492313015;
     iData->a1[3] = 0.25319726474218082618594241992157103785758599484179;

     iData->a2[0] = 1.7393876913398137178367408896470696703591369767880;
     iData->a2[1] = 3.5678400846904054949240412722156950122337492313015;
     iData->a2[2] = 0.7752551286084109509013579626470543040170262596716;
     iData->a2[3] = 1.0531972647421808261859424199215710378575859948418;

     iData->a3[0] = 3.0;
     iData->a3[1] = 5.5319726474218082618594241992157103785758599484179;
     iData->a3[2] = 7.5319726474218082618594241992157103785758599484179;
     iData->a3[3] = 5.0;

     iData->d1[0] = 4.3013155617496424838955952368431696002490512113396;
     iData->d1[1] = 3.6180339887498948482045868343656381177203091798058;
     iData->d1[2] = 0.8541019662496845446137605030969143531609275394172;
     iData->d1[3] = 0.17082039324993690892275210061938287063218550788345;
     iData->d1[4] = 0.44721359549995793928183473374625524708812367192230;
     iData->invd1_4 = 2.2360679774997896964091736687312762354406183596115;

     iData->d2[0] = 3.3013155617496424838955952368431696002490512113396;
     iData->d2[1] = 5.8541019662496845446137605030969143531609275394172;
     iData->d2[2] = 1.3819660112501051517954131656343618822796908201942;
     iData->d2[3] = 1.1708203932499369089227521006193828706321855078834;
     iData->d2[4] = 0.44721359549995793928183473374625524708812367192230;

     iData->d3[0] = 7.0;
     iData->d3[1] = 11.180339887498948482045868343656381177203091798058;
     iData->d3[2] = 11.180339887498948482045868343656381177203091798058;
     iData->d3[3] = 7.0;
     iData->d3[4] = 1.0;

     iData->bl[0] = 0.083333333333333333333333333333333333333333333333333;
     iData->bl[1] = 0.41666666666666666666666666666666666666666666666667;
     iData->bl[2] = iData->bl[1];
     iData->bl[3] = 1.0 - (iData->bl[0]+iData->bl[1]+iData->bl[2]);

     iData->br[2] = 0.11111111111111111111111111111111111111111111111111;
     iData->br[1] = 0.51248582618842161383881344651960809422127631890713;
     iData->br[0] = 1.0 - (iData->br[1] + iData->br[2]);
    }

  return 0;
}


/*!
 *  seting vars bounds
 *  author: Vitalij Ruge
 **/
static int optimizer_bounds_setings(DATA *data, IPOPT_DATA_ *iData)
{
  int i, j;
  modelica_boolean *tmp = (modelica_boolean*)malloc(iData->nv*sizeof(modelica_boolean));
  char **tmpname = (char**)malloc(iData->nv*sizeof(char*));
  double *start = iData->start_u;

  for(i =0;i<iData->nx;++i){
    check_nominal(iData, data->modelData.realVarsData[i].attribute.min, data->modelData.realVarsData[i].attribute.max, data->modelData.realVarsData[i].attribute.nominal, data->modelData.realVarsData[i].attribute.useNominal, i, fabs(iData->x0[i]));
    iData->scalVar[i] = 1.0 / iData->vnom[i];
    iData->scalf[i] = iData->scalVar[i];

    iData->xmin[i] = data->modelData.realVarsData[i].attribute.min*iData->scalVar[i];
    iData->xmax[i] = data->modelData.realVarsData[i].attribute.max*iData->scalVar[i];
  }

  iData->data->callback->pickUpBoundsForInputsInOptimization(data,iData->umin, iData->umax, &iData->vnom[iData->nx], tmp, tmpname, start);

  if(ACTIVE_STREAM(LOG_IPOPT)){
	char buffer[200];
    for(i=0; i<iData->nx; ++i){

    	if (iData->xmin[i] > -1e20)
    	  sprintf(buffer, ", min = %g", data->modelData.realVarsData[i].attribute.min);
    	else
    	  sprintf(buffer, ", min = -Inf");

    	printf("\nState[%i]:%s(start = %g, nominal = %g%s",i, iData->data->modelData.realVarsData[i].info.name, data->modelData.realVarsData[i].attribute.start, iData->vnom[i], buffer);

    	if (iData->xmax[i] < 1e20)
    	  sprintf(buffer, ", max = %g", data->modelData.realVarsData[i].attribute.max);
    	else
    	  sprintf(buffer, ", max = +Inf");

    	printf("%s",buffer);
    	printf(", init = %g)",iData->data->localData[1]->realVars[i]);
    }

    for(; i<iData->nv; ++i){

    	if ((double)iData->umin[i-iData->nx] > -1e20)
    	  sprintf(buffer, ", min = %g", iData->umin[i-iData->nx]);
    	else
    	  sprintf(buffer, ", min = -Inf");

    	printf("\nInput[%i]:%s(start = %g, nominal = %g%s",i, tmpname[i-iData->nx] ,start[i-iData->nx], iData->vnom[i], buffer);

    	if (iData->umax[i-iData->nx]*iData->scalVar[i] < 1e20)
    	  sprintf(buffer, ", max = %g", iData->umax[i-iData->nx]);
    	else
    	  sprintf(buffer, ", max = +Inf");

    	printf("%s)",buffer);
    }

    if(iData->nc > 0)
      printf("\nnumber of constraints: %ld", iData->nc);
  }

  for(i =0,j = iData->nx;i<iData->nu;++i,++j){
    check_nominal(iData, iData->umin[i], iData->umax[i], iData->vnom[j], tmp[i], j, fabs(start[i]));
    iData->scalVar[j] = 1.0 / iData->vnom[j];
    iData->umin[i] *= iData->scalVar[j];
    iData->umax[i] *= iData->scalVar[j];
    iData->start_u[i] = fmin(fmax(iData->start_u[i], iData->umin[i]), iData->umax[i]);
  }


  memcpy(iData->vmin, iData->xmin, sizeof(double)*iData->nx);
  memcpy(iData->vmin + iData->nx, iData->umin, sizeof(double)*iData->nu);

  memcpy(iData->vmax, iData->xmax, sizeof(double)*iData->nx);
  memcpy(iData->vmax + iData->nx, iData->umax, sizeof(double)*iData->nu);

  memcpy(iData->Vmin, iData->vmin, sizeof(double)*iData->nv);
  memcpy(iData->Vmax, iData->vmax, sizeof(double)*iData->nv);

  for(i = 0,j = iData->nv; i < iData->nsi*iData->deg;i++, j += iData->nv){
  memcpy(iData->Vmin + j, iData->vmin, sizeof(double)*iData->nv);
    memcpy(iData->Vmax + j, iData->vmax, sizeof(double)*iData->nv);
  }

  free(tmp);
  free(tmpname);
  return 0;
}


/*!
 *  seting time vector
 *  author: Vitalij Ruge
 **/
static int optimizer_time_setings(IPOPT_DATA_ *iData)
{
  int i,k,id;
  iData->time[0] = iData->t0;
  if(iData->deg == 3){
    for(i = 0,k=0,id=0; i<iData->nsi; ++i,id += iData->deg){
      if(i){
        if(iData->deg == 3){
        iData->time[++k] = iData->time[id] + iData->c1*iData->dt[i];
        iData->time[++k] = iData->time[id] + iData->c2*iData->dt[i];
      }
        iData->time[++k] = (i+1)*iData->dt[i];
    }else{
      if(iData->deg == 3){
        iData->time[++k] = iData->time[id] + iData->e1*iData->dt[i];
        iData->time[++k] = iData->time[id] + iData->e2*iData->dt[i];
      }
      iData->time[++k] = (i+1)*iData->dt[i];
    }
  }
  }
  iData->time[k] = iData->tf;
  return 0;
}

/*!
 *  file for optimizer steps
 *  author: Vitalij Ruge
 **/
static int optimizer_print_step(IPOPT_DATA_ *iData)
{
  char buffer[4096];
  int i,j,k;
  iData->pFile = (FILE**) calloc(iData->nv, sizeof(FILE*));
  for(i=0, j=0; i<iData->nv; i++, ++j){
    if(j <iData->nx)
      sprintf(buffer, "./%s_ipoptPath_states.csv", iData->data->modelData.realVarsData[j].info.name);
    else if(j< iData->nv)
      sprintf(buffer, "./%s_ipoptPath_input.csv", "u");

    iData->pFile[j] = fopen(buffer, "wt");
    fprintf(iData->pFile[j], "%s,", "iteration");
  }


  for(i = 0 ,k = 0,j = 0; i<iData->NV; ++i,++j){
    if(j >= iData->nv){
      j = 0; ++k;
    }

    if(j < iData->nx)
      fprintf(iData->pFile[j], "%s_%i,", iData->data->modelData.realVarsData[j].info.name, k);
    else if(j < iData->nv)
      fprintf(iData->pFile[j], "%s_%i,", "u", k);
  }
  return 0;
}

#endif
