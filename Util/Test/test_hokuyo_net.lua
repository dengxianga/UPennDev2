dofile'../../include.lua'
local Body       = require'Body'
local signal     = require'signal'
local carray     = require'carray'
local mp         = require'msgpack'
local util       = require'util'
local simple_ipc = require'simple_ipc'
local libHokuyo  = require'libHokuyo'
local tcp        = require'tcp'

-- CHEST IS 192.168.0.10!
-- Make the head .11
-- TODO: Broadcast to find hokuyos?
local chest_hokuyo = libHokuyo.new_hokuyo(10)
--local head_hokuyo = libHokuyo.new_hokuyo("/dev/cu.usbmodemfd121")

-- Setup the Hokuyos array
local hokuyos = {}

-- Chest Hokuyo
if chest_hokuyo then
  chest_hokuyo.name = 'Chest'
  chest_hokuyo.count = 0
  table.insert(hokuyos,chest_hokuyo)
  local chest_lidar_ch = simple_ipc.new_publisher'chest_lidar'
  chest_hokuyo.callback = function(data)
    Body.set_chest_lidar( data )
    chest_hokuyo.count = chest_hokuyo.count + 1
    
    -- Midpoint printing
    local array = carray.float(data)
    chest_hokuyo.mid = array[540]

    local meta = {}
    meta.count  = chest_hokuyo.count
    meta.pangle = Body.get_lidar_command_position(1)
    local ret = chest_lidar_ch:send( mp.pack(meta) )
  end
end

if head_hokuyo then
  head_hokuyo.name = 'Head'
  head_hokuyo.count = 0
  table.insert(hokuyos,head_hokuyo)
  local chest_lidar_ch = simple_ipc.new_publisher'head_lidar'
  head_hokuyo.callback = function(data)
    Body.set_chest_lidar( data )
    head_hokuyo.count = head_hokuyo.count + 1

    -- Midpoint printing
    local array = carray.float(data)
    head_hokuyo.mid = array[540]

    local meta = {}
    meta.count  = head_hokuyo.count
    meta.hangle = Body.get_head_command_position()
    local ret = chest_lidar_ch:send( mp.pack(meta) )
  end
end

-- Ensure that we shutdown the devices properly
function shutdown()
  print'Shutting down the Hokuyos...'
  for i,h in ipairs(hokuyos) do
    h:stream_off()
    h:close()
    print('Closed Hokuyo',i)
  end
  os.exit()
end
signal.signal("SIGINT", shutdown)
signal.signal("SIGTERM", shutdown)

-- Begin to service
os.execute('clear')
assert(#hokuyos>0,"No hokuyos detected!")
print( util.color('Servicing '..#hokuyos..' Hokuyos','green') )

local main = function()
  local main_cnt = 0
  local t0 = Body.get_time()
  while true do
    main_cnt = main_cnt + 1
    local t_now = Body.get_time()
    local t_diff = t_now - t0
    if t_diff>1 then
      local debug_str = string.format('\nMain loop: %7.2f Hz',main_cnt/t_diff)
      debug_str = util.color(debug_str,'yellow')
      for i,h in ipairs(hokuyos) do
        debug_str = debug_str..string.format(
        '\n\t%s Hokuyo was seen %5.3f seconds ago: %g m',
        h.name,t_now - h.t_last,h.mid)
      end
      os.execute('clear')
      print(debug_str)
      t0 = t_now
      main_cnt = 0
    end
    coroutine.yield()
  end
end
libHokuyo.service( hokuyos, main )

