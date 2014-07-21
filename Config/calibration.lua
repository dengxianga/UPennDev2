module(..., package.seeall)
local vector = require'vector'

cal={}
cal[HOSTNAME] = {}
cal["asus"] = {}
cal["alvin"]={}



-- Updated date: Sat Jun 21 20:14:27 2014
cal["asus"].legBias=vector.new({
   2.024974,0.000000,0.000000,0.000000,0.000000,0.000000,
   0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,
   })*math.pi/180


-- Updated date: Sun Jun 22 19:06:55 2014
cal["alvin"].legBias=vector.new({
   0,0,0,0,0,0,
   0,0,0,0,0,0,
   })*math.pi/180


-- Updated date: Sun Jun 22 19:41:19 2014
--Front is kinda stable
cal["alvin"].legBias=vector.new({
   1.417500,0.742500,-1.417500,-1.552500,-0.540000,0.472500,
   0.472500,-2.025000,0.000000,-0.810000,0.000000,1.417500,
   })*math.pi/180


-- Updated date: Thu Jun 26 11:26:16 2014
cal["alvin"].legBias=vector.new({
   1.417500,0.742500,-0.202500,-1.552500,-0.540000,0.472500,
   0.472500,-2.025000,0.000000,-0.810000,0.000000,1.417500,
   })*math.pi/180


-- Updated date: Thu Jun 26 11:31:12 2014
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.202500,-1.552500,-0.540000,1.755000,
   0.472500,-2.025000,0.000000,-0.810000,0.000000,0.877500,
   })*math.pi/180


-- Updated date: Thu Jun 26 11:32:23 2014
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.202500,-1.552500,-0.540000,1.755000,
   0.472500,-2.025000,0.000000,-0.810000,0.000000,0.877500,
   })*math.pi/180

cal["alvin"].headBias = vector.new({-0.5,-6,0,2})*math.pi/180



-- Updated date: Sat Jul 19 02:43:12 2014
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.945000,-1.552500,-0.540000,1.755000,
   0.472500,-2.025000,0.405000,-0.810000,0.000000,1.012500,
   })*math.pi/180


-- Updated date: Sat Jul 19 16:46:35 2014
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.945000,-1.552500,0.135000,1.755000,
   0.472500,-2.025000,0.405000,-0.810000,0.472500,1.012500,
   })*math.pi/180


-- Updated date: Sun Jul 20 17:12:58 2014
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.945000,-1.552500,-0.202500,0.810000,
   0.472500,-2.025000,0.405000,-0.810000,-0.135000,-0.540000,
   })*math.pi/180


-- Updated date: Sun Jul 20 17:17:09 2014
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.945000,-1.552500,0.067500,0.810000,
   0.472500,-2.025000,0.405000,-0.810000,0.135000,-0.202500,
   })*math.pi/180


-- Updated date: Mon Jul 21 00:10:35 2014
cal["alvin"].legBias=vector.new({
   1.417500,0.405000,-0.540000,-1.552500,0.000000,0.810000,
   0.472500,-2.025000,0.405000,-0.810000,0.135000,-0.202500,
   })*math.pi/180
