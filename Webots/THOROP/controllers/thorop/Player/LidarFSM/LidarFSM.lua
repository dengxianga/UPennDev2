-- Config guides special situations
local Config = require'Config'
-- Use the fsm module
local fsm = require'fsm'

-- Require the needed states
local lidarIdle = require'lidarIdle'

-- Instantiate a new state machine with an initial state
-- This will be returned to the user
local sm = fsm.new( lidarIdle )

local lidarPan  = require'lidarPan'
sm:add_state(lidarPan)

-- Setup the transistions for this FSM
sm:set_transition( lidarIdle, 'pan', lidarPan  )
sm:set_transition( lidarPan,  'stop',  lidarIdle )

--------------------------
-- Setup the FSM object --
--------------------------
local obj = {}
obj._NAME = ...
local util = require'util'
-- Simple IPC for remote state triggers
local simple_ipc = require'simple_ipc'
local evts = simple_ipc.new_subscriber(...,true)
local debug_str = util.color(obj._NAME..' Event:','green')
local special_evts = special_evts or {}
obj.entry = function()
  sm:entry()
end
obj.update = function()
  -- Check for out of process events in non-blocking fashion
  local event, has_more = evts:receive(true)
  if event then
    local debug_tbl = {debug_str,event}
    if has_more then
      extra = evts:receive(true)
      table.insert(debug_tbl,'Extra')
    end
    local special = special_evts[event]
    if special then
      special(extra)
      table.insert(debug_tbl,'Special!')
    end
    print(table.concat(debug_tbl,' '))
    sm:add_event(event)
  end
  sm:update()
end
obj.exit = function()
  sm:exit()
end

obj.sm = sm

return obj