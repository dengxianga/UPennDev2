local libWorld = {}

--SJ: a dummy world file
--we can add slam, odometry, and so on later

-- TODO: Add Attack bearing
-- TODO: Add Webots ground truth knowledge
local Body   = require'Body'
local vector = require'vector'
local util = require'util'
local odomScale = Config.world.odomScale 
local odomDrift = Config.world.odomDrift or -0.0001

require'wcm'
require'gcm'
require'mcm'
wcm.set_robot_use_imu_yaw(Config.world.use_imu_yaw and 1 or 0)

-- Timestamps
local t_entry
-- Cycle count
local count
-- Objects
local ball, goal, obstacle, line
-- Obstacle filters
local OF = {}

-- Initial odometry
local uOdometry0 = vector.zeros(3)
-- Save the resampling times
local t_resample = 0


local yaw0 = 0
local function update_odometry(uOdometry)  
  -- Scale the odometry

  --odometry update is called every frame (~120hz)
  --we drift away 1cm back every step (~0.8sec)
  --then we have 0.01m / 0.8sec / 120 hz drift per step

  uOdometry[1] = odomScale[1] * uOdometry[1] + odomDrift
  uOdometry[2] = odomScale[2] * uOdometry[2] 
  uOdometry[3] = odomScale[3] * uOdometry[3]
  -- Next, grab the gyro yaw

--  if Config.use_imu_yaw and mcm.get_walk_ismoving()>0 then  

  if Config.use_imu_yaw then
    if IS_WEBOTS then
      gps_pose = wcm.get_robot_pose_gps()
      uOdometry[3] = gps_pose[3] - yaw0
      yaw0 = gps_pose[3]
    else
      local yaw = Body.get_rpy()[3]
      uOdometry[3] = yaw - yaw0
      yaw0 = yaw
    end    
  end
  yaw0 = Body.get_rpy()[3] --We need to keep update this (to use the increment only while walking)

  --Update pose using odometry info for now
  local pose = wcm.get_robot_pose()
  wcm.set_robot_pose(util.pose_global(uOdometry,pose))

  --TODO: slam or wall detection-based pose


end

local function update_vision(detected)  
end

function libWorld.pose_reset()
  wcm.set_robot_reset_pose(0)
  wcm.set_robot_pose({0,0,0})
  wcm.set_robot_odometry({0,0,0})
  yaw0 = Body.get_rpy()[3]
  if IS_WEBOTS then
    gps_pose = wcm.get_robot_pose_gps()
    yaw0 = gps_pose[3]
    wcm.set_robot_pose_gps0(wcm.get_robot_pose_gps())
  end  
end

function libWorld.entry()
	wcm.set_robot_use_imu_yaw(Config.world.use_imu_yaw and 1 or 0)
	t_entry = unix.time()
  -- Save this resampling time
  t_resample = t_entry
  -- Set the initial odometry
  libWorld.pose_reset()
  -- Processing count
  count = 0
end

local function print_pose()
  if not Config.debug.world then return end
  local pose = wcm.get_robot_pose()
  local gpspose1 = wcm.get_robot_pose_gps()
  local gpspose0 = wcm.get_robot_pose_gps0()
  local gpspose = util.pose_relative(gpspose1,gpspose0)
  print(string.format(
    "pose: %.3f %.3f %d gps: %.3f %.3f %d",
    pose[1],pose[2],pose[3]*180/math.pi,
    gpspose[1],gpspose[2],gpspose[3]*180/math.pi))
  local uTorso = mcm.get_status_uTorso()
  print("uTOrso:",unpack(uTorso))  
end

function libWorld.update(uOdom, detection)
  local t = unix.time()
  -- Run the updates
  if wcm.get_robot_reset_pose()==1 then
    print("libWorld | POSE RESET!")
    libWorld.pose_reset()    
  end
  if IS_WEBOTS and Config.use_gps_pose then
    local gpspose1 = wcm.get_robot_pose_gps()
    local gpspose0 = wcm.get_robot_pose_gps0()
    local gpspose = util.pose_relative(gpspose1,gpspose0)

    --subtract automatic compensation
    comoffset = mcm.get_stance_COMoffset()
    comoffset[3]=0 
    gpspose = util.pose_global(comoffset,gpspose)
    wcm.set_robot_pose(gpspose)
    print_pose()

  else
    update_odometry(uOdom)
    print_pose()   
  end
  -- Increment the process count
  count = count + 1
end


function libWorld.send()
  local to_send = {}
  to_send.info = ''
  -- Robot info
  to_send.pose = vector.new(wcm.get_robot_pose())
  to_send.time = Body.get_time()
  to_send.info = to_send.info..string.format(
    'Pose: %.2f %.2f (%.1f)\n', to_send.pose[1], to_send.pose[2], to_send.pose[3]*RAD_TO_DEG)
  to_send.time = Body.get_time()
  return to_send
end

function libWorld.exit()
end

function libWorld.get_pose()
--TODO
--  return wcm.get_robot_pose(wcm.get_robot_pose_gps())
  --return vector.pose({0,0,0})
  --return vector.pose{poseFilter.get_pose()}
  return wcm.get_robot_pose()
end

libWorld.update_odometry = update_odometry
libWorld.update_vision = update_vision

return libWorld
