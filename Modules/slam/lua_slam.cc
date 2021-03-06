/* NOTE: Torch C access begins with 0, not 1 as in Torch's Lua access */
#include <lua.hpp>
#include <torch/luaT.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <torch/TH/TH.h>
#ifdef __cplusplus
}
#endif

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <stdbool.h>
#include <float.h>
#ifdef DEBUG
#include <iostream>
#endif

#define SKIP 1
#define DEFAULT_RESOLUTION 0.05
#define DEFAULT_INV_RESOLUTION 20

// TODO: Maybe not allocate these arrays immediately...

/* Candidate indices helpers for speed */
unsigned int xis[1081];
unsigned int yis[1081];
/* Laser points in map coordinates */
double lxs_map[1081];
double lys_map[1081];

// For updating the map
const int range_max = 255;
const int range_min = 0;

/* Store the min and max values of the map */
/* TODO: either make as a module, or pass as arguments. */
/* TODO: multiple maps of various resolution? */
double xmin, ymin, xmax, ymax;
double invxmax, invymax;
double res = DEFAULT_RESOLUTION, invRes = DEFAULT_INV_RESOLUTION;

#define MT_NAME "connected_mt"
typedef struct structConnected {
  long start; // C index of the first element
	long end; // C index of the first element
} structConnected;

int lua_connected_components(lua_State *L) {
	THFloatTensor *returns_t = (THFloatTensor *) luaT_checkudata(L, 1, "torch.FloatTensor");
	THArgCheck(returns_t->nDimension == 1, 1, "tensor must have one dimensions");
	long n = returns_t->size[0];
	float* returns_ptr = returns_t->storage->data;
	double threshold = luaL_checknumber(L, 2);
	//
	lua_newtable(L);
	// New table for first connected component
	lua_createtable(L, 0, 3);
	lua_pushstring(L, "diff");
  lua_pushnumber(L, 0);
  lua_settable(L, -3);
	lua_pushstring(L, "start");
  lua_pushnumber(L, 1);
	lua_settable(L, -3);
	//
	float* cur_ptr = returns_ptr;
	float curVal = *cur_ptr, prevVal, diffVal;
	cur_ptr++;
	//
	int i, ncc = 1;
	int prevInd = 0;
	for(i=1;i<n;i++){
		prevVal = curVal;
		curVal = *cur_ptr;
		diffVal = curVal - prevVal;
		if( fabs(diffVal)>threshold ){
			// For the previous table
			lua_pushstring(L, "stop");
			lua_pushnumber(L, i);
			lua_settable(L, -3);
			//lua_pushstring(L, "size");
			//lua_pushnumber(L, i-prevInd);
			//lua_settable(L, -3);
			lua_rawseti(L, -2, ncc);
			prevInd = i;
			// New table for next connected component
			ncc++;
			lua_createtable(L, 0, 3);
			lua_pushstring(L, "diff");
			lua_pushnumber(L, diffVal);
			lua_settable(L, -3);
			lua_pushstring(L, "start");
			lua_pushnumber(L, i+1);
			lua_settable(L, -3);
		}
		cur_ptr++;
	}
	// For the previous table
	lua_pushstring(L, "stop");
	lua_pushnumber(L, i);
	lua_settable(L, -3);
	//lua_pushstring(L, "size");
	//lua_pushnumber(L, i-prevInd);
	//lua_settable(L, -3);
	// Index in the master table
	lua_rawseti(L, -2, ncc);

	// Return this table
	return 1;
}

/* Set the resolution of the map */
int lua_set_resolution(lua_State *L) {
	res = luaL_checknumber(L, 1);
	invRes = 1.0 / res;
	return 0;
}

/* Set the boundaries for scan matching and map updating */
int lua_set_boundaries(lua_State *L) {
	xmin = luaL_checknumber(L, 1);
	ymin = luaL_checknumber(L, 2);
	xmax = luaL_checknumber(L, 3);
	ymax = luaL_checknumber(L, 4);
	invymax = ymax * DEFAULT_INV_RESOLUTION;
	invxmax = xmax * DEFAULT_INV_RESOLUTION;
	return 0;
}

int lua_grow_map(lua_State *L) {
	THDoubleTensor *cost_t = (THDoubleTensor *) luaT_checkudata(L, 1, "torch.DoubleTensor");
	THArgCheck(cost_t->nDimension == 2, 1, "tensor must have two dimensions");
	int r_i = luaL_checkint(L, 2);
	int r_j = luaL_checkint(L, 3);
	long m = cost_t->size[0];
	long n = cost_t->size[1];
	//long size = m*n;
	THDoubleTensor *grown_t = THDoubleTensor_newClone(cost_t);
	double* grown_ptr = grown_t->storage->data;
	double* cur_ptr = grown_ptr;
	for (long i = 0; i<m; i++){
		for (long j = 0; j<n; j++){
			cur_ptr++;
			if(i<r_i||i>m-r_i||j<r_j||j>n-r_j) continue;
			double c = THTensor_fastGet2d( cost_t, i, j );
			if(c>127){
				for(long b = -r_j; b<r_j; b++){
					for(long a = 1; a<r_i; a++){
						double* ptr = cur_ptr + a*n + b;
						if(c>*ptr) *ptr = c;
						ptr = cur_ptr - a*n + b;
						if(c>*ptr) *ptr = c;
					}
				}
			}
		}
	}

	luaT_pushudata(L, grown_t, "torch.DoubleTensor");
	return 1;
}

/* 2D scan match, with resulting maximum correlation information */
// Input: 2D bytemap and search ranges
// Output: likehoods tensor
int lua_match(lua_State *L) {

	/* Get the map, which is a ByteTensor */
	const THByteTensor * map_t =
		(THByteTensor *) luaT_checkudata(L, 1, "torch.ByteTensor");
	/* Grab the xs and ys from the last laser scan*/
	const THDoubleTensor * lY_t =
		(THDoubleTensor *) luaT_checkudata(L, 2, "torch.DoubleTensor");
	/* Grab the scanning values for theta, x, y */
	const THDoubleTensor * pths_t =
		(THDoubleTensor *) luaT_checkudata(L, 3, "torch.DoubleTensor");
	const THDoubleTensor * pxs_t =
		(THDoubleTensor *) luaT_checkudata(L, 4, "torch.DoubleTensor");
	const THDoubleTensor * pys_t =
		(THDoubleTensor *) luaT_checkudata(L, 5, "torch.DoubleTensor");
	/* Prior on where we are now (zero position) */
	double prior = luaL_optnumber(L, 6, 0);

	/* Grab the number of points to process */
	unsigned int nps = lY_t->size[0];
	THArgCheck(lY_t->size[1]==2, 1, "Proper laser points");

	// Assume offset of zero for map
	uint8_t * map_ptr = map_t->storage->data;
	const int sizex = map_t->size[0];
	const int sizey = map_t->size[1];
	const int map_xstride = map_t->stride[0];
	// Assume ystride of 1
	//const unsigned int map_ystride = map_t->stride[1];

	/* The number of laser points and number of candidate x/y to match */
	unsigned int npxs, npys, npths;
	unsigned int th_stride = pths_t->stride[0];

	npths = pths_t->size[0];
	npxs = pxs_t->size[0];
	npys = pys_t->size[0];

	// Make the likliehoods storage
	THDoubleTensor *likelihoods_t = THDoubleTensor_newWithSize3d(npths,npxs,npys);
	// zero it...
	THDoubleTensor_zero( likelihoods_t );
	double* likestorage =
		likelihoods_t->storage->data + likelihoods_t->storageOffset;
	long nlikes = likelihoods_t->storage->size;

	// Add the prior to the likelihoods
	long mid_th = floor(npths/2);
	long mid_x  = floor(npxs/2);
	long mid_y  = floor(npys/2);
	THTensor_fastSet3d(likelihoods_t,mid_th,mid_x,mid_y,prior);

	/* Precalculate the indices for candidate x and y */
	unsigned int ii=0;
	//TODO: invRes should not be semaphored... that would be slow
#pragma omp parallel default(shared) private(ii) firstprivate(invRes)
	{
#pragma omp sections nowait
		{ /* sections */
#pragma omp section
			{
				double* pxs_ptr = pxs_t->storage->data + pxs_t->storageOffset;
				unsigned int* tmpx = xis;
				unsigned int xstride1 = pxs_t->stride[0];
				for( ii=0; ii<npxs; ii++ ){
					*tmpx = ( *pxs_ptr - xmin )*invRes;
					pxs_ptr += xstride1;
					tmpx++;
				}
			} /*section*/
#pragma omp section
			{
				double* pys_ptr = pys_t->storage->data + pys_t->storageOffset;
				unsigned int* tmpy = yis;
				unsigned int ystride1 = pys_t->stride[0];
				for(unsigned int ii=0;ii<npys;ii++){
					*tmpy = ( *pys_ptr - ymin )*invRes;
					pys_ptr+=ystride1;
					tmpy++;
				}
			}/*section*/
#pragma omp section
			{
				/* Convert the laser points to the map coordinates */
				double* pls_ptr = lY_t->storage->data + lY_t->storageOffset;
				unsigned int pstride = lY_t->stride[0];
				double * tmplx = lxs_map, * tmply = lys_map;
				for(unsigned int ii=0;ii<nps;ii++){
					*tmplx = *(pls_ptr) * invRes;
					*tmply = *(pls_ptr+1) * invRes;
					pls_ptr += pstride;
					tmplx++;
					tmply++;
				}
			} /*section*/
		}/*sections*/
	}/*omp parallel*/


	/* Loop indices */
	/* Iterate over all search angles */
	/* Shared variables for each thread */
	/* Private variables to be used within each thread */
	unsigned int * tmp_xi, * tmp_yi;
	double * tmp_lx_map, * tmp_ly_map, * tmp_like_with;
	uint8_t * map_ptr_with_x;
	unsigned int pi, pyi, pxi, xi, yi, pthi;
	double theta, costh, sinth, lx_map, ly_map, x_map, y_map;
	/* Variables for each distribution */
	double* th_ptr = pths_t->storage->data;
	double* tmp_like = likestorage;

	for ( pthi=0; pthi < npths; pthi++ ) {

		/* Matrix transform for each theta */
		theta = *th_ptr;
		costh = cos( theta );
		sinth = sin( theta );
		th_ptr += th_stride;

		/* Reset the pointers to the candidate points and liklihoods */
		tmp_xi = xis;
		tmp_yi = yis;

		/* Reset the pointers to the laser points */
		tmp_lx_map = lxs_map;
		tmp_ly_map = lys_map;
		/* Iterate over all laser points */
		// TODO: +=2 or adaptive filtering of the laser points
		for ( pi=0; pi<nps; pi+=SKIP ) {

			/* Grab the laser readings in map coordinates */
			/* Rotate them by the candidate theta */
			/* TODO: Do in bulk with torch? */

			lx_map = *tmp_lx_map;
			ly_map = *tmp_ly_map;
			x_map = lx_map*costh - ly_map*sinth;
			y_map = lx_map*sinth + ly_map*costh;
			/* Increment the pointer */
			tmp_lx_map+=SKIP;
			tmp_ly_map+=SKIP;

			/* Iterate over all candidate x's */
			tmp_like_with = tmp_like;
			tmp_xi = xis;
			for ( pxi=0; pxi<npxs; pxi++ ) {
				/* Use unsigned int - don't have to check < 0 */
				/* TODO: is this really a safe assumption at map edges? */
				xi = x_map + *(tmp_xi++);
				/*
					 int xi_old = x_map + (THTensor_fastGet1d(pxs_t,pxi)-xmin)*invRes;
					 printf("xi: %d -> xi: %u\n",xi_old,xi);
					 */
				if ( xi >= sizex ) {
					tmp_xi ++;
					tmp_like_with ++;
					continue;
				}
				map_ptr_with_x = map_ptr + xi*map_xstride;

				/* Iterate over all search y's */
				tmp_yi = yis;
				for ( pyi=0; pyi<npys; pyi++ ) {
					/* Use unsigned int - don't have to check < 0 */
					yi = y_map + *(tmp_yi++);
					/*
						 int yi_old = y_map + (THTensor_fastGet1d(pys_t,pyi)-ymin)*invRes;
						 printf("yi: %d -> yi: %u\n",yi_old,yi);
						 */
					if ( yi >= sizey ){
						tmp_like_with ++;
						continue;
					}
					double update_value = *( map_ptr_with_x + yi );
					//if(update_value<200) continue;
					// Increment likelihoods
					*tmp_like_with += update_value;
					// Increment likelihood pointer
					tmp_like_with ++;

				} /* For pose ys */
			} /* For pose xs */
		} /* For laser scan points */
		/* Update the liklihood pointers AFTER each iteration */
		tmp_like = tmp_like_with;
	} /* For yaw values */

	/* Initialize max correlation value */
	double hmax = 0;
	double* tmplikestorage = likestorage; // Assume 0 for storageOffset
	int ilikestorage = 0;
	/* TODO: Use OpenMP to find the max */
	// http://stackoverflow.com/questions/978222/openmp-c-algorithms-for-min-max-median-average
	// http://msdn.microsoft.com/en-us/magazine/cc163717.aspx#S6
	for(unsigned int ii=0;ii<nlikes;ii++){
		//#pragma omp critical
		{
			if( *tmplikestorage > hmax ){
				ilikestorage = ii;
				hmax = *tmplikestorage;
			}
		}
		tmplikestorage++;
	}
	long like_stride0 = likelihoods_t->stride[0];
	long like_stride1 = likelihoods_t->stride[1];
	long ithmax = ilikestorage / like_stride0;
	long ixmax  = (ilikestorage - ithmax*like_stride0) / like_stride1;
	long iymax  = ilikestorage - ithmax*like_stride0 - ixmax*like_stride1;

	/* Push the likelihood positions */
	luaT_pushudata(L, likelihoods_t, "torch.DoubleTensor");

	/* Push the indices of the maximum value */
	/* Lua indices start at 1, rather than 0, so +1 each index */
	lua_createtable(L, 0, 4);
	lua_pushinteger(L,ithmax+1);
	lua_setfield(L, -2, "a");
	lua_pushinteger(L,ixmax+1);
	lua_setfield(L, -2, "x");
	lua_pushinteger(L,iymax+1);
	lua_setfield(L, -2, "y");
	/* Push maximum correlation value to Lua */
	lua_pushnumber(L,hmax);
	lua_setfield(L, -2, "hits");

	return 2;
}

/* Update the map with the laser scan points */
int lua_update_map(lua_State *L) {

	/* Get the map, which is a ByteTensor */
	const THByteTensor * map_t =
		(THByteTensor *) luaT_checkudata(L, 1, "torch.ByteTensor");
	// Check contiguous
	THArgCheck(map_t->stride[1] == 1, 1, "Improper memory layout (j)");
	const long map_istride = map_t->stride[0];
	THArgCheck(map_istride == map_t->size[1], 1, "Improper memory layout (i)");

	/* Grab the updated points */
	const THDoubleTensor * pts_t =
		(THDoubleTensor *) luaT_checkudata(L, 2, "torch.DoubleTensor");
	THArgCheck(pts_t->size[1]==2, 2, "Proper laser points");

	/* Grab the increment value */
	const int inc = luaL_checknumber(L, 3);

	/* Get the updating table: sets which items were updated */
	const THDoubleTensor * update_t =
		(THDoubleTensor *) luaT_checkudata(L, 4, "torch.DoubleTensor");
	THArgCheck(update_t->stride[1] == 1, 4, "Improper memory layout (j)");
	THArgCheck(update_t->stride[0] == update_t->size[1], 4, "Improper memory layout (i)");
	THArgCheck(map_istride == update_t->stride[0], 4, "Improper memory layout (i)");
	
	/* Grab the timestamp */
	const double t = luaL_checknumber(L, 5);

	double * update_ptr = (double *)(update_t->storage->data + update_t->storageOffset);
	uint8_t * map_ptr = (uint8_t *)(map_t->storage->data + map_t->storageOffset);
	double* pts_ptr = (double *)(pts_t->storage->data + pts_t->storageOffset);

	const long nps  = pts_t->size[0];
	const int pts_stride = pts_t->stride[0];

	/*
	// TODO: Check the map size...?
	const long sizex = map_t->size[0];
	const long sizey = map_t->size[1];
	*/

	double x, y;
	int i, xi, yi, mapLikelihood;
	long map_idx;
	double * update_ptr_tmp;
	uint8_t* map_ptr_tmp;
	for( i=0; i<nps; i++ ) {
		// Grab the cartesian coordinates of the point
		x = *(pts_ptr);
		y = *(pts_ptr+1);
		pts_ptr += pts_stride;

		// If out of range...
		if( x>xmax || y>ymax || x<xmin || y<ymin ) continue;

		/* TODO: ceil or floor these? map_t bounds check? */
		xi = ( x - xmin ) * invRes;
		yi = ( y - ymin ) * invRes;

		// Get the index on the map (same as update map)
		map_idx = xi * map_istride + yi;
		update_ptr_tmp = update_ptr + map_idx;

		/* Check if this cell has been updated before */
		if( *update_ptr_tmp >=t ) continue;

		// Find the current likelihood
		map_ptr_tmp = map_ptr + map_idx;
		mapLikelihood = *map_ptr_tmp + inc;

		/* --------------------------- */
		/* bit hack for range limit */
		/* http://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax */
		/* newVal = min(mapLikelihood, 255) */
		mapLikelihood = range_max ^ ((mapLikelihood ^ range_max) & -(mapLikelihood < range_max));
		mapLikelihood = mapLikelihood ^ ((mapLikelihood ^ range_min) & -(mapLikelihood < range_min));
		/* --------------------------- */

		// Update the maps
		*map_ptr_tmp = mapLikelihood;
		*update_ptr_tmp = t;
	}
	return 0;
}

/* Decay the map around a region from the robot  */
int lua_decay_map(lua_State *L) {

	/* Get the torch that contains the map data */
	/* NOTE: This is a sub-map around the robot... stride is important now! */
	const THByteTensor * map_t =
		(THByteTensor *) luaT_checkudata(L, 1, "torch.ByteTensor");
	uint8_t * map_ptr = (uint8_t *)(map_t->storage->data + map_t->storageOffset);

	/* Grab the threshold on likelihood */
	const double thres = luaL_checknumber(L, 2);

	/* Grab by how much to decay the map */
	const double decay_factor = luaL_checknumber(L, 3);

	/* Get the map properties */
	const long ystride = map_t->stride[1];
	THArgCheck(ystride == 1, 1, "Improper memory layout (j)");
	const long xstride = map_t->stride[0];
	const long nps_x = map_t->size[0];
	const long nps_y = map_t->size[1];

	uint8_t* cur_map = map_ptr;
	int i=0, j=0;
	uint8_t val = 0;

	// Loop through the submap
	for( i=0; i<nps_x; i++ ) {
		for ( j=0; j<nps_y; j++){
			val = *(cur_map+j);
			if ( val>=thres ){
				/* If super certain: remain at high level */
				*(cur_map+j) = thres;
			} else {
				/* Decay */
				*(cur_map+j) = val * decay_factor;
			}
		} /* for j */
		cur_map += xstride;
	} /* for i */

	return 0;
}








	template<typename TT>
int isContiguous(TT *self)
{
	long z = 1;
	int d;
	for(d = self->nDimension-1; d >= 0; d--)
	{
		if(self->size[d] != 1)
		{
			if(self->stride[d] == z)
				z *= self->size[d];
			else
				return 0;
		}
	}
	return 1;
}

/* Update the height map with the laser scan points */
/* Currently use same resolution and size as OMAP*/
int lua_update_hmap(lua_State *L) {
	static int i = 0;
	static double x=0,y=0, height=0;
	static unsigned int xi = 0, yi = 0;

	/* Get the map, which is a ByteTensor */
	const THByteTensor * map_t =
		(THByteTensor *) luaT_checkudata(L, 1, "torch.ByteTensor");
#ifdef DEBUG
	int is_con = isContiguous<THByteTensor>((THByteTensor *)map_t);
	if (!is_con)
		return luaL_error(L, "map_t input must be contiguous");
#endif
	uint8_t * map_tp = (uint8_t *)(map_t->storage->data + map_t->storageOffset);


	/* Grab the updated points */
	const THDoubleTensor * ps_t =
		(THDoubleTensor *) luaT_checkudata(L, 2, "torch.DoubleTensor");
	const long nps  = ps_t->size[0];
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)ps_t);
	if (!is_con)
		return luaL_error(L, "ps_t input must be contiguous");
#endif
	double * ps_tp = (double *)(ps_t->storage->data + ps_t->storageOffset);

	/* Grab the height value */
	//const double height = luaL_checknumber(L, 3);

	//#pragma omp parallel for
	double *ps_tp_temp = NULL;
	for( i=0; i<nps; i++ ) {
		/*
			 x = THTensor_fastGet2d( ps_t, i, 0 );
			 y = THTensor_fastGet2d( ps_t, i, 1 );
			 height = THTensor_fastGet2d( ps_t, i, 2 );
			 */
		ps_tp_temp = ps_tp + i * ps_t->stride[0];
		x = *(ps_tp_temp++);
		y = *(ps_tp_temp++);
		height = *(ps_tp_temp++);

		if( x>xmax || y>ymax || x<xmin || y<ymin ) continue;

		/*  and OMAP.data_update[ xis[i] ][ yis[i] ]==0 */
		/*  OMAP.data_update[ xis[i] ][ yis[i] ] = 1 */
		/* TODO: ceil or floor these? map_t bounds check? */
		xi = (unsigned long)( ( x - xmin ) * invRes );
		yi = (unsigned long)( ( y - ymin ) * invRes );

		//currentHeight = THTensor_fastGet2d(map_t,xi,yi);

		//if (height > currentHeight){
		/* THTensor_fastSet2d( map_t, xi, yi, height); */
		map_tp[xi * map_t->stride[0] + yi]= height;
		//}

	}
	return 0;
}


/************************************************************************/


int lua_binStats(lua_State *L)
{
	static std::vector<int> Count;
	static std::vector<double> sumY, sumYY, maxY, minY;
	static double * prXp = NULL;
	static double * prYp = NULL;
	static double * prBp = NULL;
	static double * prTp = NULL;

	/* Grab the inputs */

	// #1 input: Distances from chest lidar
	const THDoubleTensor * prX =
		(THDoubleTensor *) luaT_checkudata(L, 1, "torch.DoubleTensor");
	const long nX  = prX->size[0];

#ifdef DEBUG
	int is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)prX);
	if (!is_con)
		return luaL_error(L, "prX input must be contiguous");
#endif
	prXp = (double *)(prX->storage->data + prX->storageOffset);

	// #2 input: Heights from chest lidar
	const THDoubleTensor * prY =
		(THDoubleTensor *) luaT_checkudata(L, 2, "torch.DoubleTensor");
	const long nY  = prY->size[0];

#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)prY);
	if (!is_con)
		return luaL_error(L, "prY input must be contiguous");
#endif
	prYp = (double *)(prY->storage->data + prY->storageOffset);

	// #3 input: Number of bins
	const int n = luaL_checkinteger(L, 3);

	// #4 input: Bin Table containing: "count", "mean", "max", "min", "std"
	const THDoubleTensor * prT =
		(THDoubleTensor *) luaT_checkudata(L, 4, "torch.DoubleTensor");
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)prT);
	if (!is_con)
		return luaL_error(L, "prT input must be contiguous");
#endif
	prTp = (double *)(prT->storage->data + prT->storageOffset);

	// #5 input: Bins
	const THDoubleTensor * prB =
		(THDoubleTensor *) luaT_checkudata(L, 5, "torch.DoubleTensor");
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)prB);
	if (!is_con)
		return luaL_error(L, "prB input must be contiguous");
#endif
	prBp = (double *)(prB->storage->data + prB->storageOffset);

	//fprintf(stdout, "nX, nY, n, BinTable, Bins: %ld \t %ld \t %d \t %ldx%ld \t %ldx%ld \n"
	//	,nX, nY, n, prT->size[0], prT->size[1], prB->size[0], prB->size[1]);

	if (nX != nY)
		luaL_error(L, "Number of elements in inputs should match");


	/* Initialize statistics vectors */
	if ( (int)Count.size() != n) {
		Count.resize(n);
		sumY.resize(n);
		sumYY.resize(n);
		maxY.resize(n);
		minY.resize(n);
	}

	for (int i = 0; i < n; i++) {
		Count[i] = 0;
		sumY[i] = 0;
		sumYY[i] = 0;
		maxY[i] = -__FLT_MAX__;
		minY[i] = __FLT_MAX__;
	}


	/* Calculate all the variables needed for the statistics of the points */		
	double tmpX, tmp;
	for (int i = 0; i < nX; i++) {
		tmpX = *(prXp + i);
		int j = round(tmpX) - 1;
		if ((j >= 0) && (j < n)) {
			Count[j]++;
			tmp = *(prYp + i);
			sumY[j] += tmp;
			sumYY[j] += tmp*tmp;
			if (tmp > maxY[j]) maxY[j] = tmp;
			if (tmp < minY[j]) minY[j] = tmp;
			prBp[i] = j + 1;
		}
	} 


	/* Calculate the statistics of each bin and set the returning Tensor */
	double mean = 0, std = 0, max = 0, min = 0;    
	double * prTp_temp = NULL;
	int * Countp = &(*Count.begin());
	for (int i = 0; i < n; i++) { 
		mean = 0; std = 0; max = 0; min = 0;
		if (Count[i] > 0) {
			mean = sumY[i]/Count[i];
			std = sqrt((sumYY[i]-sumY[i]*sumY[i]/Count[i])/Count[i]);
			max = maxY[i];
			min = minY[i];
		} 	  
		/*
			 THTensor_fastSet2d(prT, i, 0, Count[i]);
			 THTensor_fastSet2d(prT, i, 1, mean);
			 THTensor_fastSet2d(prT, i, 2, max);
			 THTensor_fastSet2d(prT, i, 3, min);
			 THTensor_fastSet2d(prT, i, 4, std);
			 */
		prTp_temp = prTp + i * prT->stride[0];
		*(prTp_temp++) = *(Countp ++);
		*(prTp_temp++) = mean;
		*(prTp_temp++) = max;
		*(prTp_temp++) = min;
		*(prTp_temp++) = std;

	}

	return 0;
}



/* Get the ground points from chest lidar points */
int lua_get_ground_points(lua_State *L) {
	static int i = 0;
	static double x=0,y=0,z=0,w=0;

	/* Get the torch to set */
	const THDoubleTensor * ps_x =
		(THDoubleTensor *) luaT_checkudata(L, 1, "torch.DoubleTensor");
#ifdef DEBUG
	int is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)ps_x);
	if (!is_con)
		return luaL_error(L, "ps_x input must be contiguous");
#endif
	double * ps_xp = (double *)(ps_x->storage->data + ps_x->storageOffset);

	/* Grab the lidar points */
	const THDoubleTensor * ps_t =
		(THDoubleTensor *) luaT_checkudata(L, 2, "torch.DoubleTensor");
	const long nps_t  = ps_t->size[0];
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)ps_t);
	if (!is_con)
		return luaL_error(L, "ps_t input must be contiguous");
#endif
	double * ps_tp = (double *)(ps_t->storage->data + ps_t->storageOffset);

	/* Grab the total number of points to set */
	const double nps = luaL_checknumber(L, 3);

	//#pragma omp parallel for
	double * ps_tp_temp = NULL;
	double * ps_xp_temp = NULL;
	for( i=0; i<nps; i++ ) {
		/*
			 x = THTensor_fastGet2d( ps_t, nps_t-i-1, 0 );
			 y = THTensor_fastGet2d( ps_t, nps_t-i-1, 1 );
			 z = THTensor_fastGet2d( ps_t, nps_t-i-1, 2 );
			 w = THTensor_fastGet2d( ps_t, nps_t-i-1, 3 );
			 */
		ps_tp_temp = ps_tp + (nps_t-i-1) * ps_t->stride[0];
		x = *(ps_tp_temp++);
		y = *(ps_tp_temp++);
		z = *(ps_tp_temp++);
		w = *(ps_tp_temp++);

		ps_xp_temp = ps_xp + i * ps_x->stride[0];
		*(ps_xp_temp++) = x;
		*(ps_xp_temp++) = y;
		*(ps_xp_temp++) = z;
		*(ps_xp_temp++) = w;

	}
	return 0;
}



/* Get the points from chest lidar points 
	 to update the height map later */
int lua_get_height_points(lua_State *L) {
	static int i = 0;
	static double x=0,y=0,z=0,w=0; 
	double height = 0;

	/* Get the torch to set */
	const THDoubleTensor * ps_x =
		(THDoubleTensor *) luaT_checkudata(L, 1, "torch.DoubleTensor");
#ifdef DEBUG
	int is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)ps_x);
	if (!is_con)
		return luaL_error(L, "ps_x input must be contiguous");
#endif
	double * ps_xp = (double *)(ps_x->storage->data + ps_x->storageOffset);

	/* Grab the lidar points */
	const THDoubleTensor * ps_t =
		(THDoubleTensor *) luaT_checkudata(L, 2, "torch.DoubleTensor");
	const long nps_t  = ps_t->size[0];
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)ps_t);
	if (!is_con)
		return luaL_error(L, "ps_t input must be contiguous");
#endif
	double * ps_tp = (double *)(ps_t->storage->data + ps_t->storageOffset);

	/* Grab the minimum height of the height map */
	const double min_height = luaL_checknumber(L, 3);

	/* Grab the maximum height of the height map */
	const double max_height = luaL_checknumber(L, 4);

	/* Grab the chest lidar height */
	const double offset = luaL_checknumber(L, 5);

	//#pragma omp parallel for
	double * ps_tp_temp = NULL;
	double * ps_xp_temp = NULL;
	for( i=0; i<nps_t; i++ ) {
		ps_tp_temp = ps_tp + i * ps_t->stride[0];
		x = *(ps_tp_temp++);
		y = *(ps_tp_temp++);
		z = *(ps_tp_temp++);
		w = *(ps_tp_temp++);

		/* Transformation to range [0,255] */
		ps_xp_temp = ps_xp + i * ps_x->stride[0];
		if (z >= max_height - offset){
			*(ps_xp_temp++) = x;
			*(ps_xp_temp++) = y;
			*(ps_xp_temp++) = 255;
			*(ps_xp_temp++) = w;
			continue;
		}
		else if (z <= min_height - offset) {
			*(ps_xp_temp++) = x;
			*(ps_xp_temp++) = y;
			*(ps_xp_temp++) = 0;
			*(ps_xp_temp++) = w;
			continue;
		}

		height = (z + offset - min_height)*(255/(max_height - min_height));

		ps_xp_temp = ps_xp + i * ps_x->stride[0];
		*(ps_xp_temp++) = x;
		*(ps_xp_temp++) = y;
		*(ps_xp_temp++) = height;
		*(ps_xp_temp++) = w;

	}
	return 0;
}


/* Get the chest lidar points and find the gnd pts and obs pts  */
int lua_mask_points(lua_State *L) {
	static int i = 0;
	int gndIdx=-1, obsIdx=-1;

	/* Get counts in each bin */
	const THDoubleTensor * c_t =
		(THDoubleTensor *) luaT_checkudata(L, 1, "torch.DoubleTensor");
	const long nps  = c_t->size[0];
#ifdef DEBUG
	int is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)c_t);
	if (!is_con)
		return luaL_error(L, "c_t input must be contiguous");
#endif
	double * c_tp = (double *)(c_t->storage->data + c_t->storageOffset);

	/* Get zMaxmin */
	const THDoubleTensor * mm_t =
		(THDoubleTensor *) luaT_checkudata(L, 2, "torch.DoubleTensor");
	const long nps_mm  = mm_t->size[0];
	if (nps_mm!=nps)
		luaL_error(L,"Wrong Number of elements in zMaxMin!");
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)mm_t);
	if (!is_con)
		return luaL_error(L, "mm_t input must be contiguous");
#endif
	double * mm_tp = (double *)(mm_t->storage->data + mm_t->storageOffset);

	/* Get zMean */
	const THDoubleTensor * mean_t =
		(THDoubleTensor *) luaT_checkudata(L, 3, "torch.DoubleTensor");
	const long nps_mean  = mean_t->size[0];
	if (nps_mean!=nps)
		luaL_error(L,"Wrong Number of elements in zMean!");
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)mean_t);
	if (!is_con)
		return luaL_error(L, "mean_t input must be contiguous");
#endif
	double * mean_tp = (double *)(mean_t->storage->data + mean_t->storageOffset);

	/* Get xBin */
	const THDoubleTensor * xbin_t =
		(THDoubleTensor *) luaT_checkudata(L, 4, "torch.DoubleTensor");
	const long nps_xbin  = xbin_t->size[0];
	if (nps_xbin!=nps)
		luaL_error(L,"Wrong Number of elements in xBin!");
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)xbin_t);
	if (!is_con)
		return luaL_error(L, "xbin_t input must be contiguous");
#endif
	double * xbin_tp = (double *)(xbin_t->storage->data + xbin_t->storageOffset);

	/* Get container for iGnd */
	const THDoubleTensor * gnd_t =
		(THDoubleTensor *) luaT_checkudata(L, 5, "torch.DoubleTensor");
	const long nps_gnd  = gnd_t->size[0];
	if (nps_gnd!=nps)
		luaL_error(L,"Wrong Number of elements in iGnd!");
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)gnd_t);
	if (!is_con)
		return luaL_error(L, "gnd_t input must be contiguous");
#endif
	double * gnd_tp = (double *)(gnd_t->storage->data + gnd_t->storageOffset);

	/* Get container for iObs */
	const THDoubleTensor * obs_t =
		(THDoubleTensor *) luaT_checkudata(L, 6, "torch.DoubleTensor");
	const long nps_obs  = obs_t->size[0];
	if (nps_obs!=nps)
		luaL_error(L,"Wrong Number of elements in iObs!");
#ifdef DEBUG
	is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)obs_t);
	if (!is_con)
		return luaL_error(L, "obs_t input must be contiguous");
#endif
	double * obs_tp = (double *)(obs_t->storage->data + obs_t->storageOffset);

	//#pragma omp parallel for
	for( i=0; i<nps; i++ ) {

		double counts = *(c_tp + i);
		double zMaxmin = *(mm_tp + i);
		double zMean = *(mean_tp + i);
		double xBin = *(xbin_tp + i);

		if( counts >= 1 && zMaxmin <=0.3 && zMean<(0.3*xBin+0.05-0.93)
				&& zMean>(-0.3*xBin-0.05-0.93) ){
			gndIdx += 1;
			gnd_tp[gndIdx] = i + 1;
		}

		if( counts >= 1 && zMaxmin >0.3 && zMean>(0.4*xBin+0.10-0.93) ){
			obsIdx += 1;
			obs_tp[obsIdx] = i + 1;
		}
	}
	lua_pushinteger(L, gndIdx+1);
	lua_pushinteger(L, obsIdx+1);

	return 2;
}


/* Get the chest lidar points 
	 and find the last free ground point before the first obstacle  */
int lua_find_last_free_point(lua_State *L) {
	static int i = 0;
	static double x=0;

	/* Get the torch that contains in which bin each point belongs to */
	const THDoubleTensor * ps_x =
		(THDoubleTensor *) luaT_checkudata(L, 1, "torch.DoubleTensor");
	const long nps  = ps_x->size[0];
#ifdef DEBUG
	int is_con = isContiguous<THDoubleTensor>((THDoubleTensor *)ps_x);
	if (!is_con)
		return luaL_error(L, "ps_x input must be contiguous");
#endif
	double * ps_xp = (double *)(ps_x->storage->data + ps_x->storageOffset);

	/* Grab the lidar points 
		 const THDoubleTensor * ps_t =
		 (THDoubleTensor *) luaT_checkudata(L, 2, "torch.DoubleTensor");
		 const long nps_t  = ps_t->size[0];*/

	/* Grab the total number of bins */
	const double nbins = luaL_checknumber(L, 2);

	/* Grab the bin that contains the 1st obstacle */
	const double iFirstObs = luaL_checknumber(L, 3);

	/* Grab the safe distance that we want to keep from the obstacle */
	const double safe_distance = luaL_checknumber(L, 4);


	bool not_found_last_point = true;
	int last_point = 1;
	//#pragma omp parallel for
	for( i=0; i<nps; i++ ) {
		/* x = THTensor_fastGet2d( ps_x, nps-i-1, 0 ); */
		x = ps_xp[(nps-i-1) * ps_x->stride[0] + 0];

		if (x<1) {
			/* THTensor_fastSet2d( ps_x, nps-i-1, 0, nbins ); */
			ps_xp[(nps-i-1) * ps_x->stride[0] + 0] = nbins;
		}

		if ((x>iFirstObs-safe_distance)&&(not_found_last_point)){
			last_point = i + 1;
			not_found_last_point = false;
		}
	}
	lua_pushinteger(L,last_point);
	return 1;
}


/************************************************************************/



static const struct luaL_Reg slam_lib [] = {
	{"connected_components", lua_connected_components},
	{"grow_map", lua_grow_map},
	//
	{"match", lua_match},
	{"update_map", lua_update_map},
	{"decay_map", lua_decay_map},
	//
	{"set_resolution", lua_set_resolution},
	{"set_boundaries", lua_set_boundaries},
	//
	{"binStats", lua_binStats},
	{"update_hmap", lua_update_hmap},
	{"get_ground_points", lua_get_ground_points},
	{"get_height_points", lua_get_height_points},
	{"get_last_free_point",lua_find_last_free_point},
	{"mask_points", lua_mask_points},

	{NULL, NULL}
};


#ifdef __cplusplus
extern "C"
#endif

int luaopen_slam (lua_State *L) {

	// Make the metatable for the connected lidar
  luaL_newmetatable(L, MT_NAME);

#if LUA_VERSION_NUM == 502
	luaL_newlib(L, slam_lib);
#else
	luaL_register(L, "slam", slam_lib);
#endif
	return 1;
}
