local state = {}
state._NAME = ...
local Body   = require'Body'
local util   = require'util'
local vector = require'vector'
local libStep = require'libStep'
-- FSM coordination
local simple_ipc = require'simple_ipc'
local motion_ch = simple_ipc.new_publisher('MotionFSM!')


-- Get the human guided approach
require'hcm'
-- Get the robot guided approach
require'wcm'

require'mcm'







local function calculate_footsteps()
  local step_queue

  if mcm.get_walk_kickfoot()==0 then --left foot kick

    if IS_WEBOTS then
--Take two
      step_queue={
        {{0,0,0},   2,  0.1, 0.5, 0.1,   {0,0.0,0},  {0, 0, 0}},
        {{-0.10,0,0},1,  0.1,   0.4, 0, {0,0.02,0},  {0,0.05,0.05}},    --RS
        {{0.35,0,0},1,  0,   0.7, 0, {0,0.02,0},  {0.05,0.05,0.05}},    --RS
        {{-0.20,0,0},1,  0,   0.7, 0.25, {0,0.02,0},  {0.05,0.05,0.0}},    --RS
        {{0,0,0,},  2,   0.1, 1, 1,     {0,0.0,0},  {0, 0, 0}},                  --DS
      }

    else  
--take two
      step_queue={
        {{0,0,0},   2,  0.1, 1, 0.1,   {0,0.0,0},  {0, 0, 0}},
        {{-0.10,0,0},1,  0.5,   1, 0, {0,0.02,0},  {0,0.05,0.05}},    --RS
        {{0.20,0,0},1,  0,   2, 0, {0,0.02,0},  {0.05,0.05,0.05}},    --RS
        {{-0.10,0,0},1,  0,   1, 0.5, {0,0.02,0},  {0.05,0.05,0.0}},    --RS
        {{0,0,0,},  2,   0.1, 2, 1,     {0,0.0,0},  {0, 0, 0}},                  --DS
      }
    end
  else

    if IS_WEBOTS then
--Take two
      step_queue={
        {{0,0,0},   2,  0.1, 0.5, 0.1,   {0,0.0,0},  {0, 0, 0}},
        {{-0.10,0,0},0,  0.1,   0.4, 0, {0,-0.02,0},  {0,0.05,0.05}},    --LS
        {{0.35,0,0},0,  0,   0.7, 0, {0,-0.02,0},  {0.05,0.05,0.05}},    --LS
        {{-0.20,0,0},0,  0,   0.7, 0.25, {0,-0.02,0},  {0.05,0.05,0.0}},    --LS
        {{0,0,0,},  2,   0.1, 1, 1,     {0,0.0,0},  {0, 0, 0}},                  --DS
      }

    else  

      --take two
      step_queue={
        {{0,0,0},   2,  0.1, 1, 0.1,   {0,0.0,0},  {0, 0, 0}},
        {{-0.10,0,0},0,  0.5,   1, 0, {0,-0.02,0},  {0,0.05,0.05}},    --LS
        {{0.20,0,0},0,  0,   2, 0, {0,-0.02,0},  {0.05,0.05,0.05}},    --LS
        {{-0.10,0,0},0,  0,   1, 0.5, {0,-0.02,0},  {0.05,0.05,0.0}},    --LS
        {{0,0,0,},  2,   0.1, 2, 1,     {0,0.0,0},  {0, 0, 0}},                  --DS
      }

    end

  end  

--Write to SHM
  local maxSteps = 40
  step_queue_vector = vector.zeros(12*maxSteps)
  for i=1,#step_queue do    
    local offset = (i-1)*13;
    step_queue_vector[offset+1] = step_queue[i][1][1]
    step_queue_vector[offset+2] = step_queue[i][1][2]
    step_queue_vector[offset+3] = step_queue[i][1][3]

    step_queue_vector[offset+4] = step_queue[i][2]

    step_queue_vector[offset+5] = step_queue[i][3]
    step_queue_vector[offset+6] = step_queue[i][4]    
    step_queue_vector[offset+7] = step_queue[i][5]    

    step_queue_vector[offset+8] = step_queue[i][6][1]
    step_queue_vector[offset+9] = step_queue[i][6][2]
    step_queue_vector[offset+10] = step_queue[i][6][3]

    step_queue_vector[offset+11] = step_queue[i][7][1]
    step_queue_vector[offset+12] = step_queue[i][7][2]
    step_queue_vector[offset+13] = step_queue[i][7][3]
  end
  mcm.set_step_footholds(step_queue_vector)
  mcm.set_step_nfootholds(#step_queue)
end



local kick_started

function state.entry()
  print(state._NAME..' Entry' )
  -- Update the time of entry
  local t_entry_prev = t_entry -- When entry was previously called
  t_entry = Body.get_time()
  t_update = t_entry
  mcm.set_walk_stoprequest(1)
  kick_started = false
end

function state.update()
  if Config.disable_kick then

    local ballx = wcm.get_ball_x() - Config.fsm.bodyRobocupApproach.target[1]
    local bally = wcm.get_ball_y()
    local ballr = math.sqrt(ballx*ballx+bally*bally)
    if ballr > 0.6 then
      return 'done'
    end
    return
  end

  --print(state._NAME..' Update' )
  -- Get the time of update
  local t  = Body.get_time()
  local dt = t - t_update
  -- Save this at the last update time
  t_update = t
  if mcm.get_walk_ismoving()==0 then
    if kick_started then 
      if mcm.get_walk_kicktype()==1 then
--        print("testdone")
        return 'testdone' --this means testing mode (don't run body fsm)      
      else
        return 'done'
      end
    else
--      motion_ch:send'kick'

      calculate_footsteps()
      motion_ch:send'preview'
      kick_started = true
    end
  end
end

function state.exit()
  print(state._NAME..' Exit' )
end

return state
