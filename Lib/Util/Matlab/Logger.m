function ret = Logger(teamNumber, playerID);

if (nargin < 2)
  playerID = parse_hostname();
end
if (nargin < 1)
  teamNumber = 26;
end

global LOG
global CAMERADATA

if isempty(LOG)
  LOG.camera = [];
end

% create shm interface
robot = shm_robot(teamNumber, playerID);

%while (1)
% log one image
ilog = length(LOG.camera) + 1;
LOG.camera(ilog).time = ilog;


LOG.camera(ilog).yuyv = CAMERADATA.yuyv;
%LOG.camera(ilog).yuyv = robot.get_yuyv() + 0;
LOG.camera(ilog).headAngles = CAMERADATA.headAngles;  
%LOG.camera(ilog).headAngles = robot.vcmImage.get_headAngles() + 0; 
% TODO: store the IMU data
%LOG.camera(ilog).imuAngles = CAMERADATA.imuAngles;  
LOG.camera(ilog).select = CAMERADATA.select;  
%LOG.camera(ilog).select = robot.vcmImage.get_select() + 0;

if (rem(ilog,5) == 0)
  % print ticks to indicate that the logger is working
  fprintf('logged %d images',ilog);
end

if (rem(ilog, 50) == 0)
  savefile = ['log_' datestr(now,30) '.mat'];
  fprintf('\nSaving Log file: %s...', savefile)
  save(savefile, 'LOG');
  fprintf('done\n');

  % clear log
  LOG.camera = [];
end

%  pause(0.05);
%end
