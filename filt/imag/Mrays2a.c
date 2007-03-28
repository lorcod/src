/* Ray tracing in VTI media by a Runge-Kutta integrator.

Takes: > rays.rsf

Rays can be plotted with sfplotrays.
*/
/*
  Copyright (C) 2004 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <math.h>

#include <rsf.h>

#include "araytrace.h"

int main(int argc, char* argv[])
{
    bool velocity;
    int is, n[2], im, nm, order, nshot, ndim;
    int nt, nt1, nr, ir, it, i;
    float dt, da=0., a0, amax, v0;
    float x[2], p[2], d[2], o[2], **traj, *vz2, *vx2, *q, **s, *a;
    char *velfile;
    araytrace rt;
    sf_file shots, vz, vx, eta, rays, angles;

    sf_init (argc,argv);
    vz = sf_input("in");
    rays = sf_output("out");

    /* get 2-D grid parameters */
    if (!sf_histint(vz,"n1",n))     sf_error("No n1= in input");
    if (!sf_histint(vz,"n2",n+1))   sf_error("No n2= in input");
    if (!sf_histfloat(vz,"d1",d))   sf_error("No d1= in input");
    if (!sf_histfloat(vz,"d2",d+1)) sf_error("No d2= in input");
    if (!sf_histfloat(vz,"o1",o))   o[0]=0.;
    if (!sf_histfloat(vz,"o2",o+1)) o[1]=0.;

    /* additional parameters */
    if(!sf_getbool("vel",&velocity)) velocity=true;
    /* If y, input is velocity; if n, slowness */
    if(!sf_getint("order",&order)) order=4;
    /* Interpolation order */

    if (!sf_getint("nt",&nt)) sf_error("Need nt=");
    /* Number of time steps */
    if (!sf_getfloat("dt",&dt)) sf_error("Need dt=");
    /* Sampling in time */

    /* get shot locations */
    if (NULL != sf_getstring("shotfile")) {
	/* file with shot locations */
	shots = sf_input("shotfile");
	if (!sf_histint(shots,"n1",&ndim) || 2 != ndim) 
	    sf_error("Must have n1=2 in shotfile");
	if (!sf_histint(shots,"n2",&nshot)) 
	    sf_error("No n2= in shotfile");
  
	s = sf_floatalloc2 (ndim,nshot);
	sf_floatread(s[0],ndim*nshot,shots);
	sf_fileclose (shots);
    } else {
	nshot = 1;
	ndim = 2;

	s = sf_floatalloc2 (ndim,nshot);

	if (!sf_getfloat("zshot",&s[0][0]))   s[0][0]=0.;
	/* shot coordinates (if no shotfile) */
	if (!sf_getfloat("yshot",&s[0][1])) s[0][1]=o[1] + 0.5*(n[1]-1)*d[1];
	
	sf_warning("Shooting from z=%f, x=%f",s[0][0],s[0][1]);
    }

    if (NULL != sf_getstring("anglefile")) {
	/* file with initial angles */
	angles = sf_input("anglefile");
	
	if (!sf_histint(angles,"n1",&nr)) sf_error("No n1= in anglefile");
    } else {
	angles = NULL;

	if (!sf_getint("nr",&nr)) sf_error("Need nr=");
	/* number of angles (if no anglefile) */
	if (!sf_getfloat("a0",&a0)) a0 = 0.;
	/* minimum angle (if no anglefile) */
	if (!sf_getfloat("amax",&amax)) amax=360.;
	/* maximum angle (if no anglefile) */

	/* convert degrees to radians */
	a0 = a0*SF_PI/180.;
	amax = amax*SF_PI/180.;

	/* figure out angle spacing */
	da = (nr > 1)? (amax - a0)/(nr-1) : 0.;
    }

    a = sf_floatalloc(nr);

    /* specify output dimensions */
    nt1 = nt+1;
    sf_putint (rays,"n1",nt1);
    sf_putint (rays,"n2",nr);
    sf_putint (rays,"n3",nshot);
    sf_settype (rays,SF_COMPLEX);
    sf_fileflush (rays,NULL);
    sf_settype (rays,SF_FLOAT);

    /* get velocities */
    nm = n[0]*n[1];
    vz2 = sf_floatalloc(nm);
    sf_floatread(vz2,nm,vz);

    for(im = 0; im < nm; im++) {
	v0 = vz2[im];
	vz2[im] = velocity? v0*v0: 1./(v0*v0);
    }

    if (NULL != (velfile = sf_getstring("vx"))) {
	/* horizontal velocity or slowness */
	vx = sf_input(velfile);
	vx2 = sf_floatalloc(nm);
	sf_floatread(vx2,nm,vx);

	for(im = 0; im < nm; im++) {
	    v0 = vx2[im];
	    vx2[im] = velocity? v0*v0: 1./(v0*v0);
	}
	free(velfile);
	sf_fileclose(vx);
    } else {
	vx2 = NULL;
    }

    
    if (NULL != (velfile = sf_getstring("eta"))) {
	/* eta parameter */
	eta = sf_input(velfile);
	q = sf_floatalloc(nm);
	sf_floatread(q,nm,eta);

        /* transform eta to q */
	for(im = 0; im < nm; im++) {
	    q[im] = 1./(1.+2.*q[im]);
	}

	free(velfile);
	sf_fileclose(eta);
    } else {
	q = NULL;
    }

    /* initialize ray tracing object */
    rt = araytrace_init (2, nt, dt, n, o, d, vz2, vx2, q, order);    
    free (vz2);
    if (NULL != vx2) free (vx2);
    if (NULL != q) free (q);
    
    traj = sf_floatalloc2 (2*ndim,nt1);

    for( is = 0; is < nshot; is++) { /* loop over shots */
	/* initialize angles */
	if (NULL != angles) {
	    sf_floatread(a,nr,angles);
	} else {
	    for (ir = 0; ir < nr; ir++) {
		a[ir] = a0+da*ir;
	    }
	}

	for (ir = 0; ir < nr; ir++) { /* loop over rays */
	    /* initialize position */
	    x[0] = s[is][0]; 
	    x[1] = s[is][1];

	    /* initialize direction */
	    p[0] = -cosf(a[ir]);
	    p[1] = sinf(a[ir]);

	    it = trace_aray (rt, x, p, traj);
	    if (it < 0) it = -it; /* keep side-exiting rays */
	    
	    for (i=0; i < nt1; i++) {
		if (0==it || it > i) {
		    sf_floatwrite (traj[i],ndim,rays);
		} else {
		    sf_floatwrite (traj[it],ndim,rays);
		}
	    }
	}
    }

    exit (0);
}

/* 	$Id: Mrays2a.c 1850 2006-05-18 10:06:45Z fomels $	 */
