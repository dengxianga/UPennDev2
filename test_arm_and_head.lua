dofile'include.lua'
local Body = require'Body'
local getch = require'getch'
local util = require'util'
local vector = require'vector'
local color = util.color
local running = true
local tasks = {}

local is_red = Body.get_head_led_red()[1]
function tasks.r()
	is_red = 255 - is_red
	Body.set_head_led_red( is_red )
	return is_red>0 and "RED ON" or "RED OFF"
end

local is_blue = Body.get_head_led_blue()[1]
function tasks.b()
	is_blue = 255 - is_blue
	Body.set_head_led_blue(is_blue)
	return is_blue>0 and "BLUE ON" or "BLUE OFF"
end

local active_arm = 'larm'
function tasks.a()
	active_arm = active_arm=='larm' and 'rarm' or 'larm'
end

function tasks.g()
	running = false
	return "Done"
end

tasks['+'] = function()
	local delta = vector.new({5, 0}) * DEG_TO_RAD
	Body.set_larm_command_position(
		Body.get_larm_command_position() + delta
	)
	return "Increase Roll"
end
tasks['='] = function()
	local delta = vector.new({0, 5}) * DEG_TO_RAD
	Body.set_larm_command_position(
		Body.get_larm_command_position() + delta
	)
	return "Increase Pitch"
end

tasks['_'] = function()
	local delta = -1*vector.new({5, 0}) * DEG_TO_RAD
	Body.set_larm_command_position(
		Body.get_larm_command_position() + delta
	)
	return "Increase Roll"
end
tasks['-'] = function()
	local delta = -1*vector.new({0, 5}) * DEG_TO_RAD
	Body.set_larm_command_position(
		Body.get_larm_command_position() + delta
	)
	return "Increase Pitch"
end

local tq = Body.get_larm_torque_enable()
tasks.t = function()
	local delta = vector.new({0, 5}) * DEG_TO_RAD
	tq = vector.ones(#tq) - tq
	Body.set_larm_torque_enable(
		tq
	)
	return tq[1]==0 and "Torquing off..." or "Torquing on..."
end

local function process_kb()
	local keycode = getch.block()
	if not keycode then return end
	local char = string.char(keycode)
	local char_lower = string.lower(char)
	local func = tasks[char]
	if func then return func() end
	local func = tasks[char_lower]
	if func then return func() end
end

local function print_menu(msg)
	os.execute("clear")
	local menu_tbl = {
		color("Head LED Red", 'red')..": Press r",
		color("Head LED Green", 'green')..": Press g",
		color("Head LED Blue", 'blue')..": Press b",
		color( active_arm=='larm' and 'Right Arm' or 'Left Arm' , 'yellow').." Press a to swap",
		color("Torque "..(tq[1]==0 and 'ON' or 'OFF'), "magenta"),
		"Quit: g",
		'',
		"LArm: "..tostring(Body.get_larm_position()),
		"RArm: "..tostring(Body.get_rarm_position()),
		'',
		msg or '',
	}
	print(table.concat(menu_tbl, '\n'))
end

local msg
print_menu()
while running do
	print_menu( process_kb() )
end
