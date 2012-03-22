/****************************************************************************
 *                                                                           *
 *  OpenNI 1.x Alpha                                                         *
 *  Copyright (C) 2011 PrimeSense Ltd.                                       *
 *                                                                           *
 *  This file is part of OpenNI.                                             *
 *                                                                           *
 *  OpenNI is free software: you can redistribute it and/or modify           *
 *  it under the terms of the GNU Lesser General Public License as published *
 *  by the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  OpenNI is distributed in the hope that it will be useful,                *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
 *  GNU Lesser General Public License for more details.                      *
 *                                                                           *
 *  You should have received a copy of the GNU Lesser General Public License *
 *  along with OpenNI. If not, see <http://www.gnu.org/licenses/>.           *
 *                                                                           *
 ****************************************************************************/
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnCppWrapper.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#ifdef __cplusplus
}
#endif

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define SAMPLE_XML_PATH "./Config/SamplesConfig.xml"
#define SAMPLE_XML_PATH_LOCAL "SamplesConfig.xml"
#define MAX_NUM_USERS 1
#define MAX_NUM_JOINTS 24

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
xn::Context g_Context;
xn::ScriptNode g_scriptNode;
xn::DepthGenerator g_DepthGenerator;
xn::UserGenerator g_UserGenerator;

XnBool g_bNeedPose = FALSE;
XnChar g_strPose[20] = "";

XnUserID aUsers[MAX_NUM_USERS];
XnUInt16 nUsers;
XnSkeletonJointTransformation torsoJoint;
XnUInt32 epochTime = 0;

// New
XnSkeletonJoint aJoints[MAX_NUM_JOINTS];
XnUInt16 nJoints;

// Tables
XnVector3D   posTable[MAX_NUM_JOINTS];
XnConfidence posConfTable[MAX_NUM_JOINTS];
XnMatrix3X3  rotTable[MAX_NUM_JOINTS]; // 9 element float array
XnConfidence rotConfTable[MAX_NUM_JOINTS];

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

XnBool fileExists(const char *fn)
{
  XnBool exists;
  xnOSDoesFileExist(fn, &exists);
  return exists;
}

// Callback: New user was detected
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
  XnUInt32 epochTime = 0;
  xnOSGetEpochTime(&epochTime);
  printf("%d New User %d\n", epochTime, nId);
  // New user found
  if (g_bNeedPose)
  {
    g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
  }
  else
  {
    g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
  }
}
// Callback: An existing user was lost
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
  XnUInt32 epochTime = 0;
  xnOSGetEpochTime(&epochTime);
  printf("%d Lost user %d\n", epochTime, nId);	
}
// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
  XnUInt32 epochTime = 0;
  xnOSGetEpochTime(&epochTime);
  printf("%d Pose %s detected for user %d\n", epochTime, strPose, nId);
  g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
  g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}
// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
  XnUInt32 epochTime = 0;
  xnOSGetEpochTime(&epochTime);
  printf("%d Calibration started for user %d\n", epochTime, nId);
}

void XN_CALLBACK_TYPE UserCalibration_CalibrationComplete(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie)
{
  XnUInt32 epochTime = 0;
  xnOSGetEpochTime(&epochTime);
  if (eStatus == XN_CALIBRATION_STATUS_OK)
  {
    // Calibration succeeded
    printf("%d Calibration complete, start tracking user %d\n", epochTime, nId);		
    g_UserGenerator.GetSkeletonCap().StartTracking(nId);
  }
  else
  {
    // Calibration failed
    printf("%d Calibration failed for user %d\n", epochTime, nId);
    if(eStatus==XN_CALIBRATION_STATUS_MANUAL_ABORT)
    {
      printf("Manual abort occured, stop attempting to calibrate!");
      return;
    }
    if (g_bNeedPose)
    {
      g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
    }
    else
    {
      g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
    }
  }
}


#define CHECK_RC(nRetVal, what)					    \
  if (nRetVal != XN_STATUS_OK)				    \
{								    \
  printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));    \
  return nRetVal;						    \
}

static int init() {
  XnStatus nRetVal = XN_STATUS_OK;
  xn::EnumerationErrors errors;

  const char *fn = NULL;
  if    (fileExists(SAMPLE_XML_PATH)) fn = SAMPLE_XML_PATH;
  else if (fileExists(SAMPLE_XML_PATH_LOCAL)) fn = SAMPLE_XML_PATH_LOCAL;
  else {
    printf("Could not find '%s' nor '%s'. Aborting.\n" , SAMPLE_XML_PATH, SAMPLE_XML_PATH_LOCAL);
    return XN_STATUS_ERROR;
  }
  printf("Reading config from: '%s'\n", fn);

  nRetVal = g_Context.InitFromXmlFile(fn, g_scriptNode, &errors);
  if (nRetVal == XN_STATUS_NO_NODE_PRESENT)
  {
    XnChar strError[1024];
    errors.ToString(strError, 1024);
    printf("%s\n", strError);
    return (nRetVal);
  }
  else if (nRetVal != XN_STATUS_OK)
  {
    printf("Open failed: %s\n", xnGetStatusString(nRetVal));
    return (nRetVal);
  }

  nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
  CHECK_RC(nRetVal,"No depth");

  nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
  if (nRetVal != XN_STATUS_OK)
  {
    nRetVal = g_UserGenerator.Create(g_Context);
    CHECK_RC(nRetVal, "Find user generator");
  }

  XnCallbackHandle hUserCallbacks, hCalibrationStart, hCalibrationComplete, hPoseDetected;
  if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
  {
    printf("Supplied user generator doesn't support skeleton\n");
    return 1;
  }
  nRetVal = g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
  CHECK_RC(nRetVal, "Register to user callbacks");
  printf("User callbacks\n");  

  nRetVal = g_UserGenerator.GetSkeletonCap().RegisterToCalibrationStart(UserCalibration_CalibrationStart, NULL, hCalibrationStart);
  CHECK_RC(nRetVal, "Register to calibration start");
  printf("Starting calibration\n");  
  nRetVal = g_UserGenerator.GetSkeletonCap().RegisterToCalibrationComplete(UserCalibration_CalibrationComplete, NULL, hCalibrationComplete);
  CHECK_RC(nRetVal, "Register to calibration complete");
  printf("Completing calibration\n");

  if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration())
  {
    g_bNeedPose = TRUE;
    if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
    {
      printf("Pose required, but not supported\n");
      return 1;
    }
    nRetVal = g_UserGenerator.GetPoseDetectionCap().RegisterToPoseDetected(UserPose_PoseDetected, NULL, hPoseDetected);
    CHECK_RC(nRetVal, "Register to Pose Detected");
    g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
  }

  g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

  nRetVal = g_Context.StartGeneratingAll();
  CHECK_RC(nRetVal, "StartGenerating");

  printf("Starting to run\n");
  if(g_bNeedPose)
  {
    printf("Assume calibration pose\n");
  }
  /*
     while (!xnOSWasKeyboardHit()){}
     */
}
void close(){
  g_scriptNode.Release();
  g_DepthGenerator.Release();
  g_UserGenerator.Release();
  g_Context.Release();
}


static int lua_get_jointtables(lua_State *L) {
  int joint = luaL_checkint(L, 1);
  if( joint>MAX_NUM_JOINTS || joint<=0 ) {
    lua_pushnil(L);
    return 1;
  }
  joint--; // Lua Input is [1,24], C/C++ is [0,23]

  // Push the position  
  lua_createtable(L, 3, 0);
  lua_pushnumber(L, posTable[joint].X);
  lua_rawseti(L, -2, 1);
  lua_pushnumber(L, posTable[joint].Y);
  lua_rawseti(L, -2, 2);
  lua_pushnumber(L, posTable[joint].Z);
  lua_rawseti(L, -2, 3);
  //printf("Joint %u\t(%lf,%lf,%lf)\n",joint+1,posTable[joint].X,posTable[joint].Y,posTable[joint].Z);


  // Push the orientation
  lua_createtable(L, 9, 0);
  //printf("Joint %u\t( ",joint+1);  
  for (int i = 0; i < 9; i++) {
    XnFloat* tmp = rotTable[joint].elements;
    lua_pushnumber(L, tmp[i]);   /* Push the table index */
    lua_rawseti(L, -2, i+1);
    //printf( "%lf ", tmp[i] );
  }
  //printf(")\n");  

  // Push the confidences
  lua_createtable(L, 2, 0);  
  lua_pushnumber(L, posConfTable[joint] );
  lua_rawseti(L, -2, 1);
  lua_pushnumber(L, rotConfTable[joint] );
  lua_rawseti(L, -2, 2);
  //printf("Joint %u\t(%lf,%lf)\n",joint+1,posConfTable[joint],rotConfTable[joint]);

  return 3;
}

static int lua_update_joints(lua_State *L) {

  g_Context.WaitOneUpdateAll(g_UserGenerator);
  //  printf("Waited one update...\n");
  // print the torso information for the first user already tracking
  nUsers=MAX_NUM_USERS;
  g_UserGenerator.GetUsers(aUsers, nUsers);
  //  printf("Got all users...\n");

  int ret = -1;

  for(XnUInt16 i=0; i<nUsers; i++)
  { 
    if( g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])==FALSE){
      continue;
    } else {
      ret = i;
    }

    // TODO: Error check
    //g_UserGenerator.GetSkeletonCap().EnumerateActiveJoints( aJoints, nJoints );
    //printf("Entering loop with %d joints...\n", nJoints);    
    for( XnUInt16 jj=0; jj<MAX_NUM_JOINTS; jj++ ){
      XnSkeletonJoint j = XnSkeletonJoint(jj);
      // Use torsoJoint as a temporary joint
      g_UserGenerator.GetSkeletonCap().GetSkeletonJoint(aUsers[i], j, torsoJoint);
      XnVector3D   tmpPos  = torsoJoint.position.position;
      XnConfidence posConf = torsoJoint.position.fConfidence;
      XnMatrix3X3  tmpRot  = torsoJoint.orientation.orientation; // 9 element float array
      XnConfidence rotConf = torsoJoint.orientation.fConfidence;
      //printf("Joint %u = (%lf,%lf,%lf)\n",j,tmpPos.X,tmpPos.Y,tmpPos.Z);
      // Set the right table value posTable
      posTable[jj] = tmpPos;
      rotTable[jj] = tmpRot;
      posConfTable[jj] = posConf;
      rotConfTable[jj] = rotConf;
    }
  }
  if(ret==-1){
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, ret);
  }
  return 1;
}

static const struct luaL_reg primesense_lib [] = {
  {"update_joints", lua_update_joints},
  {"get_jointtables", lua_get_jointtables},
  {NULL, NULL}
};

extern "C"
int luaopen_primesense (lua_State *L) {
  luaL_register(L, "primesense", primesense_lib);
  init();

  return 1;
}
