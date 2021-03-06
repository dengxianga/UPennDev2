h = 0.6;
g = 9.8;
p = 0.5;
t_zmp = sqrt(h/g);



% algorithm 2: given x0, x0', zmp
% step end time variable

close all;
hold on;


t_offset = 0;
t_start = 0;



support_foot = 0;
supportY = 0.5;
torsoY = 0.3;


y0 = torsoY-supportY;
t_start = 0;




%Global coordinate
uSupportY = supportY;
y_old = 0;
uSupportX = 0;

x_start = 0;
vx_start = 0;
r_next = 0.0; %given parameter
step_x = 0.03;


new_step = 1;

for t=0:0.01:3       
    
    %Next support position
    if support_foot==0 %left support
      supportMovementY = -2*supportY;
      y1 = - (torsoY - supportY);
    else        
      supportMovementY = 2*supportY;
      y1 =  torsoY - supportY;
    end

    t_local = t-t_offset;
       
    
    
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    %Estimate current pendulum parameter from current state    
    %given: current state (y,vv)    
    
    %y(t) = y0*cosh(t/tzmp) + vy0 * tzmp*sinh(t/tzmp)
    %vt(t) = y0/tzmp*sinh(t/tzmp) + vy0*cosh(t/tzmp)        
    
    y_t = y0*cosh(t_local/t_zmp);
    vy_t = y0/t_zmp*sinh(t_local/t_zmp);
    
    %
    if t==2.5 vy_t = vy_t-1; end
    %if t==2.75 vy_t = vy_t-1; end
    %if t==4 vy_t = vy_t+0.5; end
    %
        
    y0_measured = sqrt(y_t*y_t - (vy_t*t_zmp)^2)*sign(y_t);
    t_local_measured = asinh(vy_t*t_zmp/y0_measured)*t_zmp;
    pushed=0;
    
    
    if abs(y0-y0_measured)>0.0001 || abs(t_local-t_local_measured>0.001)
       [y0 y0_measured]
       [t_local t_local_measured]
       
       
       time_to_end_before = t_end-t_local
       
       t_offset = t-t_local_measured;
       y0 = y0_measured;
       
       
       pushed=1;
       %store old x_t and v_t  
       x_t = rx + x0*cosh(t_local/t_zmp) +vx0 *t_zmp*sinh(t_local/t_zmp);
       vx_t = x0/t_zmp*sinh(t_local/t_zmp) + vx0*cosh(t_local/t_zmp);
       t_local_old = t_local;
       
       
       t_local = t_local_measured;   
           
       
       
    end
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
     
    %Calculate the switch time using newton solver
    t_end = 0.37;    
    for i=1:3
      t_start_next = t_zmp * asinh(y0/y1 * sinh(t_end/t_zmp));           
      est =  y0*cosh(t_end/t_zmp)-y1*cosh(t_start_next/t_zmp);         
      y0dot = 1/t_zmp * y0 * sinh(t_end/t_zmp); 
      err = supportMovementY-est;
      t_end_delta = err/y0dot/2;
      t_end = t_end + t_end_delta;
    end      
    t_start_next = t_zmp * asinh(y0/y1 * sinh(t_end/t_zmp));
    
    if pushed 
       time_to_end_new = t_end-t_local
    end
        
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Solve X axis parameters
    
    
    step_x = 0.03;
    if j==3 step_x = 0.12;end
    
    
   % if new_step >0  
        
      if pushed==1
        
        x_t = rx + x0*cosh(t_local/t_zmp) +vx0 *t_zmp*sinh(t_local/t_zmp);
        vx_t = x0/t_zmp*sinh(t_local/t_zmp) + vx0*cosh(t_local/t_zmp);
    
        
        xparams=linsolve(...
            [1 cosh(t_local/t_zmp)      t_zmp*sinh(t_local/t_zmp) 0;
             0 sinh(t_local/t_zmp)/t_zmp  cosh(t_local/t_zmp)  0;
             0 sinh(t_end/t_zmp)/t_zmp    cosh(t_local/t_zmp) -cosh(t_start_next/t_zmp);
             1 cosh(t_end/t_zmp)          t_zmp*sinh(t_end/t_zmp) -t_zmp*sinh(t_start_next/t_zmp)],...
             [x_t;vx_t;0;r_next+step_x]);  
            
      else
         
          xparams=linsolve(...
            [1 cosh(t_start/t_zmp)      t_zmp*sinh(t_start/t_zmp) 0;
             0 sinh(t_start/t_zmp)/t_zmp  cosh(t_start/t_zmp)  0;
             0 sinh(t_end/t_zmp)/t_zmp    cosh(t_start/t_zmp) -cosh(t_start_next/t_zmp);
             1 cosh(t_end/t_zmp)          t_zmp*sinh(t_end/t_zmp) -t_zmp*sinh(t_start_next/t_zmp)],...
             [x_start;vx_start;0;r_next+step_x]);
        
      end
   %{ 
        else
        x_t = rx + x0*cosh(t_local/t_zmp) +vx0 *t_zmp*sinh(t_local/t_zmp);
        vx_t = x0/t_zmp*sinh(t_local/t_zmp) + vx0*cosh(t_local/t_zmp);
    
        
    end
     %}   
    rx=xparams(1);
    x0=xparams(2);
    vx0=xparams(3);
     
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    
    
    
    cx = uSupportX + rx + x0*cosh(t_local/t_zmp) +vx0 *t_zmp*sinh(t_local/t_zmp);  
    cy = uSupportY + y0*cosh((t_local)/t_zmp);        
    
    subplot(2,1,1);
    hold on;
    if support_foot==0  plot(t,cy,'r.')
    else     plot(t,cy,'b.')
    end
    
    subplot(2,1,2);
    hold on;
    if support_foot==0  plot(t,cx,'r.')
    else     plot(t,cx,'b.')
    end
    
    if t>t_offset +t_end-0.01
      %Advance step
      disp('step')
      t_offset = t_offset + t_end - t_start_next;
      t_start = t_start_next;
      uSupportY = uSupportY + supportMovementY;        
      support_foot = 1-support_foot;
      y0=y1;       
      
      
      xfin = rx + x0*cosh(t_end/t_zmp)+vx0*t_zmp*sinh(t_end/t_zmp);
      x_start = xfin + rx - r_next - step_x;
      vx_start = x0/t_zmp*sinh(t_end/t_zmp) + vx0*cosh(t_end/t_zmp);
      uSupportX = uSupportX + step_x + r_next-xparams(1);     
      
      new_step = 1;
      
    end
    


    
    
    
    
    
    
end


%{
x = (x0-p)*cosh(t/t_zmp) + v0*t_zmp*sinh(t/t_zmp) + p;
plot(t,x);
%}