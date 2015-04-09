#!/usr/bin/env luajit
local ENABLE_NET = true
local ENABLE_LOG = false
-----------------------------------
-- Camera manager
-- (c) Stephen McGill, 2014
-----------------------------------
dofile'../include.lua'
if type(arg)~='table' then IS_WEBOTS=true end
local si = require'simple_ipc'
local mp = require'msgpack.MessagePack'
local jpeg = require'jpeg'
local Body = require'Body'
require'hcm'
local get_time = Body.get_time
-- JPEG Compressor
local c_grey = jpeg.compressor('gray')
local c_yuyv = jpeg.compressor('yuyv')
-- TODO: Control the downsampling mode
--c_yuyv:downsampling(2)
--c_yuyv:downsampling(1)

-- Grab the metadata for this camera
local metadata, camera_id
if type(arg)~='table' or not arg[1] then
	-- TODO: Find the next available camera
	camera_id = 1
	metadata = Config.camera[1]
else
	camera_id = tonumber(arg[1])
	if camera_id then
		metadata = assert(Config.camera[camera_id], 'Bad Camera ID')
	else
		for id, c in ipairs(Config.camera) do
			if arg[1] == c.name then
				camera_id = id
				metadata = c
				break
			end
		end
		assert(metadata, 'Bad camera name')
	end
end

local t_send, t_log = 0, 0
local LOG_INTERVAL = 1/5
local SEND_INTERVAL = .5


local libLog, logger

-- Extract metadata information
local w = metadata.w
local h = metadata.h
local name = metadata.name
-- Who to send to
local operator
operator = Config.net.operator.wireless
--operator = Config.net.operator.wired
-- Network Channels/Streams
local camera_identifier = 'camera'..(camera_id-1)
local stream = Config.net.streams[camera_identifier]
local udp_ch = stream and stream.udp and si.new_sender(operator, stream.udp)
local camera_ch = stream and stream.sub and si.new_publisher(stream.sub)
print('Camera | ', operator, camera_identifier, stream.udp, udp_ch)

-- Metadata for the operator for compressed image data
local c_meta = {
	-- Required for rendering
	sz = 0,
	c = 'jpeg',
	-- Extra information
	t = 0,
	id = name..'_camera',
	n = 0,
}

local has_detection, detection = pcall(require, metadata.detection)
-- Send which camera we are using
if has_detection then detection.entry(metadata) end

-- LOGGING
if ENABLE_LOG then
	libLog = require'libLog'
	logger = libLog.new('yuyv', true)
end

local nlog = 0
local udp_ret, udp_err, udp_data
local t0 = get_time()
local t_debug = 0

local function update(img, sz, cnt, t)
	-- Update metadata
	c_meta.t = t
	c_meta.n = cnt

	local do_send = t-t_send > (1 / hcm.get_monitor_fps())
	t_send = do_send and t or t_send

	-- Check if we are sending to the operator
	if ENABLE_NET and do_send then		
		local c_img = c_yuyv:compress(img, w, h)
		c_meta.sz = #c_img
		if camera_ch then
			camera_ch:send({mp.pack(c_meta), c_img})
		end
		if udp_ch then
			udp_data = mp.pack(c_meta)..c_img
			udp_ret, udp_err = udp_ch:send(udp_data)
		end
	end

	-- Do the logging if we wish
	if ENABLE_LOG and t - t_log > LOG_INTERVAL then
		t_log = t
		nlog = nlog + 1
		metadata.rsz = sz
		metadata.head = Body.get_head_position()
		metadata.rpy = Body.get_rpy() 
		for pname, p in pairs(pipeline) do metadata[pname] = p.get_metadata() end
		logger:record(metadata, img, sz)
		if nlog % 10 == 0 then
			print("# camera logs: "..nlog)
			if nlog % 100 == 0 then
				logger:stop()
				logger = libLog.new('yuyv', true)
				print('Open new log!')
			end
		end
	end

	-- Update the vision routines
	if has_detection then
		detection.update(img)
		if ENABLE_NET and detection.send and do_send then
			if camera_ch then
				for _,v in ipairs(detection.send()) do camera_ch:send({mp.pack(v[1]), v[2]}) end
			end
			if udp_ch then
				for _,v in ipairs(detection.send()) do
					if v[2] then
						udp_data = mp.pack(v[1])..v[2]
					else
						udp_data = mp.pack(v[1])
					end
					udp_ret, udp_err = udp_ch:send(udp_data)
				end
			end
		end
	end
end

-- If required from Webots, return the table
if ... and type(...)=='string' and not tonumber(...) then
	return {entry=nil, update=update, exit=nil}
end

-- Open the camera
local camera = require'uvc'.init(metadata.dev, w, h, metadata.format, 1, metadata.fps)
os.execute('uvcdynctrl -d'..metadata.dev..' -s "Exposure, Auto" 1')
-- Set the params
for i, param in ipairs(metadata.auto_param) do
	local name, value = unpack(param)
	local before = camera:get_param(name)
	local ok = camera:set_param(name, value)
--	unix.usleep(1e5)
--	local now = camera:get_param(name)
	assert(ok, string.format('Failed to set %s: from %d to %d', name, before, value))
end
-- Set the params
for i, param in ipairs(metadata.param) do
	local name, value = unpack(param)
	camera:set_param(name, value)
	unix.usleep(1e5)
	local now = camera:get_param(name)
	-- TODO: exposure
	local count = 0
	--while count<5 and now~=value do
	local ok
	while count<5 and not ok do
		ok = camera:set_param(name, value)
		unix.usleep(1e5)
		count = count + 1
		--now = camera:get_param(name)
	end
	assert(now==value, string.format('Failed to set %s: %d -> %d',name, value, now))
end

-- Cleanly exit on Ctrl-C
local running = true
local function shutdown()
  running = false
end

local signal = require'signal'.signal
signal("SIGINT", shutdown)
signal("SIGTERM", shutdown)

while running do
	local img, sz, cnt, t = camera:get_image()
	update(img, sz, cnt, t)
	if t-t_debug>1 then
		t_debug = t
		local kb = collectgarbage('count')
		local debug_str = {
			string.format("Camera | %s Uptime: %.2f Mem: %d kB", name, t-t0, kb),
			"# logs: "..nlog
		}
		print(table.concat(debug_str,'\n'))
	end
	collectgarbage'step'
end
