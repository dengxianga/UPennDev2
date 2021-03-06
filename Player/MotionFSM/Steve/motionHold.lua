--Stance state is basically a Walk controller
--Without any torso or feet update
--We share the leg joint generation / balancing code 
--with walk controllers

local state = {}
state._NAME = ...

local Body   = require'Body'
local vector = require'vector'
local moveleg = require'moveleg'
require'mcm'

-- Keep track of important times
local t_entry, t_update, t_last_step
local timeout = 30
-- Track the torso
local uTorso, uLeft, uRight
local zLeft, zRight
local side

-- Lift properties
local xTarget
local dxTarget = 0.26
local dpose = vector.pose{0.002, 0, 0}

function state.entry()
  print(state._NAME..' Entry' )
  -- Update the time of entry
  local t_entry_prev = t_entry -- When entry was previously called
  t_entry = Body.get_time()
  t_update = t_entry
  uTorso = mcm.get_status_uTorso()  
  uLeft = mcm.get_status_uLeft()
  uRight = mcm.get_status_uRight()
  side = mcm.get_teach_sway()
  side = side=='none' and 'left' or side
  print('Support on the', side)
  local l_ft, r_ft = Body.get_lfoot(), Body.get_rfoot()
  local side_check = l_ft[3]>r_ft[3] and 'left' or 'right'
  if side_check~=side then print('BAD SUPPORT FOR HOLD!') end
  zLeft, zRight = unpack(mcm.get_status_zLeg())
  xTarget = (side=='left' and uRight[1] or uLeft[1]) + dxTarget
end

function state.update()
  -- Get the time of update
  local t = Body.get_time()
  local dt = t - t_update
  -- Save this at the last update time
  t_update = t
  if t - t_entry > timeout then return'timeout' end
  
  -- TODO: If the foot hits something, then must retract and lower the foot!
  local l_ft, r_ft = Body.get_lfoot(), Body.get_rfoot()
  
  if side=='left' then
    uRight = uRight + dpose
    if uRight[1] > xTarget then return'done' end
    mcm.set_status_uRight(uRight)
  else
    uLeft = uLeft + dpose
    if uLeft[1] > xTarget then return'done' end
    mcm.set_status_uLeft(uLeft)
  end
  moveleg.set_leg_positions_slowly(uTorso, uLeft, uRight, zLeft, zRight, dt)
end

function state.exit()
  print(state._NAME..' Exit')
end

return state
