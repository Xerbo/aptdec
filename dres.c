#include <stdlib.h>
#include <math.h>

float vcoef(float av, float at, float bv, float bt)
{
float dv,dt;
float f;

#define K 500

dv=av-bv;
dt=at-bt;

f=dv*dv+dt*dt;

return (1.0/(1.0+K*f));
}

void dres(
          float av, float at, float bv, float bt,
          float cv, float ct, float dv, float dt,
          float *v , float *t)
{
float wab,wac,wda,wbc,wbd,wcd;

wab=vcoef(av,at,bv,bt);
wac=vcoef(av,at,cv,ct);
wda=vcoef(dv,dt,av,at);
wbc=vcoef(bv,bt,cv,ct);
wbd=vcoef(bv,bt,dv,dt);
wcd=vcoef(cv,ct,dv,dt);

*v=(wab*(av+bv)+wac*(av+cv)+wda*(dv+av)+wbc*(bv+cv)+wbd*(bv+dv)+wcd*(cv+dv))/(2.0*(wab+wac+wda+wbc+wbd+wcd));
*t=(wab*(at+bt)+wac*(at+ct)+wda*(dt+at)+wbc*(bt+ct)+wbd*(bt+dt)+wcd*(ct+dt))/(2.0*(wab+wac+wda+wbc+wbd+wcd));
}

