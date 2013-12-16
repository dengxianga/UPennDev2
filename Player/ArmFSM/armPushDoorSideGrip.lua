local state = {}
state._NAME = ...
require'hcm'
local vector = require'vector'
local util   = require'util'
local movearm = require'movearm'
local libArmPlan = require 'libArmPlan'
local arm_planner = libArmPlan.new_planner()
local T      = require'Transform'

--Initial hand angle
local lhand_rpy0 = Config.armfsm.dooropen.lhand_rpy
local rollTarget, yawTarget = 0,0


local trLArm0, trRArm0, trLArm1, trRArm1, qLArm0, qRarm0
local gripL, gripR = 1,1
local stage

local function confirm_override()
  local override = hcm.get_state_override()
  hcm.set_state_override_target(override)
end

function state.entry()
  print(state._NAME..' Entry' )
  -- Update the time of entry
  local t_entry_prev = t_entry
  t_entry = Body.get_time()
  t_update = t_entry

  mcm.set_arm_lhandoffset(Config.arm.handoffset.outerhook)
  mcm.set_arm_rhandoffset(Config.arm.handoffset.outerhook)
  local qLArm = Body.get_larm_command_position()
  local qRArm = Body.get_rarm_command_position()

  qLArm0 = qLArm
  qRArm0 = qRArm
  
  trLArm0 = Body.get_forward_larm(qLArm0)
  trRArm0 = Body.get_forward_rarm(qRArm0)  
  
  qLArm1 = Body.get_inverse_arm_given_wrist( qLArm, Config.armfsm.doorpushside.arminit[1])  
  qRArm1 = qRArm
  
  trLArm1 = Body.get_forward_larm(qLArm1)
  trRArm1 = Body.get_forward_rarm(qRArm1)  

  arm_planner:set_shoulder_yaw_target(nil,qRArm0[3])--unlock left shoulder

  arm_planner:set_shoulder_yaw_target(nil,nil)--unlock left shoulder

  hcm.set_door_model(Config.armfsm.dooropenleft.default_model)
  hcm.set_door_yaw(0)

  hcm.set_state_tstartactual(unix.time()) 
  hcm.set_state_tstartrobot(Body.get_time())
  confirm_override()

  local cur_cond = arm_planner:load_boundary_condition()
  local qLArm = cur_cond[1]
  local qRArm = cur_cond[2]
  trLArm0 = Body.get_forward_larm(cur_cond[1])
  trRArm0 = Body.get_forward_rarm(cur_cond[2])  

  print("trLArm:",arm_planner.print_transform(trLArm))
  print("trRArm:",arm_planner.print_transform(trRArm))

  local wrist_seq = {{'move',trLArm0, trRArm0, 30*Body.DEG_TO_RAD,0},}
  if arm_planner:plan_arm_sequence(wrist_seq) then stage = "bodyturn" end
  hcm.set_state_proceed(0) --stop here and wait
end

function state.update()
--  print(state._NAME..' Update' )
  -- Get the time of update  
  local t  = Body.get_time()
  local dt = t - t_update
  -- Save this at the last update time
  t_update = t
  --if t-t_entry > timeout then return'timeout' end
  local cur_cond = arm_planner:load_boundary_condition()
  local qLArm = cur_cond[1]
  local qRArm = cur_cond[2]
  local trLArm = Body.get_forward_larm(cur_cond[1])
  local trRArm = Body.get_forward_rarm(cur_cond[2])  
  
  door_yaw = 0

  if stage=="bodyturn" then 
    if arm_planner:play_arm_sequence(t) then 
      if hcm.get_state_proceed()==1 then 
        local wrist_seq = {
          {'wrist',Config.armfsm.doorpushside.arminit[1], nil},
          {'wrist',Config.armfsm.doorpushside.arminit[2], nil}
        }
        if arm_planner:plan_arm_sequence2(wrist_seq) then stage="movearm" end
        hcm.set_state_proceed(0) --stop here and wait
      elseif hcm.get_state_proceed()==-1 then 
        local wrist_seq = {{'move',trLArm0, trRArm0, 0*Body.DEG_TO_RAD,0},}
        if arm_planner:plan_arm_sequence(wrist_seq) then stage = "bodyreturn" end
      end
    end
  elseif stage=="movearm" then --Turn yaw angles first
    if arm_planner:play_arm_sequence(t) then 

      if hcm.get_state_proceed()==1 then 
        print("trLArm:",arm_planner.print_transform(trLArm))
        print("trRArm:",arm_planner.print_transform(trRArm))
        hcm.set_state_proceed(0) --stop here and wait
      elseif hcm.get_state_proceed()==-1 then 
        local wrist_seq = {
          {'wrist',Config.armfsm.doorpushside.arminit[3], nil},
          {'move',Config.armfsm.doorpushside.arminit[3], nil},
          {'wrist',Config.armfsm.doorpushside.arminit[1], nil},
        }
        if arm_planner:plan_arm_sequence2(wrist_seq) then 
          stage="bodyturn" 
          print("READY")          
        end 
        

      elseif hcm.get_state_proceed()==3 then --adjust hook position
        local trArmCurrent = hcm.get_hands_left_tr()
        local overrideTarget = hcm.get_state_override_target()
        local override = hcm.get_state_override()
        local trArmTarget = {
          trArmCurrent[1]+(overrideTarget[1]-override[1]),
          trArmCurrent[2]+(overrideTarget[2]-override[2]),
          trArmCurrent[3]+(overrideTarget[3]-override[3]),
          trArmCurrent[4],

          --Pitch control
          trArmCurrent[5]+(overrideTarget[4]-override[4])*Config.armfsm.doorpushside.unit_tilt,

          --Yaw control
          trArmCurrent[6]+(overrideTarget[5]-override[5])*Config.armfsm.doorpushside.unit_yaw,
        }        
        local arm_seq = {{'move',trArmTarget,nil}}
        --Wrist movement?
        if math.abs(overrideTarget[4]-override[4]) + math.abs(overrideTarget[5]-override[5])>0 then
          arm_seq = {{'wrist',trArmTarget,nil}}
        end

        
        if arm_planner:plan_arm_sequence2(arm_seq) then 
          stage = "movearm" 
          hcm.set_state_override(overrideTarget)
        else
          hcm.set_state_override_target(override)
        end
      end
    end
  elseif stage=="bodyreturn" then 
    if arm_planner:play_arm_sequence(t) then 
      arm_planner:set_shoulder_yaw_target(qLArm0[3],qRArm0[3])    
      local wrist_seq = {{'move',trLArm0, trRArm0},}
      if arm_planner:plan_arm_sequence2(wrist_seq) then stage = "armbacktoinitpos" end
    end
  elseif stage=="armbacktoinitpos" then 
    if arm_planner:play_arm_sequence(t) then 
      return "done" 
    end
  end

end

function state.exit()    
  print(state._NAME..' Exit' )
end

return state