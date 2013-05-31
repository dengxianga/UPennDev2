module(..., package.seeall);
--SJ: IK based lookGoal to take account of bodytilt


require('Body')
require('Config')
require('vcm')
require('wcm')

t0 = 0;
yawSweep = Config.fsm.headLookGoal.yawSweep;
yawMax = Config.head.yawMax;
dist = Config.fsm.headReady.dist;
tScan = Config.fsm.headLookGoal.tScan;
minDist = Config.fsm.headLookGoal.minDist;

min_eta_look = Config.min_eta_look or 2.0;

function entry()
  print(_NAME.." entry");

  t0 = Body.get_time();
  attackAngle = wcm.get_attack_angle();
  defendAngle = wcm.get_defend_angle();
  attackClosest = math.abs(attackAngle) < math.abs(defendAngle);
  if attackClosest then
    yaw0 = wcm.get_attack_angle();
  else
    yaw0 = wcm.get_defend_angle();
  end
end

function update()
  local t = Body.get_time();
  local tpassed=t-t0;
  local ph= tpassed/tScan;
  local yawbias = (ph-0.5)* yawSweep;

  height=vcm.get_camera_height();

  yaw1 = math.min(math.max(yaw0+yawbias, -yawMax), yawMax);
  local yaw, pitch =HeadTransform.ikineCam(
	dist*math.cos(yaw1),dist*math.sin(yaw1), height);
  Body.set_head_command({yaw, pitch});

  ball = wcm.get_ball();
  ballR = math.sqrt (ball.x^2 + ball.y^2);

  --If the player is attacker and about to reach the ball

  eta = wcm.get_team_my_eta();
  if eta<min_eta_look then
    return 'timeout';
  end

  if (t - t0 > tScan) then
    tGoal = wcm.get_goal_t();
    if (tGoal - t0 > 0) then
      return 'timeout';
    else
      return 'lost';
    end
  end
end

function exit()
end

