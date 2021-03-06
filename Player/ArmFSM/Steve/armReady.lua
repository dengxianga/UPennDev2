--------------------------------
-- Humanoid arm state
-- (c) 2013 Stephen McGill, Seung-Joon Yi
--------------------------------
local state = {}
state._NAME = ...

local Body   = require'Body'
local vector = require'vector'
local movearm = require'movearm'

local t_entry, t_update, t_finish
local timeout = 30.0

local lco, rco, uComp
local okL, qLWaypoint, qLWaist
local okR, qRWaypoint, qRWaist

local sequence, s, stage = Config.arm.ready

function state.entry()
  io.write(state._NAME, ' Entry\n')
  local t_entry_prev = t_entry
  t_entry = Body.get_time()
  t_update = t_entry

	s = 1
	stage = sequence[s]
	lco, rco = movearm.goto(stage.left, stage.right)

	-- Check for no motion
	okL = type(lco)=='thread' or lco==false
	okR = type(rco)=='thread' or rco==false

end

function state.update()
	--  io.write(state._NAME,' Update\n')
  local t  = Body.get_time()
  local dt = t - t_update
  t_update = t
  --if t-t_entry > timeout then return'timeout' end
	if not stage then return'done' end

	local lStatus = type(lco)=='thread' and coroutine.status(lco)
	local rStatus = type(rco)=='thread' and coroutine.status(rco)

	local qLArm = Body.get_larm_position()
	local qRArm = Body.get_rarm_position()
	if lStatus=='suspended' then
		okL, qLWaypoint, qLWaist = coroutine.resume(lco, qLArm, qWaist)
	end
	if rStatus=='suspended' then
		okR, qRWaypoint, qRWaist = coroutine.resume(rco, qRArm, qWaist)
	end

	if not okL or not okR then
		print(state._NAME, 'L', okL, qLWaypoint)
		print(state._NAME, 'R', okR, qRWaypoint)
		-- Safety
		Body.set_larm_command_position(qLArm)
		Body.set_rarm_command_position(qRArm)
		return'teleopraw'
	end

	if type(qLWaypoint)=='table' then
		Body.set_larm_command_position(qLWaypoint)
	end
	if type(qRWaypoint)=='table' then
		Body.set_rarm_command_position(qRWaypoint)
	end
	if qLWaist and qRWaist then
		print('Conflicting Waist')
	elseif qLWaist then
		Body.set_waist_command_position(qLWaist)
	elseif qRWaist then
		Body.set_waist_command_position(qRWaist)
	end

	-- Check if done
	if lStatus=='dead' and rStatus=='dead' then
		-- Goto the nextitem in the sequnce
		s = s + 1
		stage = sequence[s]
		if stage then
			print('Next sequence:', s, stage)
			lco, rco = movearm.goto(stage.left, stage.right)
		end
	end

	-- Set the compensation: Not needed
	--mcm.set_stance_uTorsoComp(uComp)

end

function state.exit()
	io.write(state._NAME, ' Exit\n')

	if not okL or not okR then
		local qLArm = Body.get_larm_position()
		local qRArm = Body.get_rarm_position()
		hcm.set_teleop_larm(qLArm)
		hcm.set_teleop_rarm(qRArm)
	else
		local qcLArm = Body.get_larm_command_position()
		local qcRArm = Body.get_rarm_command_position()
		hcm.set_teleop_larm(qcLArm)
		hcm.set_teleop_rarm(qcRArm)
	end

end

return state
