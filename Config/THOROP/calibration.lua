module(..., package.seeall)
local vector = require'vector'

cal={}
cal[HOSTNAME] = {}
cal["asus"] = {}
cal["alvin"]={}
cal["teddy"]={}



-- Updated date: Sat Jun 21 20:14:27 2014
cal["asus"].legBias=vector.new({
   2.024974,0.000000,0.000000,0.000000,0.000000,0.000000,
   0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,
   })*math.pi/180

-- Updated date: Sat Sep 20 17:40:14 2014
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.337500,-1.552500,-0.202500,0.810000,
   0.472500,-2.025000,0.405000,-0.810000,-0.202500,-0.202500,
   })*math.pi/180


-- Updated date: Sat Sep 20 17:40:14 2014
cal["teddy"].legBias=vector.new({
   1,0.72,0.02,-0.365,0.32,0.72,
   0.47,-0.69,1.19,-0.88,-0.66,0.03,
   })*math.pi/180


-- Updated date: Tue Sep 23 00:00:50 2014
cal["teddy"].legBias=vector.new({
   1.000000,0.720000,0.020000,-0.365000,0.320000,0.720000,
   0.470000,-0.690000,1.190000,-0.880000,-0.660000,-0.375000,
   })*math.pi/180

-- Updated date: Mon Jan 19 15:01:27 2015
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.337500,-1.552500,-0.202500,0.810000,
   0.472500,-2.025000,0.405000,-0.810000,-0.202500,-0.270000,
   })*math.pi/180


-- Updated date: Mon Jan 19 15:02:27 2015
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.337500,-1.552500,-0.202500,1.417500,
   0.472500,-2.025000,0.405000,-0.810000,-0.202500,-0.000000,
   })*math.pi/180




-- Updated date: Tue Jan 20 18:41:50 2015
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.337500,-1.552500,-0.202500,1.282500,
   0.472500,-2.025000,0.405000,-0.810000,-0.202500,1.012500,
   })*math.pi/180


-- Updated date: Tue Jan 20 18:48:48 2015
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.337500,-1.552500,-0.202500,1.282500,
   0.472500,-2.025000,0.405000,-0.810000,-0.202500,0.540000,
   })*math.pi/180


-- Updated date: Wed Jan 21 12:54:12 2015
cal["alvin"].legBias=vector.new({
   1.417500,-0.337500,-0.337500,-1.552500,0.607500,0.000000,
   0.472500,-0.405000,0.405000,-0.810000,-0.000000,1.147500,
   })*math.pi/180


-- Updated date: Wed Jan 21 12:54:45 2015
cal["alvin"].legBias=vector.new({
   1.417500,2.025000,-0.337500,-1.552500,0.607500,0.000000,
   0.472500,-2.025000,0.405000,-0.810000,-0.000000,1.147500,
   })*math.pi/180


-- Updated date: Wed Jan 21 13:00:07 2015
cal["alvin"].legBias=vector.new({
   1.417500,2.902500,-0.337500,-1.552500,0.607500,1.147500,
   0.472500,-2.025000,0.405000,-0.810000,-0.000000,1.147500,
   })*math.pi/180


-- Updated date: Fri Jan 23 14:13:48 2015
cal["alvin"].legBias=vector.new({
   1.350000,1.552500,-0.337500,-1.552500,0.607500,1.147500,
   1.215000,-1.552500,0.405000,-0.810000,-0.000000,1.147500,
   })*math.pi/180


-- Updated date: Fri Jan 23 14:37:58 2015
cal["alvin"].legBias=vector.new({
   1.350000,1.552500,-0.337500,-1.552500,0.607500,1.147500,
   1.215000,-1.552500,0.405000,-0.810000,-0.000000,1.147500,
   })*math.pi/180


-- Updated date: Tue Jan 27 20:18:09 2015
cal["alvin"].legBias=vector.new({
   1.350000,1.552500,-0.337500,-1.552500,0.607500,1.147500,
   1.147500,-1.552500,0.405000,-0.810000,-0.000000,1.147500,
   })*math.pi/180


-- Updated date: Tue Jan 27 20:30:44 2015
cal["alvin"].legBias=vector.new({
   1.350000,0.675000,-0.337500,-1.552500,0.607500,1.147500,
   1.147500,-1.552500,0.405000,-0.810000,-0.000000,1.147500,
   })*math.pi/180



-- Updated date: Mon Feb  2 10:19:46 2015
cal["alvin"].legBias=vector.new({
   1.350000,0.675000,-0.337500,-1.552500,0.000000,1.147500,
   1.147500,-1.552500,0.405000,-0.810000,-0.000000,0.067500,
   })*math.pi/180


-- Updated date: Sat Mar  7 17:22:26 2015
cal["alvin"].legBias=vector.new({
   1.350000,0.675000,-0.337500,-1.552500,-0.540000,0.337500,
   1.147500,-1.552500,0.405000,-0.810000,-0.000000,2.025000,
   })*math.pi/180


-- Updated date: Sat Mar  7 17:26:47 2015
cal["alvin"].legBias=vector.new({
   1.350000,0.877500,-0.337500,-1.552500,-0.540000,0.135000,
   1.147500,-0.742500,0.405000,-0.810000,-0.000000,1.147500,
   })*math.pi/180


-- Updated date: Sat Mar  7 17:40:18 2015
cal["alvin"].legBias=vector.new({
   1.350000,1.080000,-0.337500,-1.552500,-0.540000,0.135000,
   1.147500,-1.215000,0.405000,-0.810000,-0.000000,0.742500,
   })*math.pi/180


-- Updated date: Thu Mar 12 10:21:29 2015
cal["alvin"].legBias=vector.new({
   1.350000,1.485000,-0.337500,-1.552500,-0.540000,0.135000,
   1.147500,-0.405000,0.405000,-0.810000,-0.000000,0.742500,
   })*math.pi/180


-- Updated date: Thu Mar 12 10:31:41 2015
cal["alvin"].legBias=vector.new({
   1.350000,1.485000,-0.337500,-1.552500,-0.540000,0.135000,
   1.147500,-0.405000,0.405000,-0.810000,-0.000000,0.742500,
   })*math.pi/180


-- Updated date: Thu Mar 12 10:32:46 2015
cal["alvin"].legBias=vector.new({
   1.350000,1.485000,-0.337500,-1.552500,-0.540000,0.135000,
   1.147500,-0.405000,0.405000,-0.810000,-0.135000,0.742500,
   })*math.pi/180


-- Updated date: Thu Mar 12 10:33:00 2015
cal["alvin"].legBias=vector.new({
   1.350000,1.485000,-0.337500,-1.552500,-0.540000,0.135000,
   1.147500,-0.405000,0.405000,-0.810000,-0.135000,0.742500,
   })*math.pi/180


-- Updated date: Thu Mar 12 14:28:09 2015
cal["alvin"].legBias=vector.new({
   1.350000,1.485000,-0.337500,-1.552500,-0.540000,0.135000,
   1.147500,-0.405000,0.405000,-0.810000,-0.877500,0.742500,
   })*math.pi/180

-- Updated date: Tue Mar 31 19:17:43 2015
cal["alvin"].legBias=vector.new({
   1.35,1.48,-0.337500,-1.55, -0.54,0.13,
   1.14,-0.405,-0.335000,-0.81,-0.08,0.7425,
   })*math.pi/180

-- Updated date: Wed Apr  8 15:58:15 2015
--[[
cal["alvin"].legBias=vector.new({
   0.472500,1.750000,0.202500,-3.912500,0.877500,0.130000,
   -0.277500,-2.092500,-0.335000,-0.810000,-0.080000,0.742500,
   })*math.pi/180
--]]
