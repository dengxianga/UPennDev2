module(..., package.seeall);

require('vector')
require('parse_hostname')

platform = {}; 
platform.name = 'OP'

function loadconfig(configName)
  local localConfig=require(configName);
  for k,v in pairs(localConfig) do
    Config[k]=localConfig[k];
  end
end

--Robot CFG should be loaded first to set PID values
loadconfig('Robot/Config_OP_Robot') 

--loadconfig('Walk/Config_OP_Walk')
loadconfig('Walk/Config_OP_Walk_Basic')
loadconfig('World/Config_OP_World')
loadconfig('Kick/Config_OP_Kick')
loadconfig('Vision/Config_OP_Vision')
--Location Specific Camera Parameters--
loadconfig('Vision/Config_OP_Camera_Grasp')

-- Device Interface Libraries
dev = {};
dev.body = 'OPBody'; 
dev.camera = 'OPCam';
dev.kinematics = 'OPKinematics';
dev.ip_wired = '192.168.123.255';
dev.ip_wireless = '192.168.1.255';
dev.game_control='OPGameControl';
dev.team='TeamNSL';
--dev.walk='BasicWalk';  --should be updated
dev.walk='NewNewNewWalk';
dev.kick = 'NewNewKick'

-- Game Parameters

game = {};
game.teamNumber = 18;
game.playerID = parse_hostname.get_player_id();
game.robotID = game.playerID;
game.teamColor = parse_hostname.get_team_color();
game.role = game.playerID-1; 
game.nPlayers = 5;
--------------------

game.playerID = 2;

--Default color 
game.teamColor = 0; --Blue team
--game.teamColor = 1; --Red team
--0 for goalie, 1 for attacker, 2 for defender
game.role = 1;

--FSM and behavior settings
fsm = {};
--SJ: loading FSM config  kills the variable fsm, so should be called first
loadconfig('FSM/Config_OP_FSM')
fsm.game = 'RoboCup';
fsm.head = {'GeneralPlayer'};
fsm.body = {'GeneralPlayer'};

--Behavior flags, should be defined in FSM Configs but can be overrided here
fsm.enable_obstacle_detection = 1;
fsm.kickoff_wait_enable = 0;
fsm.playMode = 3; --1 for demo, 2 for orbit, 3 for direct approach
fsm.enable_walkkick = 1;
fsm.enable_sidekick = 1;

fsm.playMode = 2; --1 for demo, 2 for orbit, 3 for direct approach
fsm.enable_walkkick = 0;
fsm.enable_sidekick = 0;


--FAST APPROACH TEST
fsm.fast_approach = 1;
fsm.bodyApproach.maxStep = 0.06;


-- Team Parameters
team = {};
team.msgTimeout = 5.0;
team.nonAttackerPenalty = 6.0; -- eta sec
team.nonDefenderPenalty = 0.5; -- dist from goal

-- keyframe files
km = {};
km.standup_front = 'km_NSLOP_StandupFromFront.lua';
km.standup_back = 'km_NSLOP_StandupFromBack.lua';

-- Low battery level
-- Need to implement this api better...
bat_low = 100; -- 10V warning

